/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved

    Desc: XHCI chipset driver main pciusb interface
*/

#if defined(PCIUSB_ENABLEXHCI)
#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"
#include "xhcichip_schedule.h"

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

WORD xhciPrepareTransfer(struct IOUsbHWReq *ioreq,
                         struct PCIUnit *unit,
                         struct PCIDevice *base)
{
    struct PCIController *hc = unit->hu_DevControllers[ioreq->iouh_DevAddr];
    struct List *ownerList = NULL;
    struct pciusbXHCIIODevPrivate *driprivate = NULL;
    ULONG txtype = ioreq->iouh_Req.io_Command;
    BOOL allowEp0AutoCreate = FALSE;

    xhciDebugControlTransfer(ioreq);

    if (!hc)
        return UHIOERR_HOSTERROR;

    switch (txtype) {
    case UHCMD_CONTROLXFER:
        ownerList = &hc->hc_CtrlXFerQueue;
        if (!xhciIsRootHubRequest(ioreq, unit))
            allowEp0AutoCreate = TRUE;
        break;
    case UHCMD_BULKXFER:
        ownerList = &hc->hc_BulkXFerQueue;
        if (!xhciIsRootHubRequest(ioreq, unit))
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
    if (!xhciInitIOTRBTransfer(hc, ioreq, ownerList, txtype, allowEp0AutoCreate, &driprivate)) {
        if (ioreq->iouh_Req.io_Error)
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

    if (txtype == UHCMD_CONTROLXFER && driprivate && driprivate->dpDevice) {
        const UBYTE bmRequestType = ioreq->iouh_SetupData.bmRequestType;
        const UBYTE bRequest      = ioreq->iouh_SetupData.bRequest;
        const UWORD wValue        = AROS_LE2WORD(ioreq->iouh_SetupData.wValue);
        const UWORD wIndex        = AROS_LE2WORD(ioreq->iouh_SetupData.wIndex);

        /* Standard SET_ADDRESS (device request) */
        if ((bmRequestType == (URTF_STANDARD | URTF_DEVICE)) &&
            (bRequest == USR_SET_ADDRESS))
        {
            const UWORD newaddr = wValue;

            pciusbXHCIDebug("xHCI",
                "SET_ADDRESS short-circuit (PrepareTransfer): slot=%u addr %u->%u ioreq=%p\n",
                driprivate->dpDevice->dc_SlotID,
                (unsigned)driprivate->dpDevice->dc_DevAddr,
                (unsigned)newaddr,
                ioreq);

            /*
             * Do NOT send SET_ADDRESS on the wire for xHCI.
             * The xHC Address Device command is performed elsewhere during bring-up.
             */
            driprivate->dpDevice->dc_DevAddr = (UBYTE)newaddr;
            unit->hu_DevControllers[newaddr] = hc;

            /* Queue a successful completion via the standard completion path. */
            Disable();
            driprivate->dpCC = TRB_CC_SUCCESS;
            AddTail(&hc->hc_TDQueue, &ioreq->iouh_Req.io_Message.mn_Node);
            Enable();

            SureCause(base, &hc->hc_CompleteInt);
            return RC_DONTREPLY; /* tell caller: already queued for completion */
        }

        /* Standard CLEAR_FEATURE(ENDPOINT_HALT) (endpoint request) */
        if ((bmRequestType == (URTF_STANDARD | URTF_ENDPOINT)) &&
            (bRequest == USR_CLEAR_FEATURE) &&
            (wValue == UFS_ENDPOINT_HALT))
        {
            struct pciusbXHCIDevice *devCtx = driprivate->dpDevice;
            const UWORD epid = xhciEndpointIDFromIndex(wIndex);
            LONG cc = TRB_CC_INVALID;

            pciusbXHCIDebug("xHCI",
                "CLEAR_FEATURE(ENDPOINT_HALT) short-circuit (PrepareTransfer): slot=%u wIndex=%u -> EPID=%u ioreq=%p\n",
                devCtx->dc_SlotID, (unsigned)wIndex, (unsigned)epid, ioreq);

            if (epid == 0) {
                /* EP0 halt clear is not expected; treat as success. */
                cc = TRB_CC_SUCCESS;
            } else {
                cc = xhciCmdEndpointStop(hc, devCtx->dc_SlotID, epid, FALSE);
                if (cc == TRB_CC_SUCCESS)
                    cc = xhciCmdEndpointReset(hc, devCtx->dc_SlotID, epid, 0);
            }

            if (cc == TRB_CC_SUCCESS) {
                driprivate->dpCC = TRB_CC_SUCCESS;
            } else {
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                driprivate->dpCC = (UBYTE)cc;
            }

            Disable();
            AddTail(&hc->hc_TDQueue, &ioreq->iouh_Req.io_Message.mn_Node);
            Enable();

            SureCause(base, &hc->hc_CompleteInt);
            return RC_DONTREPLY;
        }
    }

    return 0;
}

#endif
