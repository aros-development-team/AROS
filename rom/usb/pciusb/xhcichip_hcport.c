/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved

    Desc: XHCI chipset driver hc port support functions
*/

#if defined(PCIUSB_ENABLEXHCI)
#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"

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

static struct pciusbXHCIDevice *xhciFindPortDevice(struct PCIController *hc, UWORD hciport)
{
    UWORD maxslot = hc->hc_NumSlots;

    if (maxslot >= USB_DEV_MAX)
        maxslot = USB_DEV_MAX - 1;

    for (UWORD slot = 1; slot <= maxslot; slot++) {
        struct pciusbXHCIDevice *devCtx = hc->hc_Devices[slot];

        if (devCtx && (devCtx->dc_RootPort == hciport))
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

    ULONG linkState = (portsc & XHCI_PR_PORTSC_PLS_MASK) >> XHCIS_PR_PORTSC_PLS;
    ULONG speedBits = portsc & XHCI_PR_PORTSC_SPEED_MASK;
    BOOL isUsb3 = (speedBits == XHCIF_PR_PORTSC_SUPERSPEED);

    struct pciusbXHCIDevice *devCtx = xhciFindPortDevice(hc, hciport);
    ULONG routeString = devCtx ? devCtx->dc_RouteString : 0;

    const char *speedDesc = "Unknown";
    if (speedBits == XHCIF_PR_PORTSC_LOWSPEED) {
        speedDesc = "USB2-LS";
    } else if (speedBits == XHCIF_PR_PORTSC_FULLSPEED) {
        speedDesc = "USB2-FS";
    } else if (speedBits == XHCIF_PR_PORTSC_HIGHSPEED) {
        speedDesc = "USB2-HS";
    } else if (speedBits == XHCIF_PR_PORTSC_SUPERSPEED) {
        speedDesc = "USB3";
    }

    pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "xHCI: Port %u %s route 0x%05lx link %lu speed %s" DEBUGCOLOR_RESET" \n",
                    hciport + 1,
                    context,
                    routeString,
                    linkState,
                    speedDesc);

    if (isUsb3) {
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Port %u %s PORTPMSC $%08lx PORTLI $%08lx" DEBUGCOLOR_RESET" \n",
                        hciport + 1,
                        context,
                        portpmsc,
                        portli);
    } else {
        pciusbXHCIDebug("xHCI",
                        DEBUGCOLOR_SET "xHCI: Port %u %s PORTPMSC $%08lx PPM $%08lx" DEBUGCOLOR_RESET" \n",
                        hciport + 1,
                        context,
                        portpmsc,
                        hlpmc);
    }
}

BOOL xhciSetFeature(struct PCIUnit *unit, struct PCIController *hc,
                    UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    volatile struct xhci_pr *xhciports =
        (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);
    BOOL  cmdgood = FALSE;
    ULONG oldval  = AROS_LE2LONG(xhciports[hciport].portsc);
    ULONG newval  = oldval;
    ULONG tmpval;
    struct pciusbXHCIDevice *devCtx;

    pciusbXHCIDebug("xHCI",
        DEBUGFUNCCOLOR_SET "xHCI: %s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)"
        DEBUGCOLOR_RESET " \n",
        __func__, unit, hc, hciport, idx, val, retval);

    xhciDumpPort(&xhciports[hciport]);
    xhciLogPortState(hc, xhciports, hciport, "before SET_FEATURE");

    switch (val) {

    case UFS_PORT_ENABLE:
        /*
         * Only the HC can truly enable the port, but from the hub model
         * perspective we trigger a reset if the port is connected and
         * not yet enabled.
         */
        if (!(oldval & XHCIF_PR_PORTSC_PED)) {
            pciusbXHCIDebug("xHCI",
                DEBUGCOLOR_SET "xHCI: >Setting Reset to enable port"
                DEBUGCOLOR_RESET " \n");
            if (oldval & XHCIF_PR_PORTSC_CCS) {
                newval |= XHCIF_PR_PORTSC_PR;
            }
        } else {
            pciusbXHCIDebug("xHCI",
                DEBUGCOLOR_SET "xHCI: Port already enabled"
                DEBUGCOLOR_RESET " \n");
        }
        cmdgood = TRUE;
        break;

    case UFS_PORT_SUSPEND:
        /*
         * Proper suspend would manipulate PLS; for now we just log and
         * pretend success.
         */
        pciusbXHCIDebug("xHCI",
            DEBUGCOLOR_SET "xHCI: Suspending Port"
            DEBUGCOLOR_RESET " \n");
        cmdgood = TRUE;
        break;

    case UFS_PORT_RESET:
        pciusbXHCIDebug("xHCI",
            DEBUGCOLOR_SET "xHCI: Performing Port Reset-:"
            DEBUGCOLOR_RESET " \n");

        /*
         * If there was a device and the port was enabled, free
         * its context before reset.
         */
        if ((oldval & XHCIF_PR_PORTSC_PED) &&
            xhciFindPortDevice(hc, hciport))
        {
            pciusbXHCIDebug("xHCI",
                DEBUGCOLOR_SET "xHCI:     >Setting Port Power Off"
                DEBUGCOLOR_RESET " \n");
            devCtx = xhciFindPortDevice(hc, hciport);

            if (devCtx) {
                pciusbXHCIDebug("xHCI",
                    DEBUGCOLOR_SET "xHCI: Disabling device slot (%u) and freeing resources.."
                    DEBUGCOLOR_RESET "\n",
                    devCtx->dc_SlotID);

                xhciFreeDeviceCtx(hc, devCtx, TRUE);
            }
        }

        newval |= XHCIF_PR_PORTSC_PR;
        pciusbXHCIDebug("xHCI",
            DEBUGCOLOR_SET "xHCI:     >Setting Reset"
            DEBUGCOLOR_RESET " \n");

        /* Enable enumeration again on this port. */
        unit->hu_DevControllers[0] = hc;

        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER:
        /*
         * If controller does not support per-port power control (PPC=0),
         * ports are always powered. Treat this as a NOP and succeed.
         */
        if (!(hc->hc_Flags & HCF_PPC)) {
            pciusbXHCIDebug("xHCI",
                DEBUGCOLOR_SET "xHCI: PORT_POWER set (PPC=0, NOP)"
                DEBUGCOLOR_RESET " \n");
            cmdgood = TRUE;
            break;
        }

        if (!(oldval & XHCIF_PR_PORTSC_PP)) {
            pciusbXHCIDebug("xHCI",
                DEBUGCOLOR_SET "xHCI: Powering Port"
                DEBUGCOLOR_RESET " \n");
            newval |= XHCIF_PR_PORTSC_PP;
        } else {
            pciusbXHCIDebug("xHCI",
                DEBUGCOLOR_SET "xHCI: Port power already on"
                DEBUGCOLOR_RESET " \n");
        }
        cmdgood = TRUE;
        break;
    }

    if (cmdgood) {
        xhciports[hciport].portsc = AROS_LONG2LE(newval);
        tmpval = AROS_LE2LONG(xhciports[hciport].portsc);

        pciusbXHCIDebug("xHCI",
            DEBUGCOLOR_SET "xHCI: Port %ld SET_FEATURE $%08lx->$%08lx"
            DEBUGCOLOR_RESET " \n",
            idx, oldval, tmpval);

        xhciLogPortState(hc, xhciports, hciport, "after SET_FEATURE");
    }

    return cmdgood;
}

BOOL xhciClearFeature(struct PCIUnit *unit, struct PCIController *hc,
                      UWORD hciport, UWORD idx, UWORD val, WORD *retval)
{
    volatile struct xhci_pr *xhciports =
        (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);
    ULONG oldval    = AROS_LE2LONG(xhciports[hciport].portsc);
    ULONG newval    = oldval;
    ULONG clearbits = 0;
    BOOL  cmdgood   = FALSE;
    struct pciusbXHCIDevice *devCtx;

    pciusbXHCIDebug("xHCI",
        DEBUGFUNCCOLOR_SET "xHCI: %s(0x%p, 0x%p, %04x, %04x, %04x, 0x%p)"
        DEBUGCOLOR_RESET " \n",
        __func__, unit, hc, hciport, idx, val, retval);

    xhciDumpPort(&xhciports[hciport]);
    xhciLogPortState(hc, xhciports, hciport, "before CLEAR_FEATURE");

    switch (val) {

    case UFS_PORT_ENABLE:
        /*
         * ClearFeature(PORT_ENABLE) disables the port.
         * For xHCI, clearing PED via PORTSC is sufficient.
         */
        pciusbXHCIDebug("xHCI",
            DEBUGCOLOR_SET "xHCI: Disabling Port"
            DEBUGCOLOR_RESET " \n");

        newval &= ~XHCIF_PR_PORTSC_PED;
        cmdgood = TRUE;

        /* Stop enumeration on this port and free any context. */
        unit->hu_DevControllers[0] = NULL;

        devCtx = xhciFindPortDevice(hc, hciport);
        if (devCtx) {
            xhciFreeDeviceCtx(hc, devCtx, TRUE);
        }
        break;

    case UFS_PORT_SUSPEND:
        /* Currently treated as a no-op beyond logging. */
        pciusbXHCIDebug("xHCI",
            DEBUGCOLOR_SET "xHCI: Suspending Port"
            DEBUGCOLOR_RESET " \n");
        cmdgood = TRUE;
        break;

    case UFS_PORT_POWER:
        /*
         * If no PPC, ports are "always powered" and PORT_POWER clear
         * is a NOP.
         */
        if (!(hc->hc_Flags & HCF_PPC)) {
            pciusbXHCIDebug("xHCI",
                DEBUGCOLOR_SET "xHCI: PORT_POWER clear (PPC=0, NOP)"
                DEBUGCOLOR_RESET " \n");
            cmdgood = TRUE;
            break;
        }

        if (oldval & XHCIF_PR_PORTSC_PP) {
            pciusbXHCIDebug("xHCI",
                DEBUGCOLOR_SET "xHCI: Powering Down Port"
                DEBUGCOLOR_RESET " \n");
            newval &= ~XHCIF_PR_PORTSC_PP;
        } else {
            pciusbXHCIDebug("xHCI",
                DEBUGCOLOR_SET "xHCI: Port power already off"
                DEBUGCOLOR_RESET " \n");
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
        clearbits |= (XHCIF_PR_PORTSC_PRC | XHCIF_PR_PORTSC_WRC);
        cmdgood = TRUE;
        break;
    }

    if (cmdgood) {
        /*
         * For xHCI RW1C bits, writing 1 clears them. We OR clearbits
         * into the current value to clear only the requested change bits.
         */
        newval |= clearbits;
        xhciports[hciport].portsc = AROS_LONG2LE(newval);
        newval = AROS_LE2LONG(xhciports[hciport].portsc);

        if (hc->hc_PortChangeMap[hciport]) {
            hc->hc_Unit->hu_RootPortChanges |= (1UL << (hciport + 1));
        } else {
            hc->hc_Unit->hu_RootPortChanges &= ~(1UL << (hciport + 1));
        }

        pciusbXHCIDebug("xHCI",
            DEBUGCOLOR_SET "xHCI: Port %ld CLEAR_FEATURE $%08lx->$%08lx"
            DEBUGCOLOR_RESET " \n",
            idx, oldval, newval);

        xhciLogPortState(hc, xhciports, hciport, "after CLEAR_FEATURE");
    }

    return cmdgood;
}

BOOL xhciGetStatus(struct PCIController *hc, UWORD *mptr,
                   UWORD hciport, UWORD idx, WORD *retval)
{
    volatile struct xhci_pr *xhciports =
        (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);
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
    if (!(hc->hc_Flags & HCF_PPC)) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
    } else if (oldportsc & XHCIF_PR_PORTSC_PP) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_POWER);
    }

    if (oldportsc & XHCIF_PR_PORTSC_OCA) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_OVER_CURRENT);
    }
    if (oldportsc & XHCIF_PR_PORTSC_CCS) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
    }
    if (oldportsc & XHCIF_PR_PORTSC_PED) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_ENABLE);
    }

    /* Speed mapping */
    if ((oldportsc &
         (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) ==
        XHCIF_PR_PORTSC_LOWSPEED)
    {
        *mptr |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
    } else if ((oldportsc &
                (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) ==
               XHCIF_PR_PORTSC_HIGHSPEED)
    {
        *mptr |= AROS_WORD2LE(UPSF_PORT_HIGH_SPEED);
    } else if ((oldportsc &
                (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)) ==
               XHCIF_PR_PORTSC_SUPERSPEED)
    {
        *mptr |= AROS_WORD2LE(UPSF_PORT_SUPER_SPEED);
    } else {
        /* Fullspeed: default, no extra flag needed. */
    }

    if (oldportsc & XHCIF_PR_PORTSC_PR) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_RESET);
    }

    /* PLS == 2 (U1) is used here as SUSPEND indication. */
    if (((oldportsc >> XHCIS_PR_PORTSC_PLS) & XHCI_PR_PORTSC_PLS_SMASK) == 2) {
        *mptr |= AROS_WORD2LE(UPSF_PORT_SUSPEND);
    }

    /* Port indicator control: bits 14..15 */
    if ((oldportsc >> 14) & 0x3) {
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

    if (oldportsc & XHCIF_PR_PORTSC_PEC) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
    }
    if (oldportsc & XHCIF_PR_PORTSC_CSC) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
    }
    if (oldportsc & XHCIF_PR_PORTSC_PLC) {
        hc->hc_PortChangeMap[hciport] |= (UPSF_PORT_SUSPEND | UPSF_PORT_ENABLE);
    }
    if (oldportsc & XHCIF_PR_PORTSC_OCC) {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
    }
    if ((oldportsc & XHCIF_PR_PORTSC_WRC) ||
        (oldportsc & XHCIF_PR_PORTSC_PRC))
    {
        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
    }

    *mptr = AROS_WORD2LE(hc->hc_PortChangeMap[hciport]);

    pciusbXHCIDebug("xHCI",
        DEBUGCOLOR_SET "xHCI: Port %ld Change $%08lx"
        DEBUGCOLOR_RESET " \n",
        idx, *mptr);

    /* Preserve legacy behavior: return FALSE. */
    return FALSE;
}
#endif /* PCIUSB_ENABLEXHCI */
