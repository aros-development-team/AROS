/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved

    Desc: xHCI chipset driver isochronous transfer support functions
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/poseidon.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "xhciproto.h"
#include "xhci_schedule.h"

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

static struct PTDNode *xhciNextIsoPTD(struct RTIsoNode *rtn)
{
    if(!rtn || !rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return NULL;

    for(UWORD offset = 0; offset < rtn->rtn_PTDCount; offset++) {
        UWORD idx = (rtn->rtn_NextPTD + offset) % rtn->rtn_PTDCount;
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];

        if(!ptd)
            continue;

        if(!(ptd->ptd_Flags & (PTDF_ACTIVE | PTDF_BUFFER_VALID))) {
            rtn->rtn_NextPTD = (idx + 1) % rtn->rtn_PTDCount;
            return ptd;
        }
    }

    return NULL;
}

void xhciScheduleIsoTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq, *ionext;
    UWORD devadrep;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Scheduling new ISO transfers ..." DEBUGCOLOR_RESET" \n");
    ForeachNodeSafe(&hc->hc_IsoXFerQueue, ioreq, ionext) {
        devadrep = xhciDevEPKey(ioreq);

        if(unit->hu_DevBusyReq[devadrep] &&
                unit->hu_DevBusyReq[devadrep]->iouh_Req.io_Command != UHCMD_ISOXFER) {
            pciusbWarn("xHCI", "DevEP %02lx in use!\n", devadrep);
            continue;
        }

        volatile struct pcisusbXHCIRing *epring = NULL;
        struct pciusbXHCIIODevPrivate *driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1;
        ULONG trbflags = 0;
        WORD queued = -1;

        if(!driprivate || !driprivate->dpDevice) {
            pciusbError("xHCI",
                        DEBUGWARNCOLOR_SET "xHCI: Missing prepared ISO transfer context for Dev=%u EP=%u" DEBUGCOLOR_RESET" \n",
                        ioreq->iouh_DevAddr, ioreq->iouh_Endpoint);
            if(ioreq->iouh_DriverPrivate2)
                ((struct PTDNode *)ioreq->iouh_DriverPrivate2)->ptd_Flags &= ~(PTDF_ACTIVE | PTDF_BUFFER_VALID | PTDF_QUEUED);
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            continue;
        }

        trbflags |= TRBF_FLAG_TRTYPE_ISOCH;
        if(ioreq->iouh_Dir == UHDIR_IN)
            trbflags |= TRBF_FLAG_ISP;

        BOOL explicit_frame = FALSE;
        struct PTDNode *ptd = (struct PTDNode *)ioreq->iouh_DriverPrivate2;
        if(ptd)
            explicit_frame = (ptd->ptd_BufferReq.ubr_Frame != 0);
        else
            explicit_frame = (ioreq->iouh_Frame != 0);

        if(!explicit_frame)
            trbflags |= TRBF_FLAG_SIA;
        trbflags |= TRBF_FLAG_FRAMEID(ioreq->iouh_Frame);

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        if(ptd)
            ptd->ptd_Flags &= ~PTDF_QUEUED;

        BOOL txdone = FALSE;
        driprivate->dpSTRB = (UWORD)-1;
        driprivate->dpSttTRB = (UWORD)-1;

        if(xhciActivateEndpointTransfer(hc, unit, ioreq, driprivate, devadrep, &epring)) {
            queued = xhciQueuePayloadTRBs(hc, ioreq, driprivate, epring, trbflags, TRUE);

            if(queued != -1) {
                AddTail(&hc->hc_PeriodicTDQueue, (struct Node *) ioreq);
                if(ptd) {
                    ptd->ptd_Flags |= PTDF_ACTIVE;
                    ptd->ptd_Flags &= ~PTDF_BUFFER_VALID;
                }
                txdone = TRUE;
            }
        }

        if(!txdone) {
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

    if(!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return UHIOERR_BADPARAMS;

    for(UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];

        if(ptd) {
            if(ptd->ptd_Descriptor)
                FreeMem(ptd->ptd_Descriptor, sizeof(struct pciusbXHCITRBParams));

            FreeMem(ptd, sizeof(struct PTDNode));
            rtn->rtn_PTDs[idx] = NULL;
        }

        ptd = AllocMem(sizeof(*ptd), MEMF_CLEAR);
        if(!ptd)
            goto fail;

        ptd->ptd_Descriptor = AllocMem(sizeof(struct pciusbXHCITRBParams), MEMF_CLEAR);
        if(!ptd->ptd_Descriptor) {
            FreeMem(ptd, sizeof(*ptd));
            rtn->rtn_PTDs[idx] = NULL;
            goto fail;
        }

        CopyMem(&rtn->rtn_IOReq, &ptd->ptd_IOReq, sizeof(ptd->ptd_IOReq));
        ptd->ptd_IOReq.iouh_DriverPrivate1 = NULL;
        ptd->ptd_IOReq.iouh_DriverPrivate2 = NULL;
        ptd->ptd_IOReq.iouh_Req.io_Message.mn_Node.ln_Succ = NULL;
        ptd->ptd_IOReq.iouh_Req.io_Message.mn_Node.ln_Pred = NULL;
        ptd->ptd_RTIsoNode = rtn;

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
    struct PTDNode *ptd = xhciNextIsoPTD(rtn);
    struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
    ULONG interval_uf;
    ULONG interval_frames;

    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if(!ptd)
        return UHIOERR_NAKTIMEOUT;

    struct IOUsbHWBufferReq *bufreq = &ptd->ptd_BufferReq;
    *bufreq = rtn->rtn_BufferReq;

    if(urti) {
        if(ioreq->iouh_Dir == UHDIR_IN) {
            if(urti->urti_InReqHook)
                CallHookPkt(urti->urti_InReqHook, rtn, bufreq);
        } else {
            if(urti->urti_OutReqHook)
                CallHookPkt(urti->urti_OutReqHook, rtn, bufreq);
        }
    } else {
        bufreq->ubr_Buffer = ioreq->iouh_Data;
        bufreq->ubr_Length = ioreq->iouh_Length;
        bufreq->ubr_Frame = ioreq->iouh_Frame;
        bufreq->ubr_Flags = 0;
    }

    if(!bufreq->ubr_Length)
        bufreq->ubr_Length = ioreq->iouh_Length;

    if(bufreq->ubr_Frame)
        rtn->rtn_Flags |= RTISO_FLAG_EXPLICIT_FRAME;
    else
        rtn->rtn_Flags &= ~RTISO_FLAG_EXPLICIT_FRAME;

    {
        UWORD interval = ioreq->iouh_Interval ? ioreq->iouh_Interval : 1;
        BOOL superspeed = (ioreq->iouh_Flags & UHFF_SUPERSPEED) != 0;
        BOOL highspeed = (ioreq->iouh_Flags & UHFF_HIGHSPEED) != 0;

        if(interval > 16)
            interval = 16;

        if(superspeed || highspeed) {
            interval_uf = 1UL << (interval - 1);
        } else {
            UWORD exp = (interval - 1) + 3;

            if(exp > 15)
                exp = 15;

            interval_uf = 1UL << exp;
        }

        if(interval_uf < 1)
            interval_uf = 1;
    }

    interval_frames = (interval_uf + 7) >> 3;
    if(interval_frames < 1)
        interval_frames = 1;

    if(!bufreq->ubr_Frame) {
        /*
         * xHCI needs ISO TDs queued sufficiently ahead of the service
         * interval. A single-interval lead is often too tight on VMware.
         */
        ULONG lead_uf = interval_uf * 2;
        ULONG lead_frames = (lead_uf + 7) >> 3;
        ULONG base_frame = hc->hc_FrameCounter >> 3;
        ULONG next = rtn->rtn_NextFrame ? rtn->rtn_NextFrame : (base_frame + lead_frames);
        bufreq->ubr_Frame = next;
    }
    rtn->rtn_NextFrame = bufreq->ubr_Frame + interval_frames;

    ptd->ptd_FrameIdx = bufreq->ubr_Frame;
    ptd->ptd_Flags |= PTDF_BUFFER_VALID;
    rtn->rtn_BufferReq = *bufreq;

    return RC_OK;
}

void xhciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if(!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return;

    struct PTDNode *ptd = NULL;
    for(UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
        struct PTDNode *candidate = rtn->rtn_PTDs[idx];
        if(candidate && (candidate->ptd_Flags & PTDF_BUFFER_VALID) &&
                !(candidate->ptd_Flags & PTDF_QUEUED) &&
                !(candidate->ptd_Flags & PTDF_ACTIVE)) {
            ptd = candidate;
            break;
        }
    }

    if(!ptd)
        return;

    /* Use the IOReq stored per PTD for the actual transfer. */
    struct IOUsbHWReq *ioreq = &ptd->ptd_IOReq;

    if(!ptd->ptd_BufferReq.ubr_Buffer || !ptd->ptd_BufferReq.ubr_Length)
        return;

    ioreq->iouh_Data = ptd->ptd_BufferReq.ubr_Buffer;
    ioreq->iouh_Length = ptd->ptd_BufferReq.ubr_Length;
    ioreq->iouh_Frame = ptd->ptd_BufferReq.ubr_Frame;
    ioreq->iouh_Req.io_Command = UHCMD_ISOXFER;
    ioreq->iouh_Req.io_Error = 0;
    ioreq->iouh_Actual = 0;
    ioreq->iouh_DriverPrivate2 = ptd;

    if(!ioreq->iouh_DriverPrivate1) {
        struct pciusbXHCIIODevPrivate *driprivate = NULL;
        if(!xhciInitIOTRBTransfer(hc, ioreq, &hc->hc_IsoXFerQueue, UHCMD_ISOXFER, FALSE,
                                  hc->hc_Unit->hu_TimerReq, &driprivate))
            return;
    }

    /* Queue it on the ISO path. */
    ptd->ptd_Flags |= PTDF_QUEUED;
    AddTail(&hc->hc_IsoXFerQueue, (struct Node *)&ioreq->iouh_Req.io_Message.mn_Node);
    xhciScheduleIsoTDs(hc);
}

void xhciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if(!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return;

    Disable();
    for(UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd && ptd->ptd_IOReq.iouh_DriverPrivate1)
            xhciFreePeriodicContext(hc, hc->hc_Unit, &ptd->ptd_IOReq);
    }
    Enable();

    if(rtn->rtn_BounceBuffer && rtn->rtn_BounceBuffer != rtn->rtn_BufferReq.ubr_Buffer) {
        usbReleaseBuffer(rtn->rtn_BounceBuffer, rtn->rtn_BufferReq.ubr_Buffer,
                         rtn->rtn_BufferReq.ubr_Length, rtn->rtn_IOReq.iouh_Dir);
        rtn->rtn_BounceBuffer = NULL;
    }

    for(UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd)
            ptd->ptd_Flags &= ~(PTDF_ACTIVE | PTDF_BUFFER_VALID);
    }
}

void xhciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    pciusbXHCIDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    xhciStopIsochIO(hc, rtn);

    if(!rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return;

    for(UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd) {
            if(ptd->ptd_Descriptor)
                FreeMem(ptd->ptd_Descriptor, sizeof(struct pciusbXHCITRBParams));

            FreeMem(ptd, sizeof(*ptd));
            rtn->rtn_PTDs[idx] = NULL;
        }
    }
}
