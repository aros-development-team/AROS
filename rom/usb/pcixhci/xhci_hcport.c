/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver hc port support functions
*/

#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"

static const char strXhciPortTaskName[] = "xHCI port task";

#if defined(DEBUG) && defined(XHCI_LONGDEBUGNAK)
#define XHCI_NAKTOSHIFT         (8)
#else
#define XHCI_NAKTOSHIFT         (3)
#endif

#ifdef base
#undef base
#endif
#define base (hc->hc_Device)

#if defined(AROS_USE_LOGRES)
#ifdef LogHandle
#undef LogHandle
#endif
#ifdef LogResBase
#undef LogResBase
#endif
#define LogHandle (hc->hc_LogRHandle)
#define LogResBase (base->hd_LogResBase)
#endif

/*
 * PORTSC write handling:
 *
 * Do NOT write back the full PORTSC value as read: it contains RO fields and
 * link/speed state. Writing those back (even unchanged) can cause undefined
 * behavior on some controllers/VMs.
 *
 * Instead, only write:
 *  - RW fields we deliberately change (PP/PR/WPR and, for legacy disable, PED)
 *  - RW1C change bits we want to clear
 *
 * Note: Some controllers tolerate writing PED=0 to "disable"; others may ignore
 * it. We keep this behavior for now, but still mask to avoid disturbing RO bits.
 */
#define XHCI_PORTSC_RW_MASK \
    (XHCIF_PR_PORTSC_PP | XHCIF_PR_PORTSC_PR | XHCIF_PR_PORTSC_WPR | XHCIF_PR_PORTSC_PED)

#define XHCI_PORTSC_RW1C_MASK \
    (XHCIF_PR_PORTSC_CSC | XHCIF_PR_PORTSC_PEC | XHCIF_PR_PORTSC_PLC | \
     XHCIF_PR_PORTSC_OCC | XHCIF_PR_PORTSC_PRC | XHCIF_PR_PORTSC_WRC | \
     XHCIF_PR_PORTSC_CEC)

/* Compose a masked PORTSC write: preserve only the RW bits, plus requested RW1C clears. */
static inline ULONG xhciPortscComposeWrite(ULONG oldportsc, ULONG set_rw_bits, ULONG clear_rw_bits, ULONG rw1c_bits)
{
    ULONG v = oldportsc & XHCI_PORTSC_RW_MASK;
    v |= (set_rw_bits & XHCI_PORTSC_RW_MASK);
    v &= ~(clear_rw_bits & XHCI_PORTSC_RW_MASK);

    /* RW1C bits are cleared by writing 1s. */
    v |= (rw1c_bits & XHCI_PORTSC_RW1C_MASK);

    return v;
}

/* RW1C helper: change bits we commonly clear around reset/state transitions */
static inline ULONG xhciPortscClearChangeBits(ULONG v)
{
    v |= XHCI_PORTSC_RW1C_MASK;
    return v;
}

static struct pciusbXHCIDevice *xhciFindPortDevice(struct PCIController *hc, UWORD hciport)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    UWORD maxslot = xhcic->xhc_NumSlots;

    if(maxslot >= USB_DEV_MAX)
        maxslot = USB_DEV_MAX - 1;

    for(UWORD slot = 1; slot <= maxslot; slot++) {
        struct pciusbXHCIDevice *devCtx = xhcic->xhc_Devices[slot];

        if(devCtx && (devCtx->dc_RootPort == hciport))
            return devCtx;
    }

    return NULL;
}

static void xhciLogPortState(struct PCIController *hc,
                             volatile struct xhci_pr *xhciports,
                             UWORD hciport,
                             const char *context)
{
    ULONG portsc   = AROS_LE2LONG(xhciports[hciport].portsc);
    ULONG portpmsc = AROS_LE2LONG(xhciports[hciport].portpmsc);
    ULONG portli   = AROS_LE2LONG(xhciports[hciport].portli);
    ULONG hlpmc    = AROS_LE2LONG(xhciports[hciport].porthlpmc);

    ULONG linkState = xhciPortLinkState(portsc);
    ULONG speedBits = xhciPortSpeedBits(portsc);
    BOOL  isUsb3    = xhciPortIsUsb3(hc, hciport, portsc);

    struct pciusbXHCIDevice *devCtx = xhciFindPortDevice(hc, hciport);
    ULONG routeString = devCtx ? devCtx->dc_RouteString : 0;

    const char *speedDesc = "Unknown";
    if(speedBits == XHCIF_PR_PORTSC_LOWSPEED) {
        speedDesc = "USB2-LS";
    } else if(speedBits == XHCIF_PR_PORTSC_FULLSPEED) {
        speedDesc = "USB2-FS";
    } else if(speedBits == XHCIF_PR_PORTSC_HIGHSPEED) {
        speedDesc = "USB2-HS";
    } else if(speedBits == XHCIF_PR_PORTSC_SUPERSPEED) {
        speedDesc = "USB3-SS";
    } else if(speedBits == XHCIF_PR_PORTSC_SUPERSPEEDPLUS) {
        speedDesc = "USB3-SS+";
    }

    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "xHCI: Port %u %s route 0x%05lx link %lu speed %s" DEBUGCOLOR_RESET " \n",
                    hciport + 1,
                    context,
                    routeString,
                    linkState,
                    speedDesc);

    if(isUsb3) {
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Port %u %s PORTPMSC $%08lx PORTLI $%08lx" DEBUGCOLOR_RESET " \n",
                        hciport + 1,
                        context,
                        portpmsc,
                        portli);
    } else {
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Port %u %s PORTPMSC $%08lx PPM $%08lx" DEBUGCOLOR_RESET " \n",
                        hciport + 1,
                        context,
                        portpmsc,
                        hlpmc);
    }
}

static inline BOOL xhciPortIsSuperSpeedLink(ULONG portsc)
{
    ULONG speed = portsc & XHCI_PR_PORTSC_SPEED_MASK;
    return (speed == XHCIF_PR_PORTSC_SUPERSPEED) ||
           (speed == XHCIF_PR_PORTSC_SUPERSPEEDPLUS);
}

/* Issue a reset appropriate to negotiated speed (SS => WPR, otherwise PR). */
static inline ULONG xhciComposePortResetWrite(struct PCIController *hc, UWORD hciport, ULONG oldportsc)
{
    ULONG rw1c = XHCI_PORTSC_RW1C_MASK;

    /* If not connected, don't try to reset (caller usually already checks CCS). */
    if(!(oldportsc & XHCIF_PR_PORTSC_CCS))
        return 0;

    /*
     * Choose by negotiated speed, not "port kind".
     * This avoids issuing WPR when a USB2 device is attached on a port that
     * the platform considers "USB3-capable".
     */
    if(xhciPortIsSuperSpeedLink(oldportsc)) {
        return xhciPortscComposeWrite(oldportsc,
                                      XHCIF_PR_PORTSC_WPR,   /* set */
                                      XHCIF_PR_PORTSC_PR,    /* clear */
                                      rw1c);
    }

    return xhciPortscComposeWrite(oldportsc,
                                  XHCIF_PR_PORTSC_PR,      /* set */
                                  XHCIF_PR_PORTSC_WPR,     /* clear */
                                  rw1c);
}

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc,
                    UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_pr *xhciports =
        (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);
    BOOL  cmdgood = FALSE;
    ULONG oldval  = AROS_LE2LONG(xhciports[hciport].portsc);
    ULONG writeval = 0;
    ULONG tmpval;
    struct pciusbXHCIDevice *devCtx;

    pciusbXHCIDebug("xHCI",
                    DEBUGFUNCCOLOR_SET "xHCI: %s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)"
                    DEBUGCOLOR_RESET " \n",
                    __func__, unit, hc, hciport, idx, val, retval);

    xhciDumpPort(&xhciports[hciport]);
    xhciLogPortState(hc, xhciports, hciport, "before SET_FEATURE");

    switch(val) {

    case UFS_PORT_ENABLE:
        /*
         * Hub-class "ENABLE" request:
         * - USB2: if connected and not enabled, a PR reset is the usual way
         *         to transition the port into enabled state.
         * - USB3: do NOT key on PED. Treat "operational" as CCS + PLS==U0.
         *         If connected but not operational, issue WPR.
         */
        if(!(oldval & XHCIF_PR_PORTSC_CCS)) {
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: PORT_ENABLE ignored (not connected)"
                            DEBUGCOLOR_RESET " \n");
            cmdgood = TRUE;
            break;
        }

        if(xhciPortIsUsb3(hc, hciport, oldval)) {
            if(xhciUsb3Operational(oldval)) {
                pciusbXHCIDebug("xHCI",
                                DEBUGCOLOR_SET "xHCI: PORT_ENABLE (USB3) already operational (U0)"
                                DEBUGCOLOR_RESET " \n");
                cmdgood = TRUE;
                break;
            }

            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: PORT_ENABLE (USB3) issuing Warm Reset (WPR)"
                            DEBUGCOLOR_RESET " \n");
            writeval = xhciComposePortResetWrite(hc, hciport, oldval);
            cmdgood = TRUE;
        } else {
            if(oldval & XHCIF_PR_PORTSC_PED) {
                pciusbXHCIDebug("xHCI",
                                DEBUGCOLOR_SET "xHCI: PORT_ENABLE (USB2) already enabled"
                                DEBUGCOLOR_RESET " \n");
                cmdgood = TRUE;
                break;
            }

            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: PORT_ENABLE (USB2) issuing Reset (PR)"
                            DEBUGCOLOR_RESET " \n");
            writeval = xhciComposePortResetWrite(hc, hciport, oldval);
            cmdgood = TRUE;
        }
        break;

    case UFS_PORT_SUSPEND:
        /*
         * Proper suspend/resume would manipulate PLS (USB3 U-states vs USB2 suspend)
         * and requires careful sequencing and feature support checks.
         * For now, remain a no-op beyond logging.
         */
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Suspending Port (NOP)"
                        DEBUGCOLOR_RESET " \n");
        cmdgood = TRUE;
        break;

    case UFS_PORT_RESET:
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Performing Port Reset"
                        DEBUGCOLOR_RESET " \n");

        /*
         * If there was a device and we own a slot for it...
         *
         * - During initial bring-up, this driver creates a slot at address 0
         *   (Address Device command) before the stack issues SET_ADDRESS.
         *   A hub-driver PORT_RESET in that window is part of normal
         *   enumeration; do not tear down the slot.
         *
         * - For an already addressed device (dc_DevAddr != 0), PORT_RESET is
         *   treated as a re-enumeration trigger; drop the old context.
         */
        devCtx = xhciFindPortDevice(hc, hciport);
        if(devCtx && devCtx->dc_DevAddr != 0) {
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: PORT_RESET: disabling device slot (%u) for re-enumeration"
                            DEBUGCOLOR_RESET "\n", devCtx->dc_SlotID);

            xhciDisconnectDevice(hc, devCtx, unit->hu_TimerReq);
        }

        if(oldval & XHCIF_PR_PORTSC_CCS) {
            if(xhciPortIsUsb3(hc, hciport, oldval)) {
                pciusbXHCIDebug("xHCI",
                                DEBUGCOLOR_SET "xHCI:     >Warm Reset (WPR) for USB3"
                                DEBUGCOLOR_RESET " \n");
            } else {
                pciusbXHCIDebug("xHCI",
                                DEBUGCOLOR_SET "xHCI:     >Reset (PR) for USB2"
                                DEBUGCOLOR_RESET " \n");
            }

            writeval = xhciComposePortResetWrite(hc, hciport, oldval);

            /* Enable enumeration again on this port. */
            unit->hu_DevControllers[0] = hc;
        } else {
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: PORT_RESET ignored (not connected)"
                            DEBUGCOLOR_RESET " \n");
        }

        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER:
        /*
         * If controller does not support per-port power control (PPC=0),
         * ports are always powered. Treat this as a NOP and succeed.
         */
        if(!(hc->hc_Flags & HCF_PPC)) {
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: PORT_POWER set (PPC=0, NOP)"
                            DEBUGCOLOR_RESET " \n");
            cmdgood = TRUE;
            break;
        }

        if(!(oldval & XHCIF_PR_PORTSC_PP)) {
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: Powering Port"
                            DEBUGCOLOR_RESET " \n");
            writeval = xhciPortscComposeWrite(oldval, XHCIF_PR_PORTSC_PP, 0, 0);
        } else {
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: Port power already on"
                            DEBUGCOLOR_RESET " \n");
            /* No write required. */
            writeval = 0;
        }
        cmdgood = TRUE;
        break;
    }

    if(cmdgood && writeval) {
        xhciports[hciport].portsc = AROS_LONG2LE(writeval);
        tmpval = AROS_LE2LONG(xhciports[hciport].portsc);

        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Port %ld SET_FEATURE write $%08lx (old $%08lx, now $%08lx)"
                        DEBUGCOLOR_RESET " \n",
                        idx, writeval, oldval, tmpval);

        xhciLogPortState(hc, xhciports, hciport, "after SET_FEATURE");
    } else if(cmdgood) {
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Port %ld SET_FEATURE no-write (old $%08lx)"
                        DEBUGCOLOR_RESET " \n",
                        idx, oldval);
    }

    return cmdgood;
}

BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc,
                      UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_pr *xhciports =
        (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);
    ULONG oldval    = AROS_LE2LONG(xhciports[hciport].portsc);
    ULONG writeval  = 0;
    ULONG clearbits = 0;
    BOOL  cmdgood   = FALSE;
    struct pciusbXHCIDevice *devCtx;

    pciusbXHCIDebug("xHCI",
                    DEBUGFUNCCOLOR_SET "xHCI: %s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)"
                    DEBUGCOLOR_RESET " \n",
                    __func__, unit, hc, hciport, idx, val, retval);

    xhciDumpPort(&xhciports[hciport]);
    xhciLogPortState(hc, xhciports, hciport, "before CLEAR_FEATURE");

    switch(val) {

    case UFS_PORT_ENABLE:
        /*
         * ClearFeature(PORT_ENABLE) disables the port.
         * For xHCI, "port disable" semantics are controller-defined; we keep the
         * existing behavior of clearing PED plus tearing down the slot.
         */
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Disabling Port"
                        DEBUGCOLOR_RESET " \n");

        /* Stop enumeration on this port and free any context. */
        unit->hu_DevControllers[0] = NULL;

        devCtx = xhciFindPortDevice(hc, hciport);
        if(devCtx) {
            xhciDisconnectDevice(hc, devCtx, unit->hu_TimerReq);
        }

        /* Attempt to clear PED via masked write (won't disturb RO fields). */
        writeval = xhciPortscComposeWrite(oldval, 0, XHCIF_PR_PORTSC_PED, 0);
        cmdgood = TRUE;
        break;

    case UFS_PORT_SUSPEND:
        /* Currently treated as a no-op beyond logging. */
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Clearing Suspend (NOP)"
                        DEBUGCOLOR_RESET " \n");
        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER:
        /*
         * If no PPC, ports are "always powered" and PORT_POWER clear is a NOP.
         */
        if(!(hc->hc_Flags & HCF_PPC)) {
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: PORT_POWER clear (PPC=0, NOP)"
                            DEBUGCOLOR_RESET " \n");
            cmdgood = TRUE;
            break;
        }

        if(oldval & XHCIF_PR_PORTSC_PP) {
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: Powering Down Port"
                            DEBUGCOLOR_RESET " \n");
            writeval = xhciPortscComposeWrite(oldval, 0, XHCIF_PR_PORTSC_PP, 0);
        } else {
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "xHCI: Port power already off"
                            DEBUGCOLOR_RESET " \n");
            writeval = 0;
        }
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_CONNECTION:
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Clearing Connection Change"
                        DEBUGCOLOR_RESET " \n");
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_CONNECTION;
        clearbits |= XHCIF_PR_PORTSC_CSC;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_ENABLE:
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Clearing Enable Change"
                        DEBUGCOLOR_RESET " \n");
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_ENABLE;
        clearbits |= XHCIF_PR_PORTSC_PEC;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_SUSPEND:
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Clearing Suspend Change"
                        DEBUGCOLOR_RESET " \n");
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_SUSPEND;
        clearbits |= XHCIF_PR_PORTSC_PLC;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_OVER_CURRENT:
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Clearing Over-Current Change"
                        DEBUGCOLOR_RESET " \n");
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_OVER_CURRENT;
        clearbits |= XHCIF_PR_PORTSC_OCC;
        cmdgood = TRUE;
        break;

    case UFS_C_PORT_RESET:
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Clearing Reset Change"
                        DEBUGCOLOR_RESET " \n");
        hc->hc_PortChangeMap[hciport] &= ~UPSF_PORT_RESET;
        /* Clear both USB2 (PRC) and USB3 (WRC) reset-change bits */
        clearbits |= (XHCIF_PR_PORTSC_PRC | XHCIF_PR_PORTSC_WRC);
        cmdgood = TRUE;
        break;
    }

    if(cmdgood) {
        /*
         * For RW1C change bits, write 1 to clear. Do not write back the full
         * PORTSC; use a masked write.
         */
        if(clearbits) {
            ULONG rw1c = clearbits & XHCI_PORTSC_RW1C_MASK;
            ULONG v = xhciPortscComposeWrite(oldval, 0, 0, rw1c);
            /* If we also had a direct writeval request, merge it (still masked). */
            if(writeval) {
                v = xhciPortscComposeWrite(oldval,
                                           (writeval & XHCI_PORTSC_RW_MASK),
                                           (~writeval) & XHCI_PORTSC_RW_MASK,
                                           rw1c);
            }
            writeval = v;
        }

        if(writeval) {
            xhciports[hciport].portsc = AROS_LONG2LE(writeval);
            writeval = AROS_LE2LONG(xhciports[hciport].portsc);
        }

        if(hc->hc_PortChangeMap[hciport]) {
            hc->hc_Unit->hu_RootPortChanges |= (1UL << (hciport + 1));
        } else {
            hc->hc_Unit->hu_RootPortChanges &= ~(1UL << (hciport + 1));
        }

        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Port %ld CLEAR_FEATURE old $%08lx write $%08lx"
                        DEBUGCOLOR_RESET " \n",
                        idx, oldval, writeval);

        xhciLogPortState(hc, xhciports, hciport, "after CLEAR_FEATURE");
    }

    return cmdgood;
}

BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr,
                   UWORD hciport, UWORD idx, WORD *retval)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);
    volatile struct xhci_pr *xhciports =
        (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);
    ULONG oldportsc = AROS_LE2LONG(xhciports[hciport].portsc);

    pciusbXHCIDebug("xHCI",
                    DEBUGFUNCCOLOR_SET "xHCI: %s(0x%p, 0x%p, %04x, %04x, 0x%p)"
                    DEBUGCOLOR_RESET " \n",
                    __func__, hc, mptr, hciport, idx, retval);

    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "xHCI: %s: xhci-portsc = $%08x"
                    DEBUGCOLOR_RESET " \n",
                    __func__, oldportsc);

    xhciDumpPort(&xhciports[hciport]);

    /*
     * First word: wPortStatus
     */
    *mptr = 0;

    /*
     * Port power:
     * - If PPC == 0 (no per-port power), ports are always powered.
     * - If PPC != 0, use PP bit.
     */
    if(!(hc->hc_Flags & HCF_PPC)) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
    } else if(oldportsc & XHCIF_PR_PORTSC_PP) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
    }

    if(oldportsc & XHCIF_PR_PORTSC_OCA) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_OVER_CURRENT);
    }
    if(oldportsc & XHCIF_PR_PORTSC_CCS) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
    }

    /*
     * Enable reporting:
     * - USB2: use PED.
     * - USB3: treat CCS+U0 as enabled/operational for hub model.
     */
    if(xhciPortIsUsb3(hc, hciport, oldportsc)) {
        if(xhciUsb3Operational(oldportsc)) {
            *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
        }
    } else {
        if(oldportsc & XHCIF_PR_PORTSC_PED) {
            *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
        }
    }

    /*
     * Speed mapping:
     * - FS is the default (no explicit FS flag in UPSF model)
     * - LS/HS set the corresponding USB2 flags
     * - SS/SS+ map to UPSF_PORT_SUPER_SPEED (no distinct SS+ flag)
     */
    switch(oldportsc & XHCI_PR_PORTSC_SPEED_MASK) {
    case XHCIF_PR_PORTSC_LOWSPEED:
        *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
        break;

    case XHCIF_PR_PORTSC_HIGHSPEED:
        *mptr |= AROS_WORD2LE(UPSF_PORT_HIGH_SPEED);
        break;

    case XHCIF_PR_PORTSC_SUPERSPEED:
    case XHCIF_PR_PORTSC_SUPERSPEEDPLUS:
        *mptr |= AROS_WORD2LE(UPSF_PORT_SUPER_SPEED);
        break;

    default:
        /* Full-speed or unknown encoding: no extra flags. */
        break;
    }

    /*
     * Reset in progress:
     * - USB2 uses PR.
     * - USB3 uses WPR.
     */
    if((oldportsc & XHCIF_PR_PORTSC_PR) ||
            (oldportsc & XHCIF_PR_PORTSC_WPR)) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
    }

    /*
     * Suspend:
     * Keep the legacy USB2 heuristic only; USB3 PLS encodes U-states.
     */
    if(!xhciPortIsUsb3(hc, hciport, oldportsc)) {
        if(((oldportsc >> XHCIS_PR_PORTSC_PLS) & XHCI_PR_PORTSC_PLS_SMASK) == 2) {
            *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);
        }
    }

    /* Port indicator control: bits 14..15 */
    if((oldportsc >> 14) & 0x3) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_INDICATOR);
    }

    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "xHCI: Port %ld Status $%08lx"
                    DEBUGCOLOR_RESET " \n",
                    idx, *mptr);

    /*
     * Second word: wPortChange
     */
    mptr++;

    if(oldportsc & XHCIF_PR_PORTSC_PEC) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
    }
    if(oldportsc & XHCIF_PR_PORTSC_CSC) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
    }
    if(oldportsc & XHCIF_PR_PORTSC_PLC) {
        hc->hc_PortChangeMap[hciport] |= (UPSF_PORT_SUSPEND | UPSF_PORT_ENABLE);
    }
    if(oldportsc & XHCIF_PR_PORTSC_OCC) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
    }
    if((oldportsc & XHCIF_PR_PORTSC_WRC) ||
            (oldportsc & XHCIF_PR_PORTSC_PRC)) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
    }

    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);

    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "xHCI: Port %ld Change $%08lx"
                    DEBUGCOLOR_RESET " \n",
                    idx, *mptr);

    return FALSE;
}

AROS_UFH0(void, xhciPortTask)
{
    AROS_USERFUNC_INIT

    volatile struct xhci_pr *xhciports;
    struct PCIController *hc;
    struct pciusbXHCIDevice *devCtx;
    struct XhciHCPrivate *xhcic;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    {
        struct Task *thistask;
        BOOL timer_ok;

        thistask = FindTask(NULL);
        hc = thistask->tc_UserData;
        xhcic = xhciGetHCPrivate(hc);
        xhcic->xhc_PortTask.xpt_Task = thistask;
        SetTaskPri(thistask, 99);

        timer_ok = xhciOpenTaskTimer(&xhcic->xhc_PortTask.xpt_TimerPort,
                                     &xhcic->xhc_PortTask.xpt_TimerReq,
                                     strXhciPortTaskName);
        if(!timer_ok) {
            pciusbError("xHCI", DEBUGWARNCOLOR_SET "%s: unable to open timer.device" DEBUGCOLOR_RESET" \n", __func__);
        }
    }
    xhcic->xhc_PortTask.xpt_PortChangeSignal = AllocSignal(-1);

    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "'%s' @ 0x%p, PortChangeSignal=%d"
                    DEBUGCOLOR_RESET" \n",
                    ((struct Node *)xhcic->xhc_PortTask.xpt_Task)->ln_Name,
                    xhcic->xhc_PortTask.xpt_Task,
                    xhcic->xhc_PortTask.xpt_PortChangeSignal);

    if(xhcic->xhc_ReadySigTask)
        Signal(xhcic->xhc_ReadySigTask, 1L << xhcic->xhc_ReadySignal);

    if(!xhcic->xhc_PortTask.xpt_TimerReq || xhcic->xhc_PortTask.xpt_PortChangeSignal == -1)
        goto task_cleanup;

    xhciports = (volatile struct xhci_pr *)((IPTR)xhcic->xhc_XHCIPorts);

    for(;;) {
        ULONG xhcictsigs = Wait(1 << xhcic->xhc_PortTask.xpt_PortChangeSignal);
        if(xhcictsigs & (1 << xhcic->xhc_PortTask.xpt_PortChangeSignal)) {
            UWORD hciport;
            pciusbXHCIDebug("xHCI",
                            DEBUGCOLOR_SET "Port change detected" DEBUGCOLOR_RESET" \n");
            for(hciport = 0; hciport < hc->hc_NumPorts; hciport++) {
                ULONG portsc  = AROS_LE2LONG(xhciports[hciport].portsc);
                devCtx = xhciFindPortDevice(hc, hciport);

                /*
                 * USB3 ports do not use PED semantics reliably; treat "enabled"
                 * as "operational" (CCS + U0) for SS/SS+.
                 */
                BOOL connected = xhciHubPortConnected(portsc);
                BOOL enabled   = xhciHubPortEnabled(hc, hciport, portsc);

                pciusbDebug("xHCI",
                            DEBUGCOLOR_SET "port %d, connected = %d, enabled = %d" DEBUGCOLOR_RESET" \n", hciport, connected, enabled);

                if(connected && enabled && (!devCtx)) {
                    UWORD rootPort = hciport;     /* 0-based */
                    ULONG route    = 0;           /* root-attached */
                    ULONG flags    = 0;
                    UWORD mps0     = 8;           /* safe default */

                    /* Decode port speed from PORTSC */
                    ULONG speedBits = portsc & XHCI_PR_PORTSC_SPEED_MASK;

                    if(speedBits == XHCIF_PR_PORTSC_LOWSPEED) {
                        flags |= UHFF_LOWSPEED;
                        mps0   = 8;              /* LS EP0 = 8 bytes */
                    } else if(speedBits == XHCIF_PR_PORTSC_FULLSPEED) {
                        /* FS EP0 is 8/16/32/64 - start with 8 until bMaxPacketSize0 is known */
                        mps0   = 8;
                    } else if(speedBits == XHCIF_PR_PORTSC_HIGHSPEED) {
                        flags |= UHFF_HIGHSPEED;
                        mps0   = 64;             /* HS EP0 typically 64 bytes */
                    } else if(speedBits == XHCIF_PR_PORTSC_SUPERSPEED ||
                              speedBits == XHCIF_PR_PORTSC_SUPERSPEEDPLUS) {
                        flags |= UHFF_SUPERSPEED;
                        mps0   = 512;            /* SS EP0 max packet size */
                    } else {
                        /* Unknown/invalid speed - leave defaults, enumeration will adjust */
                    }

                    devCtx = xhciCreateDeviceCtx(hc,
                                                 rootPort,
                                                 route,
                                                 flags,
                                                 mps0,
                                                 xhcic->xhc_PortTask.xpt_TimerReq);
                    if(devCtx) {
                        /* Root port "device" lives on this controller */
                        hc->hc_Unit->hu_DevControllers[0] = hc;
                    } else {
                        pciusbXHCIDebug("xHCI",
                                        DEBUGCOLOR_SET "Failed to create a device context" DEBUGCOLOR_RESET" \n");
                    }
                } else if(!connected && devCtx) {
                    pciusbXHCIDebug("xHCI",
                                    DEBUGCOLOR_SET "Detaching HCI Device Ctx @ 0x%p" DEBUGCOLOR_RESET" \n",
                                    devCtx);

                    xhciDisconnectDevice(hc, devCtx, xhcic->xhc_PortTask.xpt_TimerReq);
                }
            }
            uhwCheckRootHubChanges(hc->hc_Unit);
        }
    }

task_cleanup:
    if(xhcic->xhc_PortTask.xpt_PortChangeSignal != -1) {
        FreeSignal(xhcic->xhc_PortTask.xpt_PortChangeSignal);
        xhcic->xhc_PortTask.xpt_PortChangeSignal = -1;
    }
    xhciCloseTaskTimer(&xhcic->xhc_PortTask.xpt_TimerPort, &xhcic->xhc_PortTask.xpt_TimerReq);
    AROS_USERFUNC_EXIT
}
