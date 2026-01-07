/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver transfer preparation and special case handling
*/

#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"
#include "xhci_schedule.h"

#if defined(AROS_USE_LOGRES)
#ifdef LogHandle
#undef LogHandle
#endif
#ifdef LogResBase
#undef LogResBase
#endif
#define LogHandle (base->hd_LogRHandle)
#define LogResBase (base->hd_LogResBase)
#endif


#if defined(DEBUG) && defined(XHCI_LONGDEBUGNAK)
#define XHCI_NAKTOSHIFT         (8)
#else
#define XHCI_NAKTOSHIFT         (3)
#endif

static inline const char *xhciSlotStateName(UBYTE state)
{
    switch(state) {
    case XHCI_SLOT_STATE_DISABLED_OR_ENABLED:
        return "Disabled/Enabled";
    case XHCI_SLOT_STATE_DEFAULT:
        return "Default";
    case XHCI_SLOT_STATE_ADDRESSED:
        return "Addressed";
    case XHCI_SLOT_STATE_CONFIGURED:
        return "Configured";
    default:
        return "Unknown";
    }
}

static inline UBYTE xhciSlotStateFromDW3(ULONG dw3)
{
    return (UBYTE)((dw3 >> XHCI_SLOTCTX3_SLOTSTATE_SHIFT) & 0x1FUL);
}

WORD xhciPrepareTransfer(struct IOUsbHWReq *ioreq,
                         struct PCIUnit *unit,
                         struct PCIDevice *base)
{
    struct PCIController *hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    struct List *ownerList = NULL;
    struct pciusbXHCIIODevPrivate *driprivate = NULL;
    ULONG txtype = ioreq->iouh_Req.io_Command;
    BOOL allowEp0AutoCreate = FALSE;
    const UBYTE bmRequestType = ioreq->iouh_SetupData.bmRequestType;
    const UBYTE bRequest      = ioreq->iouh_SetupData.bRequest;
    const UWORD wValue        = AROS_LE2WORD(ioreq->iouh_SetupData.wValue);
    const UWORD wIndex        = AROS_LE2WORD(ioreq->iouh_SetupData.wIndex);

    xhciDebugControlTransfer(ioreq);

    if(!hc) {
        pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "xHCI: hc = NULL" DEBUGCOLOR_RESET"\n");
        return UHIOERR_HOSTERROR;
    }

    switch(txtype) {
    case UHCMD_CONTROLXFER:
        ownerList = &hc->hc_CtrlXFerQueue;
        if(!xhciIsRootHubRequest(ioreq, unit))
            allowEp0AutoCreate = TRUE;
        break;
    case UHCMD_BULKXFER:
        ownerList = &hc->hc_BulkXFerQueue;
        if(!xhciIsRootHubRequest(ioreq, unit))
            allowEp0AutoCreate = TRUE;
        break;
    case UHCMD_INTXFER:
        ownerList = &hc->hc_IntXFerQueue;
        break;
    case UHCMD_ISOXFER:
        ownerList = &hc->hc_IsoXFerQueue;
        break;
    default:
        break;
    }

    /* Prepare TRB transaction bookkeeping before the request is scheduled.
     * This ensures scheduling (and the event-ring task) only deals with already
     * prepared transactions and never has to allocate/initialise driver-private
     * state.
     *
     * For downstream devices (non-roothub requests), EP0 may need to be
     * configured here because the port-change task only handles root ports.
     */
    if(!xhciInitIOTRBTransfer(hc, ioreq, ownerList, txtype, allowEp0AutoCreate,
                              unit->hu_TimerReq, &driprivate)) {
        pciusbError("xHCI",
                    DEBUGWARNCOLOR_SET "xHCI: xhciInitIOTRBTransfer failed" DEBUGCOLOR_RESET"\n");
        if(ioreq->iouh_Req.io_Error)
            return ioreq->iouh_Req.io_Error;
        return UHIOERR_HOSTERROR;
    }

    /*
     * xHCI special-case control requests
     *
     * MUST be handled after xhciInitIOTRBTransfer() so that:
     *  - EP0 can be auto-created (DevAddr0)
     *  - ioreq->iouh_DriverPrivate1 is populated
     *
     * MUST be handled here (not in the scheduler), so uhwcmd.c can stay generic.
     */

    if(txtype == UHCMD_CONTROLXFER && driprivate && driprivate->dpDevice) {
        /* Standard SET_ADDRESS (device request) */
        if((bmRequestType == (URTF_STANDARD | URTF_DEVICE)) &&
                (bRequest == USR_SET_ADDRESS)) {
            const UWORD newaddr = wValue;
            struct pciusbXHCIDevice *devCtx = driprivate->dpDevice;
            volatile struct xhci_slot *oslot = (volatile struct xhci_slot *)devCtx->dc_SlotCtx.dmaa_Ptr;
            volatile struct xhci_inctx *in = (volatile struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;
            volatile struct xhci_slot *islot;
            volatile struct xhci_ep *oep0;
            volatile struct xhci_ep *iep0;
            UWORD ctxoff = 1;
            const UWORD ctxsize = (hc->hc_Flags & HCF_CTX64) ? 64 : 32;
            LONG cc = TRB_CC_INVALID;
            ULONG odw3;
            ULONG s3;
            ULONG e0;
            UBYTE state;

            if(hc->hc_Flags & HCF_CTX64)
                ctxoff <<= 1;

            pciusbXHCIDebugV("xHCI",
                             "SET_ADDRESS short-circuit (PrepareTransfer): slot=%u addr %u->%u ioreq=%p\n",
                             devCtx->dc_SlotID,
                             (unsigned)devCtx->dc_DevAddr,
                             (unsigned)newaddr,
                             ioreq);

            if(!oslot || !in) {
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                driprivate->dpCC = TRB_CC_INVALID;
                goto _xhci_setaddr_complete;
            }

            /* Ensure we see the controller's latest output context state (Slot + EP0). */
            CacheClearE((APTR)oslot, ctxsize * 2, CACRF_InvalidateD);

            odw3 = AROS_LE2LONG(oslot->ctx[3]);
            state = (odw3 & XHCI_SLOTCTX3_SLOTSTATE_MASK) >> XHCI_SLOTCTX3_SLOTSTATE_SHIFT;

            pciusbXHCIDebugV("xHCI",
                             "SET_ADDRESS: current slot state = %u (%s)\n",
                             (unsigned)state,
                             xhciSlotStateName(state));

            /* Prepare input pointers */
            islot = xhciInputSlotCtx((struct xhci_inctx *)in, ctxoff);
            iep0  = xhciInputEndpointCtx((struct xhci_inctx *)in, ctxoff, 1);
            oep0  = (volatile struct xhci_ep *)((UBYTE *)oslot + ctxsize);

#define BUILD_INPUT_CTX(_devaddr_)                                           \
            do {                                                             \
                /* Only Slot + EP0, no drops */                              \
                in->dcf = 0;                                                 \
                in->acf = (1UL << 0) | (1UL << 1);                           \
                                                                             \
                /* Slot: copy output -> input, scrub state, set devaddr */   \
                CopyMem((const void *)oslot, (void *)islot, ctxsize);        \
                s3 = AROS_LE2LONG(islot->ctx[3]);                            \
                s3 &= ~XHCI_SLOTCTX3_SLOTSTATE_MASK;                        \
                s3 &= ~XHCI_SLOTCTX3_DEVADDR_MASK;                          \
                s3 |= ((ULONG)((_devaddr_) & 0xFFU) << XHCI_SLOTCTX3_DEVADDR_SHIFT); \
                islot->ctx[3] = AROS_LONG2LE(s3);                            \
                                                                             \
                /* EP0: copy output -> input, force EPSTATE=Disabled */      \
                CacheClearE((APTR)oep0, ctxsize, CACRF_InvalidateD);         \
                CopyMem((const void *)oep0, (void *)iep0, ctxsize);          \
                e0 = AROS_LE2LONG(iep0->ctx[0]);                             \
                e0 &= ~0x7UL;                                               \
                iep0->ctx[0] = AROS_LONG2LE(e0);                             \
                                                                             \
                /* Flush updated input context */                            \
                CacheClearE((APTR)in, devCtx->dc_IN.dmaa_Entry.me_Length, CACRF_ClearD); \
            } while (0)

            /*
             * xHCI never forwards SET_ADDRESS on EP0; translate it into an Address Device
             * Command (BSR=0). Do not attempt a redundant "Enabled->Default" transition:
             * Slot State value 1 is Default (not Enabled), and issuing BSR=1 from Default
             * causes Context State Error (CC=19).
             */
            if(state == XHCI_SLOT_STATE_ADDRESSED || state == XHCI_SLOT_STATE_CONFIGURED) {
                /* Already addressed/configured; do not re-address. */
                cc = TRB_CC_SUCCESS;
            } else {
                BUILD_INPUT_CTX(newaddr);
                cc = xhciCmdDeviceAddress(hc, devCtx->dc_SlotID, devCtx->dc_IN.dmaa_Ptr, 0, NULL,
                                          unit->hu_TimerReq);
                if(cc == TRB_CC_SUCCESS) {
                    /* Verify what address the controller actually selected. */
                    CacheClearE((APTR)oslot, ctxsize, CACRF_InvalidateD);
                    odw3 = AROS_LE2LONG(oslot->ctx[3]);
                    const UBYTE hcaddr = (UBYTE)(odw3 & 0xFFu);
                    if(hcaddr != (UBYTE)newaddr) {
                        pciusbXHCIDebugV("xHCI",
                                         "SET_ADDRESS: xHC selected address %u (stack requested %u); enumeration may fail if the stack does not adapt\n",
                                         (unsigned)hcaddr, (unsigned)(UBYTE)newaddr);
                    }
                }
            }

#undef BUILD_INPUT_CTX

            if((driprivate->dpCC = (UBYTE)cc) == TRB_CC_SUCCESS) {
                devCtx->dc_DevAddr = (UBYTE)newaddr;
                unit->hu_DevControllers[newaddr] = hc;
            } else {
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            }

_xhci_setaddr_complete:
            Disable();
            AddTail(&hc->hc_TDQueue, &ioreq->iouh_Req.io_Message.mn_Node);
            Enable();

            SureCause(base, &hc->hc_CompleteInt);
            return RC_DONTREPLY;
        }

    }

    /*
     * Hub port connection change handling (downstream disconnects).
     *
     * When a hub clears C_PORT_CONNECTION for a downstream port, the device
     * on that port may have disappeared. Build the child route string and
     * drop any matching xHCI device context to release slot/resources.
     */
    if((txtype == UHCMD_CONTROLXFER) &&
            (ioreq->iouh_Flags & UHFF_HUB)) {
        if((bmRequestType == (URTF_CLASS | URTF_OTHER)) &&
                (bRequest == USR_CLEAR_FEATURE) &&
                (wValue == UFS_C_PORT_CONNECTION)) {
            UWORD port = wIndex & 0xFF;

            if(port > 0 && port <= 0x0F) {
                ULONG route = ioreq->iouh_RouteString & SLOT_CTX_ROUTE_MASK;
                int depth;

                for(depth = 0; depth < 5; depth++) {
                    if(((route >> (depth * 4)) & 0xF) == 0)
                        break;
                }

                if(depth < 5) {
                    route |= ((ULONG)port << (depth * 4));

                    UWORD rootPortIndex = (ioreq->iouh_RootPort > 0)
                                          ? (ioreq->iouh_RootPort - 1)
                                          : 0;
                    struct pciusbXHCIDevice *child =
                        xhciFindRouteDevice(hc, route, rootPortIndex);
                    if(child) {
                        pciusbXHCIDebug("xHCI",
                                        "Hub port %u cleared C_PORT_CONNECTION, disconnecting route 0x%05lx\n",
                                        (unsigned)port, route);
                        xhciDisconnectDevice(hc, child, unit->hu_TimerReq);
                    }
                }
            }
        }
    }

    return 0;
}
