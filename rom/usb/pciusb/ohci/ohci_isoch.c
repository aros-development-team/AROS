/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved

    Desc: OHCI chipset driver isochronous/RTIso support functions
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/pci.h>
#include <utility/hooks.h>
#include <exec/memory.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "ohciproto.h"

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

void ohciScheduleIsoTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;

    pciusbOHCIDebug("OHCI", "Scheduling new ISO transfers...\n");
    ioreq = (struct IOUsbHWReq *)hc->hc_IsoXFerQueue.lh_Head;
    while (((struct Node *)ioreq)->ln_Succ) {
        devadrep = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint +
                   ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        if (unit->hu_DevBusyReq[devadrep]) {
            ioreq = (struct IOUsbHWReq *)((struct Node *)ioreq)->ln_Succ;
            continue;
        }

        struct RTIsoNode *rtn = pciusbAllocStdIsoNode(hc, ioreq);
        if (!rtn)
            break;

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);

        if (ohciInitIsochIO(hc, rtn) != RC_OK) {
            pciusbFreeStdIsoNode(hc, rtn);
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            ioreq = (struct IOUsbHWReq *)hc->hc_IsoXFerQueue.lh_Head;
            continue;
        }

        ohciQueueIsochIO(hc, rtn);
        AddTail((struct List *)&hc->hc_RTIsoHandlers, (struct Node *)&rtn->rtn_Node);
        unit->hu_DevBusyReq[devadrep] = ioreq;
        ohciStartIsochIO(hc, rtn);

        ioreq = (struct IOUsbHWReq *)hc->hc_IsoXFerQueue.lh_Head;
    }
}

WORD ohciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct PTDNode *ptd0 = NULL;
    struct PTDNode *ptd1 = NULL;
    struct OhciIsoTD *oitd0 = NULL;
    struct OhciIsoTD *oitd1 = NULL;
    struct OhciED *oed = NULL;
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;

    pciusbOHCIDebug("OHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || ptdcount < 2)
        return(UHIOERR_BADPARAMS);

    for(idx = 0; idx < ptdcount; idx++)
        rtn->rtn_PTDs[idx] = NULL;

    ptd0 = AllocMem(sizeof(*ptd0), MEMF_CLEAR);
    ptd1 = AllocMem(sizeof(*ptd1), MEMF_CLEAR);
    if(!ptd0 || !ptd1) {
        if(ptd0)
            FreeMem(ptd0, sizeof(*ptd0));
        if(ptd1)
            FreeMem(ptd1, sizeof(*ptd1));
        return(UHIOERR_OUTOFMEMORY);
    }

    oitd0 = ohciAllocIsoTD(hc);
    oitd1 = ohciAllocIsoTD(hc);
    oed = ohciAllocED(hc);
    if(!oitd0 || !oitd1 || !oed) {
        if(oitd0)
            ohciFreeIsoTD(hc, oitd0);
        if(oitd1)
            ohciFreeIsoTD(hc, oitd1);
        if(oed)
            ohciFreeED(hc, oed);
        FreeMem(ptd0, sizeof(*ptd0));
        FreeMem(ptd1, sizeof(*ptd1));
        return(UHIOERR_OUTOFMEMORY);
    }

    ptd0->ptd_Descriptor = oitd0;
    ptd0->ptd_Phys = READMEM32_LE(&oitd0->oitd_Self);
    ptd1->ptd_Descriptor = oitd1;
    ptd1->ptd_Phys = READMEM32_LE(&oitd1->oitd_Self);

    CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP|OECF_ISO);
    WRITEMEM32_LE(&oed->oed_HeadPtr, 0);
    WRITEMEM32_LE(&oed->oed_TailPtr, 0);
    oed->oed_FirstTD = NULL;
    oed->oed_IOReq = &rtn->rtn_IOReq;

    rtn->rtn_IOReq.iouh_DriverPrivate1 = oed;
    rtn->rtn_PTDs[0] = ptd0;
    rtn->rtn_PTDs[1] = ptd1;
    rtn->rtn_NextPTD = 0;
    rtn->rtn_BounceBuffer = NULL;

    return RC_OK;
}

WORD ohciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct PTDNode *ptd;
    struct OhciIsoTD *oitd;
    APTR dmabuffer;
    ULONG phys;
    ULONG frame;
    ULONG ctrl;
    UWORD pktcnt;
    UWORD pktidx;
    UWORD remaining;
    UWORD offset;
    struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
    struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
    ULONG interval;

    pciusbOHCIDebug("OHCI", "%s()\n", __func__);

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
        ULONG framecnt = READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT) & 0xffff;
        ULONG next = rtn->rtn_NextFrame ? rtn->rtn_NextFrame : ((framecnt + interval) & 0xffff);
        rtn->rtn_BufferReq.ubr_Frame = next;
    }
    rtn->rtn_NextFrame = (rtn->rtn_BufferReq.ubr_Frame + interval) & 0xffff;

    dmabuffer = rtn->rtn_BufferReq.ubr_Buffer;
#if __WORDSIZE == 64
    if(!dmabuffer)
        return RC_OK;

    dmabuffer = usbGetBuffer(dmabuffer, rtn->rtn_BufferReq.ubr_Length,
                             rtn->rtn_IOReq.iouh_Dir);
    if(!dmabuffer)
        return UHIOERR_OUTOFMEMORY;

    if(dmabuffer != rtn->rtn_BufferReq.ubr_Buffer &&
            rtn->rtn_IOReq.iouh_Dir == UHDIR_OUT) {
        CopyMem(rtn->rtn_BufferReq.ubr_Buffer, dmabuffer, rtn->rtn_BufferReq.ubr_Length);
    }
#endif
    rtn->rtn_BounceBuffer = (dmabuffer != rtn->rtn_BufferReq.ubr_Buffer) ? dmabuffer : NULL;

    ptd = rtn->rtn_PTDs[rtn->rtn_NextPTD];
    if(!ptd)
        return(UHIOERR_BADPARAMS);

    oitd = (struct OhciIsoTD *)ptd->ptd_Descriptor;
    if(!oitd)
        return(UHIOERR_BADPARAMS);

    if(!dmabuffer || !rtn->rtn_BufferReq.ubr_Length)
        return RC_OK;

    phys = (ULONG)(IPTR)pciGetPhysical(hc, dmabuffer);
    frame = rtn->rtn_BufferReq.ubr_Frame;

    pktcnt = (rtn->rtn_BufferReq.ubr_Length + rtn->rtn_IOReq.iouh_MaxPktSize - 1) /
             rtn->rtn_IOReq.iouh_MaxPktSize;
    if(pktcnt > 8)
        pktcnt = 8;

    ctrl = (frame << OITCS_STARTINGFRAME) | OITF_NOINT |
           ((pktcnt - 1) << OITCS_FRAMECOUNT) | OITF_CC_NOTACCESSED;

    oitd->oitd_Succ = NULL;
    oitd->oitd_ED = (struct OhciED *)rtn->rtn_IOReq.iouh_DriverPrivate1;
    oitd->oitd_Length = rtn->rtn_BufferReq.ubr_Length;
    WRITEMEM32_LE(&oitd->oitd_Ctrl, ctrl);
    WRITEMEM32_LE(&oitd->oitd_BufferPage0, phys & ~0xfff);
    WRITEMEM32_LE(&oitd->oitd_NextTD, 0);

    remaining = rtn->rtn_BufferReq.ubr_Length;
    offset = phys & 0xfff;
    for(pktidx = 0; pktidx < pktcnt; pktidx++) {
        UWORD pktlen = remaining;
        if(pktlen > rtn->rtn_IOReq.iouh_MaxPktSize)
            pktlen = rtn->rtn_IOReq.iouh_MaxPktSize;

        oitd->oitd_Offset[pktidx] = offset | OITM_PSW_CC;
        offset += pktlen;
        remaining -= pktlen;
    }
    while(pktidx < 8) {
        oitd->oitd_Offset[pktidx++] = OITM_PSW_CC;
    }

    WRITEMEM32_LE(&oitd->oitd_BufferEnd, phys + offset - 1);

    CacheClearE(oitd, sizeof(*oitd), CACRF_ClearD);

    ptd->ptd_Length = offset;
    ptd->ptd_FrameIdx = frame;
    ptd->ptd_Flags = PTDF_BUFFER_VALID;

    rtn->rtn_NextPTD ^= 1;

    return RC_OK;
}

void ohciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
    struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
    struct OhciED *oed = (struct OhciED *)rtn->rtn_IOReq.iouh_DriverPrivate1;
    UWORD idx;
    struct OhciED *intoed;

    pciusbOHCIDebug("OHCI", "%s()\n", __func__);

    if(!oed)
        return;

    WRITEMEM32_LE(&oed->oed_EPCaps,
                  OECF_ISO |
                  ((rtn->rtn_IOReq.iouh_DevAddr << OECS_DEVADDR) & OECM_DEVADDR) |
                  ((rtn->rtn_IOReq.iouh_Endpoint << OECS_ENDPOINT) & OECM_ENDPOINT) |
                  (rtn->rtn_IOReq.iouh_MaxPktSize << OECS_MAXPKTLEN));
    oed->oed_IOReq = ioreq;
    ioreq->iouh_DriverPrivate2 = rtn;

    for(idx = 0; idx < 2; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        struct OhciIsoTD *oitd;

        if(!ptd || !(ptd->ptd_Flags & PTDF_BUFFER_VALID))
            continue;

        oitd = (struct OhciIsoTD *)ptd->ptd_Descriptor;
        if(!oitd)
            continue;

        WRITEMEM32_LE(&oitd->oitd_NextTD, ohcihcp->ohc_OhciTermTD->otd_Self);
        oitd->oitd_ED = oed;
        CacheClearE(oitd, sizeof(*oitd), CACRF_ClearD);

        oed->oed_FirstTD = (struct OhciTD *)oitd;
        WRITEMEM32_LE(&oed->oed_TailPtr, ohcihcp->ohc_OhciTermTD->otd_Self);
        WRITEMEM32_LE(&oed->oed_HeadPtr, READMEM32_LE(&oitd->oitd_Self));
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);

        ptd->ptd_Flags |= PTDF_ACTIVE;
    }

    if(!oed->oed_Pred) {
        if(rtn->rtn_IOReq.iouh_Interval >= 31) {
            intoed = ohcihcp->ohc_OhciIntED[4];
        } else {
            UWORD cnt = 0;
            do {
                intoed = ohcihcp->ohc_OhciIntED[cnt++];
            } while(rtn->rtn_IOReq.iouh_Interval >= (1 << cnt));
        }

        Disable();
        oed->oed_Succ = intoed->oed_Succ;
        oed->oed_Pred = intoed;
        oed->oed_NextED = intoed->oed_Succ->oed_Self;
        intoed->oed_Succ->oed_Pred = oed;
        intoed->oed_Succ = oed;
        intoed->oed_NextED = oed->oed_Self;
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);
        CacheClearE(&intoed->oed_EPCaps, 16, CACRF_ClearD);
        ohciUpdateIntTree(hc);
        Enable();
    }

    WRITEMEM32_LE(&oed->oed_EPCaps, READMEM32_LE(&oed->oed_EPCaps) & ~OECF_SKIP);
    CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);
}

void ohciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct OhciED *oed = (struct OhciED *)rtn->rtn_IOReq.iouh_DriverPrivate1;
    UWORD ptdcount = rtn->rtn_PTDCount;
    UWORD limit = (ptdcount < 2) ? ptdcount : 2;
    UWORD idx;

    pciusbOHCIDebug("OHCI", "%s()\n", __func__);

    if(oed && !(READMEM32_LE(&oed->oed_EPCaps) & OECF_SKIP)) {
        ohciDisableED(oed);
        ohciEnableInt(hc, OISF_SOF);
    }

    for(idx = 0; idx < limit; idx++) {
        if(rtn->rtn_PTDs[idx])
            rtn->rtn_PTDs[idx]->ptd_Flags &= ~(PTDF_ACTIVE|PTDF_BUFFER_VALID);
    }

    if(rtn->rtn_BounceBuffer &&
            rtn->rtn_BounceBuffer != rtn->rtn_BufferReq.ubr_Buffer) {
        usbReleaseBuffer(rtn->rtn_BounceBuffer, rtn->rtn_BufferReq.ubr_Buffer,
                         rtn->rtn_BufferReq.ubr_Length, rtn->rtn_IOReq.iouh_Dir);
        rtn->rtn_BounceBuffer = NULL;
    }
}

void ohciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct OhciED *oed = (struct OhciED *)rtn->rtn_IOReq.iouh_DriverPrivate1;
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;
    UWORD limit = (ptdcount < 2) ? ptdcount : 2;

    pciusbOHCIDebug("OHCI", "%s()\n", __func__);

    ohciStopIsochIO(hc, rtn);

    if(oed) {
        ohciFreeED(hc, oed);
        rtn->rtn_IOReq.iouh_DriverPrivate1 = NULL;
    }

    for(idx = 0; idx < limit; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd) {
            if(ptd->ptd_Descriptor)
                ohciFreeIsoTD(hc, (struct OhciIsoTD *)ptd->ptd_Descriptor);
            FreeMem(ptd, sizeof(*ptd));
            rtn->rtn_PTDs[idx] = NULL;
        }
    }

    rtn->rtn_BounceBuffer = NULL;
}

#if defined(AROS_USE_LOGRES)
#undef LogResBase
#undef LogHandle
#endif
#undef base
