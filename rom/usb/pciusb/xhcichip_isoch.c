/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver isochronous transfer support functions
*/

#if defined(PCIUSB_ENABLEXHCI)
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"
#include "xhcichip_schedule.h"

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

void xhciScheduleIsoTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq, *ionext;
    UWORD devadrep;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Scheduling new ISO transfers ..." DEBUGCOLOR_RESET" \n");
    ForeachNodeSafe(&hc->hc_IsoXFerQueue, ioreq, ionext) {
        devadrep = xhciDevEPKey(ioreq);

        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("xHCI", "DevEP %02lx in use!\n", devadrep);
            continue;
        }

        volatile struct pcisusbXHCIRing *epring = NULL;
        struct pciusbXHCIIODevPrivate *driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;
        ULONG trbflags = 0;
        WORD queued = -1;

        if (!driprivate || !driprivate->dpDevice) {
            pciusbError("xHCI",
                        DEBUGWARNCOLOR_SET "xHCI: Missing prepared ISO transfer context for Dev=%u EP=%u" DEBUGCOLOR_RESET" \n",
                        ioreq->iouh_DevAddr, ioreq->iouh_Endpoint);
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            continue;
        }

        trbflags |= TRBF_FLAG_TRTYPE_ISOCH;
        trbflags |= TRBF_FLAG_SIA;
        if (ioreq->iouh_Dir == UHDIR_IN)
            trbflags |= TRBF_FLAG_ISP;

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);

        BOOL txdone = FALSE;
        driprivate->dpSTRB = (UWORD)-1;
        driprivate->dpSttTRB = (UWORD)-1;

        if (xhciActivateEndpointTransfer(hc, unit, ioreq, driprivate, devadrep, &epring)) {
            queued = xhciQueuePayloadTRBs(hc, ioreq, driprivate, epring, trbflags, TRUE);

            if (queued != -1) {
                AddTail(&hc->hc_PeriodicTDQueue, (struct Node *) ioreq);
                txdone = TRUE;
            }
        }

        if (!txdone) {
            driprivate->dpSTRB = (UWORD)-1;
            driprivate->dpTxSTRB = (UWORD)-1;
            driprivate->dpTxETRB = (UWORD)-1;
            driprivate->dpSttTRB = (UWORD)-1;

            Disable();
            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Failed to submit ISO transaction" DEBUGCOLOR_RESET" \n");
            AddHead(&hc->hc_IsoXFerQueue, (struct Node *) ioreq);
            Enable();
        } else {
            xhciRingDoorbell(hc, driprivate->dpDevice->dc_SlotID, driprivate->dpEPID);
        }
    }
}

WORD xhciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if (!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return UHIOERR_BADPARAMS;

    for (UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];

        if (ptd) {
            if (ptd->ptd_Descriptor)
                FreeMem(ptd->ptd_Descriptor, sizeof(struct pciusbXHCITRBParams));

            FreeMem(ptd, sizeof(struct PTDNode));
            rtn->rtn_PTDs[idx] = NULL;
        }

        ptd = AllocMem(sizeof(*ptd), MEMF_CLEAR);
        if (!ptd)
            goto fail;

        ptd->ptd_Descriptor = AllocMem(sizeof(struct pciusbXHCITRBParams), MEMF_CLEAR);
        if (!ptd->ptd_Descriptor) {
            FreeMem(ptd, sizeof(*ptd));
            rtn->rtn_PTDs[idx] = NULL;
            goto fail;
        }

        rtn->rtn_PTDs[idx] = ptd;
    }

    rtn->rtn_NextPTD = 0;
    rtn->rtn_BounceBuffer = NULL;

    return RC_OK;

fail:
    xhciFreeIsochIO(hc, rtn);
    return UHIOERR_OUTOFMEMORY;
}

WORD xhciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
    struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
    ULONG interval;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if (urti) {
        if (ioreq->iouh_Dir == UHDIR_IN) {
            if (urti->urti_InReqHook)
                CallHookPkt(urti->urti_InReqHook, rtn, &rtn->rtn_BufferReq);
        } else {
            if (urti->urti_OutReqHook)
                CallHookPkt(urti->urti_OutReqHook, rtn, &rtn->rtn_BufferReq);
        }
    } else {
        rtn->rtn_BufferReq.ubr_Buffer = ioreq->iouh_Data;
        rtn->rtn_BufferReq.ubr_Length = ioreq->iouh_Length;
        rtn->rtn_BufferReq.ubr_Frame = ioreq->iouh_Frame;
        rtn->rtn_BufferReq.ubr_Flags = 0;
    }

    if (!rtn->rtn_BufferReq.ubr_Length)
        rtn->rtn_BufferReq.ubr_Length = ioreq->iouh_Length;

    interval = ioreq->iouh_Interval ? ioreq->iouh_Interval : 1;
    if (!rtn->rtn_BufferReq.ubr_Frame) {
        ULONG next = rtn->rtn_NextFrame ? rtn->rtn_NextFrame : (hc->hc_FrameCounter + interval);
        rtn->rtn_BufferReq.ubr_Frame = next;
    }
    rtn->rtn_NextFrame = rtn->rtn_BufferReq.ubr_Frame + interval;

    rtn->rtn_NextPTD = 0;

    return RC_OK;
}

void xhciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if (!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return;

    /* Use the IOReq stored in the RTIso node for the actual transfer. */
    struct IOUsbHWReq *ioreq = &rtn->rtn_IOReq;

    if (!rtn->rtn_BufferReq.ubr_Buffer || !rtn->rtn_BufferReq.ubr_Length)
        return;

    ioreq->iouh_Data = rtn->rtn_BufferReq.ubr_Buffer;
    ioreq->iouh_Length = rtn->rtn_BufferReq.ubr_Length;
    ioreq->iouh_Frame = rtn->rtn_BufferReq.ubr_Frame;
    ioreq->iouh_Req.io_Command = UHCMD_ISOXFER;
    ioreq->iouh_Req.io_Error = 0;
    ioreq->iouh_DriverPrivate2 = rtn;

    if (!ioreq->iouh_DriverPrivate1) {
        struct pciusbXHCIIODevPrivate *driprivate = NULL;
        if (!xhciInitIOTRBTransfer(hc, ioreq, &hc->hc_IsoXFerQueue, UHCMD_ISOXFER, FALSE,
                                   hc->hc_Unit->hu_TimerReq, &driprivate))
            return;
    }

    /* Queue it on the ISO path. */
    AddTail(&hc->hc_IsoXFerQueue, (struct Node *)&ioreq->iouh_Req.io_Message.mn_Node);
    xhciScheduleIsoTDs(hc);
}

void xhciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if (!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return;

    Disable();
    xhciFreePeriodicContext(hc, hc->hc_Unit, &rtn->rtn_IOReq);
    Enable();

    if (rtn->rtn_BounceBuffer && rtn->rtn_BounceBuffer != rtn->rtn_BufferReq.ubr_Buffer) {
        usbReleaseBuffer(rtn->rtn_BounceBuffer, rtn->rtn_BufferReq.ubr_Buffer,
                         rtn->rtn_BufferReq.ubr_Length, rtn->rtn_IOReq.iouh_Dir);
        rtn->rtn_BounceBuffer = NULL;
    }
}

void xhciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    xhciStopIsochIO(hc, rtn);

    if (!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return;

    for (UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if (ptd) {
            if (ptd->ptd_Descriptor)
                FreeMem(ptd->ptd_Descriptor, sizeof(struct pciusbXHCITRBParams));

            FreeMem(ptd, sizeof(*ptd));
            rtn->rtn_PTDs[idx] = NULL;
        }
    }
}

#endif /* PCIUSB_ENABLEXHCI */
