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

    return 0;
}

#endif
