/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved

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

static inline struct OhciPTDPrivate *ohciPTDPrivate(struct PTDNode *ptd)
{
    return (struct OhciPTDPrivate *)ptd->ptd_Chipset;
}

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

static struct PTDNode *ohciNextIsoPTD(struct RTIsoNode *rtn)
{
    if (!rtn || !rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return NULL;

    for (UWORD offset = 0; offset < rtn->rtn_PTDCount; offset++) {
        UWORD idx = (rtn->rtn_NextPTD + offset) % rtn->rtn_PTDCount;
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];

        if (!ptd)
            continue;

        if (!(ptd->ptd_Flags & (PTDF_ACTIVE | PTDF_BUFFER_VALID))) {
            rtn->rtn_NextPTD = (idx + 1) % rtn->rtn_PTDCount;
            return ptd;
        }
    }

    return NULL;
}

WORD ohciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct OhciED *oed = NULL;
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;

    pciusbOHCIDebug("OHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || !ptdcount)
        return(UHIOERR_BADPARAMS);

    for(idx = 0; idx < ptdcount; idx++) {
        if(rtn->rtn_PTDs[idx]) {
            struct PTDNode *old = rtn->rtn_PTDs[idx];
            if(old->ptd_Descriptor)
                ohciFreeIsoTD(hc, (struct OhciIsoTD *)old->ptd_Descriptor);
            if(old->ptd_Chipset)
                FreeMem(old->ptd_Chipset, sizeof(struct OhciPTDPrivate));
            FreeMem(old, sizeof(*old));
            rtn->rtn_PTDs[idx] = NULL;
        }
    }

    oed = ohciAllocED(hc);
    if(!oed)
        return(UHIOERR_OUTOFMEMORY);

    for(idx = 0; idx < ptdcount; idx++) {
        struct PTDNode *ptd = AllocMem(sizeof(*ptd), MEMF_CLEAR);
        struct OhciPTDPrivate *ptdpriv = ptd ? AllocMem(sizeof(*ptdpriv), MEMF_CLEAR) : NULL;
        struct OhciIsoTD *oitd = ptd ? ohciAllocIsoTD(hc) : NULL;

        if(!ptd || !ptdpriv || !oitd) {
            if(ptdpriv)
                FreeMem(ptdpriv, sizeof(*ptdpriv));
            if(ptd)
                FreeMem(ptd, sizeof(*ptd));

            for(UWORD freeidx = 0; freeidx < idx; freeidx++) {
                struct PTDNode *old = rtn->rtn_PTDs[freeidx];
                if(old->ptd_Descriptor)
                    ohciFreeIsoTD(hc, (struct OhciIsoTD *)old->ptd_Descriptor);
                if(old->ptd_Chipset)
                    FreeMem(old->ptd_Chipset, sizeof(struct OhciPTDPrivate));
                FreeMem(old, sizeof(*old));
                rtn->rtn_PTDs[freeidx] = NULL;
            }

            ohciFreeED(hc, oed);
            return(UHIOERR_OUTOFMEMORY);
        }

        ptd->ptd_Descriptor = oitd;
        ptd->ptd_Phys = READMEM32_LE(&oitd->oitd_Self);
        ptd->ptd_Chipset = ptdpriv;
        ptdpriv->ptd_RTIsoNode = rtn;
        rtn->rtn_PTDs[idx] = ptd;
    }

    CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP|OECF_ISO);
    WRITEMEM32_LE(&oed->oed_HeadPtr, 0);
    WRITEMEM32_LE(&oed->oed_TailPtr, 0);
    oed->oed_FirstTD = NULL;
    oed->oed_IOReq = &rtn->rtn_IOReq;

    rtn->rtn_IOReq.iouh_DriverPrivate1 = oed;
    rtn->rtn_NextPTD = 0;
    rtn->rtn_BounceBuffer = NULL;

    return RC_OK;
}

WORD ohciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
    struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
    struct PTDNode *ptd = ohciNextIsoPTD(rtn);
    struct IOUsbHWBufferReq *bufreq;
    ULONG interval;
    BOOL explicit_frame = FALSE;

    pciusbOHCIDebug("OHCI", "%s()\n", __func__);

    if (!ptd)
        return UHIOERR_NAKTIMEOUT;

    struct OhciPTDPrivate *ptdpriv = ohciPTDPrivate(ptd);
    bufreq = &ptdpriv->ptd_BufferReq;
    *bufreq = rtn->rtn_BufferReq;
    explicit_frame = (rtn->rtn_Flags & RTISO_FLAG_EXPLICIT_FRAME) != 0;

    if (urti && !explicit_frame)
        bufreq->ubr_Frame = 0;

    if (urti) {
        if (ioreq->iouh_Dir == UHDIR_IN) {
            if (urti->urti_InReqHook)
                CallHookPkt(urti->urti_InReqHook, rtn, bufreq);
        } else {
            if (urti->urti_OutReqHook)
                CallHookPkt(urti->urti_OutReqHook, rtn, bufreq);
        }
        if (bufreq->ubr_Frame)
            explicit_frame = TRUE;
    } else {
        bufreq->ubr_Buffer = ioreq->iouh_Data;
        bufreq->ubr_Length = ioreq->iouh_Length;
        bufreq->ubr_Frame = ioreq->iouh_Frame;
        bufreq->ubr_Flags = 0;
    }

    if (!bufreq->ubr_Length)
        bufreq->ubr_Length = ioreq->iouh_Length;

    if (urti) {
        if (explicit_frame)
            rtn->rtn_Flags |= RTISO_FLAG_EXPLICIT_FRAME;
        else
            rtn->rtn_Flags &= ~RTISO_FLAG_EXPLICIT_FRAME;
    }

    if (urti && !(rtn->rtn_Flags & RTISO_FLAG_EXPLICIT_FRAME))
        bufreq->ubr_Frame = 0;

    interval = ioreq->iouh_Interval ? ioreq->iouh_Interval : 1;
    if (!bufreq->ubr_Frame) {
        ULONG current_frame;
        ULONG lead = interval * 8;
        ULONG next = 0;

        if (lead < 8)
            lead = 8;

        current_frame = READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT) & 0xffff;

        if (rtn->rtn_NextFrame) {
            LONG delta = (LONG)rtn->rtn_NextFrame - (LONG)current_frame;
            if (delta > 0 && delta < (LONG)(0x10000 - lead))
                next = rtn->rtn_NextFrame;
        }

        if (!next)
            next = (current_frame + lead) & 0xffff;

        bufreq->ubr_Frame = next;
        pciusbOHCIDebug("OHCI", "ISO schedule current=%ld next=%ld lead=%ld interval=%ld\n",
                        current_frame, bufreq->ubr_Frame, lead, interval);
    } else {
        ULONG current_frame;
        ULONG lead = interval * 8;
        LONG delta;

        if (lead < 8)
            lead = 8;

        current_frame = READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT) & 0xffff;
        delta = (LONG)bufreq->ubr_Frame - (LONG)current_frame;
        if (delta <= 0 || delta >= (LONG)(0x10000 - lead)) {
            bufreq->ubr_Frame = (current_frame + lead) & 0xffff;
            pciusbOHCIDebug("OHCI", "ISO schedule adjusted current=%ld next=%ld lead=%ld interval=%ld\n",
                            current_frame, bufreq->ubr_Frame, lead, interval);
        } else {
            pciusbOHCIDebug("OHCI", "ISO schedule current=%ld interval=%ld\n",
                            bufreq->ubr_Frame, interval);
        }
    }

    rtn->rtn_NextFrame = (bufreq->ubr_Frame + interval) & 0xffff;

    ptd->ptd_FrameIdx = bufreq->ubr_Frame;
    ptd->ptd_Length = bufreq->ubr_Length;
    ptd->ptd_Flags |= PTDF_BUFFER_VALID;
    ptdpriv->ptd_BounceBuffer = NULL;
    rtn->rtn_BufferReq = *bufreq;

    return RC_OK;
}

void ohciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
    struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
    struct OhciED *oed = (struct OhciED *)rtn->rtn_IOReq.iouh_DriverPrivate1;
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;
    struct OhciED *intoed;
    struct OhciTD *lasttd = NULL;
    ULONG headphys;

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

    headphys = READMEM32_LE(&oed->oed_HeadPtr) & OHCI_PTRMASK;
    if (headphys && headphys != ohcihcp->ohc_OhciTermTD->otd_Self) {
        struct OhciTD *scan = (struct OhciTD *)((IPTR)headphys - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
        ULONG nextphys;

        while(scan) {
            nextphys = READMEM32_LE(&scan->otd_NextTD) & OHCI_PTRMASK;
            if(!nextphys || nextphys == ohcihcp->ohc_OhciTermTD->otd_Self)
                break;
            scan = (struct OhciTD *)((IPTR)nextphys - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
        }
        lasttd = scan;
    }

    for(idx = 0; idx < ptdcount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        struct OhciPTDPrivate *ptdpriv;
        struct OhciIsoTD *oitd;
        APTR dmabuffer;
        ULONG phys;
        ULONG frame;
        ULONG ctrl;
        UWORD pktcnt;
        UWORD pktidx;
        UWORD remaining;
        UWORD offset;
        ULONG bus_end_off;

        if(!ptd || !(ptd->ptd_Flags & PTDF_BUFFER_VALID))
            continue;

        if(ptd->ptd_Flags & PTDF_ACTIVE)
            continue;

        ptdpriv = ohciPTDPrivate(ptd);
        oitd = (struct OhciIsoTD *)ptd->ptd_Descriptor;
        if(!oitd)
            continue;

        dmabuffer = ptdpriv->ptd_BufferReq.ubr_Buffer;
#if __WORDSIZE == 64
        if(dmabuffer) {
            dmabuffer = usbGetBuffer(dmabuffer, ptdpriv->ptd_BufferReq.ubr_Length,
                                     rtn->rtn_IOReq.iouh_Dir);
            if(!dmabuffer) {
                ptd->ptd_Flags &= ~PTDF_BUFFER_VALID;
                continue;
            }

            if(dmabuffer != ptdpriv->ptd_BufferReq.ubr_Buffer &&
                    rtn->rtn_IOReq.iouh_Dir == UHDIR_OUT) {
                CopyMem(ptdpriv->ptd_BufferReq.ubr_Buffer, dmabuffer, ptdpriv->ptd_BufferReq.ubr_Length);
            }
        }
#endif
        ptdpriv->ptd_BounceBuffer = (dmabuffer != ptdpriv->ptd_BufferReq.ubr_Buffer) ? dmabuffer : NULL;

        if(!dmabuffer || !ptdpriv->ptd_BufferReq.ubr_Length) {
            ptd->ptd_Flags &= ~PTDF_BUFFER_VALID;
            continue;
        }

        phys = (ULONG)(IPTR)pciGetPhysical(hc, dmabuffer);
        frame = ptdpriv->ptd_BufferReq.ubr_Frame;

        pktcnt = (ptdpriv->ptd_BufferReq.ubr_Length + rtn->rtn_IOReq.iouh_MaxPktSize - 1) /
                 rtn->rtn_IOReq.iouh_MaxPktSize;
        if(pktcnt > 8)
            pktcnt = 8;

        ctrl = (frame << OITCS_STARTINGFRAME) |
               ((pktcnt - 1) << OITCS_FRAMECOUNT) | OITF_CC_NOTACCESSED;

        oitd->oitd_Succ = NULL;
        oitd->oitd_ED = oed;
        oitd->oitd_Length = ptdpriv->ptd_BufferReq.ubr_Length;
        WRITEMEM32_LE(&oitd->oitd_Ctrl, ctrl);
        WRITEMEM32_LE(&oitd->oitd_BufferPage0, phys & ~0xfff);
        WRITEMEM32_LE(&oitd->oitd_NextTD, ohcihcp->ohc_OhciTermTD->otd_Self);

        remaining = ptdpriv->ptd_BufferReq.ubr_Length;
        offset = (UWORD)(phys & 0xfff);
        bus_end_off = offset;

        /*
         * OHCI ISO PSW Offset field contains the offset of the last byte of each
         * packet relative to BufferPage0, modulo 4 KiB. The HC will advance to the
         * next page when the offset wraps.
         */
        for(pktidx = 0; pktidx < pktcnt; pktidx++) {
            UWORD pktlen = remaining;
            if(pktlen > rtn->rtn_IOReq.iouh_MaxPktSize)
                pktlen = rtn->rtn_IOReq.iouh_MaxPktSize;

            bus_end_off += pktlen;
            oitd->oitd_Offset[pktidx] = ((bus_end_off - 1) & 0xfff) | OITM_PSW_CC;
            remaining -= pktlen;
        }
        while(pktidx < 8) {
            oitd->oitd_Offset[pktidx++] = OITM_PSW_CC;
        }

        WRITEMEM32_LE(&oitd->oitd_BufferEnd, phys + ptdpriv->ptd_BufferReq.ubr_Length - 1);

        CacheClearE(oitd, sizeof(*oitd), CACRF_ClearD);
        SYNC;

        if(lasttd) {
            WRITEMEM32_LE(&lasttd->otd_NextTD, READMEM32_LE(&oitd->oitd_Self));
            CacheClearE(lasttd, sizeof(*lasttd), CACRF_ClearD);
        } else {
            WRITEMEM32_LE(&oed->oed_HeadPtr, READMEM32_LE(&oitd->oitd_Self));
            oed->oed_FirstTD = (struct OhciTD *)oitd;
        }

        lasttd = (struct OhciTD *)oitd;

        ptd->ptd_Flags |= PTDF_ACTIVE;
    }

    if(lasttd) {
        WRITEMEM32_LE(&oed->oed_TailPtr, ohcihcp->ohc_OhciTermTD->otd_Self);
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);
        SYNC;
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
        SYNC;
        CacheClearE(&intoed->oed_EPCaps, 16, CACRF_ClearD);
        SYNC;
        ohciUpdateIntTree(hc);
        Enable();
    }

    WRITEMEM32_LE(&oed->oed_EPCaps, READMEM32_LE(&oed->oed_EPCaps) & ~OECF_SKIP);
    CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);
    SYNC;
}

void ohciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct OhciED *oed = (struct OhciED *)rtn->rtn_IOReq.iouh_DriverPrivate1;
    UWORD ptdcount = rtn->rtn_PTDCount;
    UWORD idx;

    pciusbOHCIDebug("OHCI", "%s()\n", __func__);

    if(oed && !(READMEM32_LE(&oed->oed_EPCaps) & OECF_SKIP)) {
        ohciDisableED(oed);
        ohciEnableInt(hc, OISF_SOF);
    }

    for(idx = 0; idx < ptdcount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd) {
            struct OhciPTDPrivate *ptdpriv = ohciPTDPrivate(ptd);
            ptd->ptd_Flags &= ~(PTDF_ACTIVE|PTDF_BUFFER_VALID);
            if(ptdpriv->ptd_BounceBuffer &&
                    ptdpriv->ptd_BounceBuffer != ptdpriv->ptd_BufferReq.ubr_Buffer) {
                usbReleaseBuffer(ptdpriv->ptd_BounceBuffer, ptdpriv->ptd_BufferReq.ubr_Buffer,
                                 ptdpriv->ptd_BufferReq.ubr_Length, rtn->rtn_IOReq.iouh_Dir);
                ptdpriv->ptd_BounceBuffer = NULL;
            }
        }
    }
}

void ohciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct OhciED *oed = (struct OhciED *)rtn->rtn_IOReq.iouh_DriverPrivate1;
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;

    pciusbOHCIDebug("OHCI", "%s()\n", __func__);

    ohciStopIsochIO(hc, rtn);

    if(oed) {
        ohciFreeED(hc, oed);
        rtn->rtn_IOReq.iouh_DriverPrivate1 = NULL;
    }

    for(idx = 0; idx < ptdcount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd) {
            if(ptd->ptd_Descriptor)
                ohciFreeIsoTD(hc, (struct OhciIsoTD *)ptd->ptd_Descriptor);
            if(ptd->ptd_Chipset)
                FreeMem(ptd->ptd_Chipset, sizeof(struct OhciPTDPrivate));
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
