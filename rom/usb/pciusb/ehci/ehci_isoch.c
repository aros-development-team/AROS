/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved

    Desc: EHCI chipset driver isochronous/RTIso support functions
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/pci.h>
#include <utility/hooks.h>
#include <exec/memory.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "ehciproto.h"
#include "ohci/ohcichip.h"
#include "uhci/uhcichip.h"

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

static inline struct EhciPTDPrivate *ehciPTDPrivate(struct PTDNode *ptd)
{
    return (struct EhciPTDPrivate *)ptd->ptd_Chipset;
}

/* Release a PTD's DMA bounce.  High-bandwidth whole-frame bounces are
   AllocVec'd (also on 32-bit, where usbGetBuffer() is a no-op); controller
   bounces from usbGetBuffer() are released through it. */
static void ehciReleaseBounce(struct EhciPTDPrivate *ptdpriv, UWORD dir, ULONG len)
{
    if(!ptdpriv->ptd_BounceBuffer)
        return;

    if(ptdpriv->ptd_BounceOwned) {
        FreeVec(ptdpriv->ptd_BounceBuffer);
    } else {
        usbReleaseBuffer(ptdpriv->ptd_BounceBuffer, ptdpriv->ptd_BufferReq.ubr_Buffer,
                         len, dir);
    }

    ptdpriv->ptd_BounceBuffer = NULL;
    ptdpriv->ptd_BounceOwned = FALSE;
}

/* FRINDEX is 14 bits; after >> 3 the frame number is 11 bits and wraps
   at 2048, not at 8192. */
#define EHCI_FRAME_NUMBER_MASK  0x07ff
#define EHCI_ISOCH_THRESHOLD    2
#define EHCI_ISOCH_LEAD         4
#define EHCI_ISOCH_QUEUE_DEPTH  4
#define EHCI_MICROFRAME_BW_MAX  6000

static ULONG ehciGetFrameNumber(struct PCIController *hc)
{
    return (READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT) >> 3) & EHCI_FRAME_NUMBER_MASK;
}

/* Signed distance from 'earlier' to 'later', modulo the frame-number wrap.
   Plain subtraction breaks across the wrap; sign-extend from 11 bits. */
static LONG ehciFrameDelta(UWORD later, UWORD earlier)
{
    LONG delta = (LONG)((later - earlier) & EHCI_FRAME_NUMBER_MASK);

    if (delta & ((EHCI_FRAME_NUMBER_MASK + 1) >> 1))
        delta -= EHCI_FRAME_NUMBER_MASK + 1;

    return delta;
}

static ULONG ehciGetIsochLead(ULONG interval)
{
    return interval > EHCI_ISOCH_LEAD ? interval : EHCI_ISOCH_LEAD;
}

/* iouh_Interval is in microframes for high-speed endpoints, but an iTD is
   scheduled per frame: 1..8 microframes all map to one frame. */
static ULONG ehciIsoFrameInterval(struct IOUsbHWReq *ioreq)
{
    ULONG interval = ioreq->iouh_Interval ? ioreq->iouh_Interval : 1;

    if (ioreq->iouh_Flags & UHFF_HIGHSPEED)
        interval = (interval + 7) >> 3;

    return interval ? interval : 1;
}

/* True when every transaction of this descriptor has completed. */
static BOOL ehciIsoPTDComplete(struct PTDNode *ptd)
{
    struct EhciPTDPrivate *ptdpriv = ehciPTDPrivate(ptd);

    if (!ptd->ptd_Descriptor)
        return FALSE;

    if (ptd->ptd_Flags & PTDF_SITD) {
        struct EhciSiTD *sitd = (struct EhciSiTD *)ptd->ptd_Descriptor;
        ULONG token;

        CacheClearE(sitd, sizeof(*sitd), CACRF_InvalidateD);
        token = READMEM32_LE(&sitd->sitd_Token);
        return (token & ESITF_STATUS_ACTIVE) ? FALSE : TRUE;
    } else {
        struct EhciITD *itd = (struct EhciITD *)ptd->ptd_Descriptor;
        UWORD pkt;

        CacheClearE(itd, sizeof(*itd), CACRF_InvalidateD);
        for (pkt = 0; pkt < ptdpriv->ptd_PktCount; pkt++) {
            ULONG trans = READMEM32_LE(&itd->itd_Transaction[pkt * ptdpriv->ptd_MfStep]);
            if (trans & EITF_STATUS_ACTIVE)
                return FALSE;
        }
        return TRUE;
    }
}

static UWORD ehciIsoPendingPTDs(struct RTIsoNode *rtn)
{
    UWORD pending = 0;

    if (rtn && rtn->rtn_PTDs) {
        for (UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
            struct PTDNode *ptd = rtn->rtn_PTDs[idx];

            if (ptd && (ptd->ptd_Flags & PTDF_ACTIVE))
                pending++;
        }
    }

    return pending;
}

static UWORD ehciIsoClampFrame(struct RTIsoNode *rtn, UWORD candidate,
                              ULONG interval, struct PTDNode *exclude)
{
    UWORD idx;
    UWORD maxf = candidate;
    BOOL have = FALSE;

    if (!rtn || !rtn->rtn_PTDs)
        return candidate;

    for (idx = 0; idx < rtn->rtn_PTDCount; idx++) {
        struct PTDNode *p = rtn->rtn_PTDs[idx];

        if (!p || p == exclude || !(p->ptd_Flags & PTDF_ACTIVE))
            continue;
        if (!have || ehciFrameDelta(p->ptd_FrameIdx, maxf) > 0) {
            maxf = p->ptd_FrameIdx;
            have = TRUE;
        }
    }

    if (have && ehciFrameDelta(candidate, maxf) <= 0)
        candidate = (maxf + interval) & EHCI_FRAME_NUMBER_MASK;

    return candidate;
}

static void ehciIsoAccumulateBandwidth(struct PTDNode *ptd, ULONG usage[8])
{
    if (!ptd || !(ptd->ptd_Flags & PTDF_ACTIVE))
        return;

    if (ptd->ptd_Flags & PTDF_SITD) {
        struct EhciSiTD *sitd = (struct EhciSiTD *)ptd->ptd_Descriptor;
        ULONG maskval = READMEM32_LE(&sitd->sitd_BufferPtr[1]);
        UBYTE smask = (UBYTE)(maskval & EITM_SMASK);
        UBYTE cmask = (UBYTE)((maskval & EITM_CMASK) >> 8);

        for (UWORD idx = 0; idx < 8; idx++) {
            if (smask & (1U << idx))
                usage[idx] += ptd->ptd_Length;
            if (cmask & (1U << idx))
                usage[idx] += ptd->ptd_Length;
        }
        return;
    }

    {
        struct EhciPTDPrivate *ptdpriv = ehciPTDPrivate(ptd);
        for (UWORD idx = 0; idx < ptdpriv->ptd_PktCount; idx++)
            usage[idx] += ptdpriv->ptd_PktLength[idx];
    }
}

static BOOL ehciIsoBandwidthAvailable(struct EhciHCPrivate *ehcihcp, ULONG slot, BOOL is_split,
                                      const UWORD *pktlen, UWORD pktcnt, ULONG ptdlen,
                                      UBYTE smask, UBYTE cmask)
{
    ULONG usage[8] = { 0 };
    struct PTDNode *scan = ehcihcp->ehc_IsoHead[slot];

    while (scan) {
        ehciIsoAccumulateBandwidth(scan, usage);
        scan = scan->ptd_NextPTD;
    }

    if (is_split) {
        for (UWORD idx = 0; idx < 8; idx++) {
            if (smask & (1U << idx)) {
                if (usage[idx] + ptdlen > EHCI_MICROFRAME_BW_MAX)
                    return FALSE;
            }
            if (cmask & (1U << idx)) {
                if (usage[idx] + ptdlen > EHCI_MICROFRAME_BW_MAX)
                    return FALSE;
            }
        }
        return TRUE;
    }

    for (UWORD idx = 0; idx < pktcnt; idx++) {
        if (usage[idx] + pktlen[idx] > EHCI_MICROFRAME_BW_MAX)
            return FALSE;
    }

    return TRUE;
}

static void ehciSelectSplitMasks(const struct IOUsbHWReq *ioreq, UBYTE *smask, UBYTE *cmask)
{
    UBYTE start = (UBYTE)((ioreq->iouh_DevAddr + ioreq->iouh_Endpoint) % 3);

    *smask = (UBYTE)(1U << start);
    *cmask = (UBYTE)((1U << (start + 2)) | (1U << (start + 3)) | (1U << (start + 4)));
}

static void ehciUnlinkIsoDescriptor(struct EhciHCPrivate *ehcihcp, struct PTDNode *ptd)
{
    ULONG slot = ptd->ptd_FrameIdx & ehcihcp->ehc_FrameListMask;
    struct PTDNode *head = ehcihcp->ehc_IsoHead ? ehcihcp->ehc_IsoHead[slot] : NULL;
    struct PTDNode *prev = NULL;

    if(!head)
        return;

    if(ptd != head) {
        prev = head;
        while(prev && prev->ptd_NextPTD != ptd)
            prev = prev->ptd_NextPTD;
    }

    if(prev) {
        prev->ptd_NextPTD = ptd->ptd_NextPTD;
        if(ptd == ehcihcp->ehc_IsoTail[slot])
            ehcihcp->ehc_IsoTail[slot] = prev;

        if(prev->ptd_Flags & PTDF_SITD) {
            struct EhciSiTD *sitd = (struct EhciSiTD *)prev->ptd_Descriptor;
            WRITEMEM32_LE(&sitd->sitd_Next, prev->ptd_NextPTD ? prev->ptd_NextPTD->ptd_Phys : ehcihcp->ehc_IsoAnchor[slot]);
            CacheClearE(sitd, sizeof(*sitd), CACRF_ClearD);
        } else {
            struct EhciITD *itd = (struct EhciITD *)prev->ptd_Descriptor;
            WRITEMEM32_LE(&itd->itd_Next, prev->ptd_NextPTD ? prev->ptd_NextPTD->ptd_Phys : ehcihcp->ehc_IsoAnchor[slot]);
            CacheClearE(itd, sizeof(*itd), CACRF_ClearD);
        }
    } else if(ptd == head) {
        struct PTDNode *next = ptd->ptd_NextPTD;

        ehcihcp->ehc_IsoHead[slot] = next;
        ehcihcp->ehc_IsoTail[slot] = next ? ehcihcp->ehc_IsoTail[slot] : NULL;

        WRITEMEM32_LE(&ehcihcp->ehc_EhciFrameList[slot], next ? next->ptd_Phys : ehcihcp->ehc_IsoAnchor[slot]);
        CacheClearE(&ehcihcp->ehc_EhciFrameList[slot], sizeof(ULONG), CACRF_ClearD);
    }
}

void ehciHandleIsochTDs(struct PCIController *hc)
{
    struct RTIsoNode *rtn;
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    struct PCIUnit *unit = hc->hc_Unit;

    rtn = (struct RTIsoNode *)hc->hc_RTIsoHandlers.mlh_Head;
    while(rtn->rtn_Node.mln_Succ) {
        struct RTIsoNode *next = (struct RTIsoNode *)rtn->rtn_Node.mln_Succ;
        struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
        UWORD idx;
        UWORD ptdcount = rtn->rtn_PTDCount;

        if(!rtn->rtn_PTDs || !ptdcount) {
            rtn = (struct RTIsoNode *) rtn->rtn_Node.mln_Succ;
            continue;
        }

        for(;;) {
            struct PTDNode *ptd = NULL;
            LONG bestframe = 0;
            UWORD curframe = (UWORD)ehciGetFrameNumber(hc);

            /* Two iTDs can complete before this runs.  Deliver the done
               hooks in scheduled-frame order (oldest first), not PTD index
               order, so clients see data in the order it was sent. */
            for(idx = 0; idx < ptdcount; idx++) {
                struct PTDNode *cand = rtn->rtn_PTDs[idx];
                LONG delta;

                if(!cand || !(cand->ptd_Flags & PTDF_ACTIVE))
                    continue;
                if(!ehciIsoPTDComplete(cand))
                    continue;

                delta = ehciFrameDelta((UWORD)cand->ptd_FrameIdx, curframe);
                if(!ptd || delta < bestframe) {
                    ptd = cand;
                    bestframe = delta;
                }
            }

            if(!ptd)
                break;

            struct EhciPTDPrivate *ptdpriv = ehciPTDPrivate(ptd);
            struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
            ULONG txn_off[8];
            ULONG txn_len[8];
            UWORD txn_count = 0;
            if(ptd->ptd_Flags & PTDF_SITD) {
                struct EhciSiTD *sitd = (struct EhciSiTD *)ptd->ptd_Descriptor;
                ULONG token;
                ULONG remaining;
                BOOL error = FALSE;

                CacheClearE(sitd, sizeof(*sitd), CACRF_InvalidateD);
                token = READMEM32_LE(&sitd->sitd_Token);
                if(token & ESITF_STATUS_ACTIVE)
                    continue;

                remaining = (token >> ESITF_LENGTH_SHIFT) & ESITF_LENGTH_MASK;
                if(token & (ESITF_STATUS_ERR|ESITF_STATUS_BABBLE|ESITF_STATUS_XACTERR|ESITF_STATUS_MISSEDUF))
                    error = TRUE;

                ioreq->iouh_Actual = (ptd->ptd_Length > remaining) ? (ptd->ptd_Length - remaining) : 0;
                ioreq->iouh_Req.io_Error = error ? UHIOERR_HOSTERROR : UHIOERR_NO_ERROR;
                ehciUnlinkIsoDescriptor(ehcihcp, ptd);
                ehciFreeSiTD(hc, sitd);
                ptd->ptd_Descriptor = NULL;
                ptd->ptd_NextPTD = NULL;
            } else {
                struct EhciITD *itd = (struct EhciITD *)ptd->ptd_Descriptor;
                ULONG actual = 0;
                BOOL error = FALSE;
                UWORD pkt;
                ULONG pos = 0;

                CacheClearE(itd, sizeof(*itd), CACRF_InvalidateD);
                for(pkt = 0; pkt < ptdpriv->ptd_PktCount; pkt++) {
                    ULONG trans = READMEM32_LE(&itd->itd_Transaction[pkt * ptdpriv->ptd_MfStep]);
                    UWORD length = (trans >> EITF_LENGTH_SHIFT) & EITF_LENGTH_MASK;
                    UWORD pktlen = ptdpriv->ptd_PktLength[pkt];
                    ULONG n = 0;

                    txn_off[pkt] = pos;
                    pos += pktlen;
                    txn_len[pkt] = 0;

                    if(trans & EITF_STATUS_ACTIVE) {
                        break;
                    }

                    if(trans & (EITF_STATUS_DBE|EITF_STATUS_BABBLE|EITF_STATUS_XACTERR))
                        error = TRUE;

                    if(rtn->rtn_IOReq.iouh_Dir == UHDIR_IN) {
                        /* EHCI: for IN the field is the number of bytes
                           received (actual).  Zero means no data this
                           microframe. */
                        n = length;
                    } else {
                        if(length == pktlen)
                            n = pktlen;
                        else if(pktlen > length)
                            n = pktlen - length;
                    }
                    txn_len[pkt] = n;
                    actual += n;
                }

                if(pkt < ptdpriv->ptd_PktCount)
                    continue;

                txn_count = ptdpriv->ptd_PktCount;
                ioreq->iouh_Actual = actual;
                ioreq->iouh_Req.io_Error = error ? UHIOERR_HOSTERROR : UHIOERR_NO_ERROR;
                ehciUnlinkIsoDescriptor(ehcihcp, ptd);
                ehciFreeITD(hc, itd);
                ptd->ptd_Descriptor = NULL;
                ptd->ptd_NextPTD = NULL;
            }

            struct IOUsbHWBufferReq *bufreq = &ptdpriv->ptd_BufferReq;

            bufreq->ubr_Length = ioreq->iouh_Actual;
            bufreq->ubr_Frame = ptd->ptd_FrameIdx;

            if(rtn->rtn_IOReq.iouh_Dir == UHDIR_IN) {
                const UBYTE *src = (const UBYTE *)(ptdpriv->ptd_BounceBuffer
                                                   ? ptdpriv->ptd_BounceBuffer
                                                   : bufreq->ubr_Buffer);

                /* Hand each completed transaction to the client with its
                   exact length.  An aggregate length cannot describe the
                   per-microframe split when packets are short. */
                if(urti && urti->urti_InDoneHook && src) {
                    UWORD ti;
                    for(ti = 0; ti < txn_count; ti++) {
                        struct IOUsbHWBufferReq sub;
                        ULONG n = txn_len[ti];

                        /* The received length is made available to the request
                           hook, like the non-EHCI HCDs and denebusb do, so
                           clients that only supply a buffer still get data. */
                        sub.ubr_Buffer = NULL;
                        sub.ubr_Length = n;
                        sub.ubr_Frame = ptd->ptd_FrameIdx;
                        sub.ubr_Flags = 0;

                        if(urti->urti_InReqHook)
                            CallHookPkt(urti->urti_InReqHook, rtn, &sub);

                        if(n && sub.ubr_Buffer) {
                            ULONG copy = n;

                            if(sub.ubr_Length < copy)
                                copy = sub.ubr_Length;
                            if(copy)
                                CopyMem((APTR)(src + txn_off[ti]),
                                        sub.ubr_Buffer, copy);
                            sub.ubr_Length = copy;
                        } else {
                            /* Zero-length transactions still get a done hook;
                               some clients use it to finish an image. */
                            sub.ubr_Length = 0;
                        }

                        sub.ubr_Frame = ptd->ptd_FrameIdx;
                        CallHookPkt(urti->urti_InDoneHook, rtn, &sub);
                    }
                }
                /* IN data was already copied per transaction above, so the
                   bounce itself needs no copy-back. */
                ehciReleaseBounce(ptdpriv, rtn->rtn_IOReq.iouh_Dir, 0);
            } else {
                ehciReleaseBounce(ptdpriv, rtn->rtn_IOReq.iouh_Dir, bufreq->ubr_Length);
                if(urti && urti->urti_OutDoneHook)
                    CallHookPkt(urti->urti_OutDoneHook, rtn, bufreq);
            }

            ptd->ptd_Flags &= ~(PTDF_ACTIVE|PTDF_BUFFER_VALID);

            if (rtn->rtn_StdReq == ioreq) {
                UWORD devadrep = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint +
                                 ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                Remove((struct Node *)&rtn->rtn_Node);
                unit->hu_DevBusyReq[devadrep] = NULL;
                ReplyMsg(&ioreq->iouh_Req.io_Message);
                ehciFreeIsochIO(hc, rtn);
                pciusbFreeStdIsoNode(hc, rtn);
                urti = NULL;
                rtn = next;
                break;
            }
        }

        if (urti) {
            /* Refill the fixed in-flight depth after drops: a zero-length
               request frees a PTD without a completion, so one requeue per
               done hook would let the queue shrink permanently. */
            for (;;) {
                UWORD pending = ehciIsoPendingPTDs(rtn);

                if (pending >= EHCI_ISOCH_QUEUE_DEPTH)
                    break;
                if (ehciQueueIsochIO(hc, rtn) != RC_OK)
                    break;
                if (!ehciStartIsochIO(hc, rtn))
                    break;
                if (ehciIsoPendingPTDs(rtn) <= pending)
                    break;
            }
        }

        rtn = next;
    }
}

void ehciScheduleIsoTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;

    pciusbEHCIDebug("EHCI", "Scheduling new ISO transfers...\n");
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

        if (ehciInitIsochIO(hc, rtn) != RC_OK) {
            pciusbFreeStdIsoNode(hc, rtn);
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            ioreq = (struct IOUsbHWReq *)hc->hc_IsoXFerQueue.lh_Head;
            continue;
        }

        ehciQueueIsochIO(hc, rtn);
        AddTail((struct List *)&hc->hc_RTIsoHandlers, (struct Node *)&rtn->rtn_Node);
        unit->hu_DevBusyReq[devadrep] = ioreq;
        ehciStartIsochIO(hc, rtn);

        ioreq = (struct IOUsbHWReq *)hc->hc_IsoXFerQueue.lh_Head;
    }
}

WORD ehciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    BOOL split = (rtn->rtn_IOReq.iouh_Flags & UHFF_SPLITTRANS) != 0;
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;

    pciusbEHCIDebug("EHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || !ptdcount)
        return UHIOERR_BADPARAMS;

    for(idx = 0; idx < ptdcount; idx++) {
        struct PTDNode *ptd;

        rtn->rtn_PTDs[idx] = NULL;
        ptd = AllocMem(sizeof(*ptd), MEMF_CLEAR);
        struct EhciPTDPrivate *ptdpriv = ptd ? AllocMem(sizeof(*ptdpriv), MEMF_CLEAR) : NULL;
        if(!ptd) {
            ehciFreeIsochIO(hc, rtn);
            return(UHIOERR_OUTOFMEMORY);
        }
        if(!ptdpriv) {
            FreeMem(ptd, sizeof(*ptd));
            ehciFreeIsochIO(hc, rtn);
            return(UHIOERR_OUTOFMEMORY);
        }

        if(split) {
            struct EhciSiTD *sitd = ehciAllocSiTD(hc);
            if(!sitd) {
                FreeMem(ptdpriv, sizeof(*ptdpriv));
                FreeMem(ptd, sizeof(*ptd));
                ehciFreeIsochIO(hc, rtn);
                return(UHIOERR_OUTOFMEMORY);
            }

            ptd->ptd_Descriptor = sitd;
            ptd->ptd_Phys = READMEM32_LE(&sitd->sitd_Self);
            ptd->ptd_Flags |= PTDF_SITD;
        } else {
            struct EhciITD *itd = ehciAllocITD(hc);
            if(!itd) {
                FreeMem(ptdpriv, sizeof(*ptdpriv));
                FreeMem(ptd, sizeof(*ptd));
                ehciFreeIsochIO(hc, rtn);
                return(UHIOERR_OUTOFMEMORY);
            }

            ptd->ptd_Descriptor = itd;
            ptd->ptd_Phys = READMEM32_LE(&itd->itd_Self);
        }

        ptd->ptd_Chipset = ptdpriv;
        rtn->rtn_PTDs[idx] = ptd;
    }

    rtn->rtn_NextPTD = 0;
    rtn->rtn_NextFrame = 0;
    rtn->rtn_Flags &= ~(RTISO_FLAG_CADENCE_VALID|RTISO_FLAG_EXPLICIT_FRAME);
    rtn->rtn_BounceBuffer = NULL;

    return RC_OK;
}

static struct PTDNode *ehciNextIsoPTD(struct RTIsoNode *rtn)
{
    UWORD offset;

    if (!rtn || !rtn->rtn_PTDs || !rtn->rtn_PTDCount)
        return NULL;

    for (offset = 0; offset < rtn->rtn_PTDCount; offset++) {
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

WORD ehciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
    struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
    struct PTDNode *ptd = ehciNextIsoPTD(rtn);
    struct EhciPTDPrivate *ptdpriv;
    struct IOUsbHWBufferReq *bufreq;
    ULONG interval;
    BOOL explicit_frame;

    pciusbEHCIDebug("EHCI", "%s()\n", __func__);

    if (!ptd)
        return UHIOERR_NAKTIMEOUT;

    /* Give each PTD its own buffer request, like OHCI.  With one shared
       rtn_BufferReq only a single iTD could ever be in flight, which forced
       interval-1 streams to run one iTD per scheduling lead. */
    ptdpriv = ehciPTDPrivate(ptd);
    bufreq = &ptdpriv->ptd_BufferReq;
    *bufreq = rtn->rtn_BufferReq;
    explicit_frame = FALSE;

    /* Never inherit a completed frame as an explicit frame; the request
       hook must ask for one on this call.  A stale past frame would make the
       new iTD run only after the frame list wraps. */
    bufreq->ubr_Frame = 0;

    if (urti) {
        if (ioreq->iouh_Dir == UHDIR_IN) {
            if (urti->urti_InReqHook)
                CallHookPkt(urti->urti_InReqHook, rtn, bufreq);
        } else {
            if (urti->urti_OutReqHook)
                CallHookPkt(urti->urti_OutReqHook, rtn, bufreq);
        }
    } else {
        bufreq->ubr_Buffer = ioreq->iouh_Data;
        bufreq->ubr_Length = ioreq->iouh_Length;
        bufreq->ubr_Frame = ioreq->iouh_Frame;
        bufreq->ubr_Flags = 0;
    }

    if (!bufreq->ubr_Length)
        bufreq->ubr_Length = ioreq->iouh_Length;

    interval = ehciIsoFrameInterval(ioreq);
    {
        ULONG frameidx = ehciGetFrameNumber(hc);
        ULONG lead = ehciGetIsochLead(interval);

        /* Only accept an explicit frame that is still in the unambiguous
           future window; anything else is rescheduled from now. */
        if (bufreq->ubr_Frame) {
            ULONG frame = bufreq->ubr_Frame & EHCI_FRAME_NUMBER_MASK;
            LONG delta = ehciFrameDelta((UWORD)frame, (UWORD)frameidx);

            if (delta >= EHCI_ISOCH_THRESHOLD && delta < (LONG)(EHCI_FRAME_NUMBER_MASK / 2)) {
                explicit_frame = TRUE;
                bufreq->ubr_Frame = (UWORD)frame;
            } else {
                bufreq->ubr_Frame = 0;
            }
        }

        if (!bufreq->ubr_Frame) {
            ULONG next;
            BOOL have_next = FALSE;

            if (rtn->rtn_Flags & RTISO_FLAG_CADENCE_VALID) {
                LONG delta = ehciFrameDelta((UWORD)rtn->rtn_NextFrame, (UWORD)frameidx);

                /* Accept a running cadence only when it is beyond the
                   scheduling threshold and still unambiguous.  Anything
                   else (including a stale value from a previous run) is
                   rescheduled from now. */
                if (delta >= EHCI_ISOCH_THRESHOLD && delta < (LONG)(EHCI_FRAME_NUMBER_MASK / 2)) {
                    next = rtn->rtn_NextFrame;
                    have_next = TRUE;
                }
            }
            if (!have_next)
                next = (frameidx + lead) & EHCI_FRAME_NUMBER_MASK;
            bufreq->ubr_Frame = (UWORD)next;
        }

    }

    if (urti) {
        if (explicit_frame)
            rtn->rtn_Flags |= RTISO_FLAG_EXPLICIT_FRAME;
        else
            rtn->rtn_Flags &= ~RTISO_FLAG_EXPLICIT_FRAME;
    }

    bufreq->ubr_Frame = ehciIsoClampFrame(rtn, bufreq->ubr_Frame, interval, ptd);
    rtn->rtn_NextFrame = (bufreq->ubr_Frame + interval) & EHCI_FRAME_NUMBER_MASK;
    rtn->rtn_Flags |= RTISO_FLAG_CADENCE_VALID;

    ptd->ptd_FrameIdx = bufreq->ubr_Frame;
    ptd->ptd_Length = bufreq->ubr_Length;
    ptd->ptd_Flags |= PTDF_BUFFER_VALID;
    ptdpriv->ptd_BounceBuffer = NULL;
    rtn->rtn_BufferReq = *bufreq;
    /* RT iso contract (cf. denebusb/OHCI): advance the client buffer past
       the data just queued; the next request hook continues from there
       unless it supplies a new buffer. */
    if(urti)
        rtn->rtn_BufferReq.ubr_Buffer += bufreq->ubr_Length;

    return RC_OK;
}

static void ehciAbortIsochIO(struct PCIController *hc, struct RTIsoNode *rtn, UWORD error)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
    UWORD devadrep = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint +
                     ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);

    if(rtn->rtn_Node.mln_Succ) {
        Remove((struct Node *)&rtn->rtn_Node);
    }

    unit->hu_DevBusyReq[devadrep] = NULL;

    if(rtn->rtn_RTIso) {
        rtn->rtn_RTIso->urti_DriverPrivate1 = NULL;
        rtn->rtn_RTIso = NULL;
        ehciFreeIsochIO(hc, rtn);
        if(rtn->rtn_PTDs) {
            FreeMem(rtn->rtn_PTDs, rtn->rtn_PTDCount * sizeof(struct PTDNode *));
            rtn->rtn_PTDs = NULL;
            rtn->rtn_PTDCount = 0;
        }
        AddHead((struct List *)&unit->hu_FreeRTIsoNodes, (struct Node *)&rtn->rtn_Node);
        return;
    }

    ioreq->iouh_Req.io_Error = error;
    ReplyMsg(&ioreq->iouh_Req.io_Message);
    ehciFreeIsochIO(hc, rtn);
    pciusbFreeStdIsoNode(hc, rtn);
}

BOOL ehciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;
    BOOL scheduled_any = FALSE;
    ULONG interval = ehciIsoFrameInterval(&rtn->rtn_IOReq);
    ULONG lead = ehciGetIsochLead(interval);
    UWORD curframe = (UWORD)ehciGetFrameNumber(hc);
    UWORD assign = (curframe + lead) & EHCI_FRAME_NUMBER_MASK;
    BOOL adjusted = FALSE;

    pciusbEHCIDebug("EHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || !ptdcount)
        return FALSE;

    for(idx = 0; idx < ptdcount; idx++) {
        UWORD ptdidx = idx;
        struct PTDNode *ptd = rtn->rtn_PTDs[ptdidx];
        struct EhciITD *itd;
        struct EhciSiTD *sitd;
        APTR buffer;
        APTR dmabuffer;
        IPTR phys;
        ULONG len;
        ULONG multi = 1;
        ULONG dma_len;
        BOOL hw_bw = FALSE;
        ULONG slot;
        ULONG framemask = ehcihcp->ehc_FrameListMask;
        UWORD pktlen;
        UWORD pktidx;
        UWORD pktcnt;
        ULONG pktoffset;
        ULONG ptdlen;
        BOOL is_split;
        UBYTE smask = 0;
        UBYTE cmask = 0;

        if(!ptd)
            continue;

        if(ptd->ptd_Flags & PTDF_ACTIVE)
            continue;

        if(!ptd->ptd_Descriptor) {
            if(ptd->ptd_Flags & PTDF_SITD) {
                sitd = ehciAllocSiTD(hc);
                if(!sitd)
                    continue;
                ptd->ptd_Descriptor = sitd;
                ptd->ptd_Phys = READMEM32_LE(&sitd->sitd_Self);
            } else {
                itd = ehciAllocITD(hc);
                if(!itd)
                    continue;
                ptd->ptd_Descriptor = itd;
                ptd->ptd_Phys = READMEM32_LE(&itd->itd_Self);
            }
        }

        if(!(ptd->ptd_Flags & PTDF_BUFFER_VALID))
            continue;

        struct EhciPTDPrivate *ptdpriv = ehciPTDPrivate(ptd);
        struct IOUsbHWBufferReq *bufreq = &ptdpriv->ptd_BufferReq;
        buffer = bufreq->ubr_Buffer;
        len = bufreq->ubr_Length;

        /* A request hook that supplies no buffer still asked for a request:
           send it as a zero-length transfer (nothing is read through the
           pointer of an empty iTD, so any address will do), like pcixhci and
           denebusb do.  Dropping it can empty the in-flight queue and stop the
           stream for good. */
        if(!buffer) {
            len = 0;
            buffer = (APTR)ptd;
        }
        ptdlen = len;

        dma_len = len;
        if(!(ptd->ptd_Flags & PTDF_SITD)) {
            ULONG mflags = rtn->rtn_IOReq.iouh_Flags;

            /* UHFF_MULTI_3 is UHFF_MULTI_1|UHFF_MULTI_2, and Poseidon sets
               UHFF_MULTI_1 on every high-speed endpoint, so a bit test
               classifies all of them as Mult 3.  Compare the field. */
            if((mflags & UHFF_MULTI_3) == UHFF_MULTI_3)
                multi = 3;
            else if(mflags & UHFF_MULTI_2)
                multi = 2;

            /* High-bandwidth IN: the client requests one microframe's worth
               (Multi * max packet); an iTD covers eight microframes. */
            if(multi > 1 && len && rtn->rtn_IOReq.iouh_Dir == UHDIR_IN) {
                hw_bw = TRUE;
                dma_len = (ULONG)rtn->rtn_IOReq.iouh_MaxPktSize * multi * 8;
            }
        }

        dmabuffer = buffer;
        ptdpriv->ptd_BounceOwned = FALSE;
        if(hw_bw) {
            /* A high-bandwidth iTD spans 8 microframes, so use a physically
               contiguous bounce for the whole frame.  This is needed on
               32-bit builds too: the client buffer only covers one microframe
               and is not guaranteed contiguous. */
            dmabuffer = AllocVec(dma_len, MEMF_31BIT|MEMF_PUBLIC);
            if(!dmabuffer) {
                ehciAbortIsochIO(hc, rtn, UHIOERR_OUTOFMEMORY);
                return FALSE;
            }
            ptdpriv->ptd_BounceOwned = TRUE;
        } else if(dmabuffer && !ehcihcp->ehc_64BitCapable) {
            dmabuffer = usbGetBuffer(dmabuffer, len, rtn->rtn_IOReq.iouh_Dir);
            if(!dmabuffer) {
                ehciAbortIsochIO(hc, rtn, UHIOERR_OUTOFMEMORY);
                return FALSE;
            }
            if(dmabuffer != buffer && rtn->rtn_IOReq.iouh_Dir == UHDIR_OUT)
                CopyMem(buffer, dmabuffer, len);
        }
        ptdpriv->ptd_BounceBuffer = (dmabuffer != buffer) ? dmabuffer : NULL;

        {
            LONG fd = ehciFrameDelta((UWORD)ptd->ptd_FrameIdx, curframe);

            if (adjusted || fd < (LONG)EHCI_ISOCH_THRESHOLD ||
                    fd >= (LONG)(EHCI_FRAME_NUMBER_MASK / 2)) {
                assign = ehciIsoClampFrame(rtn, assign, interval, ptd);
                ptd->ptd_FrameIdx = assign;
                assign = (assign + interval) & EHCI_FRAME_NUMBER_MASK;
                adjusted = TRUE;
            }
        }

        is_split = (ptd->ptd_Flags & PTDF_SITD) != 0;
        slot = ptd->ptd_FrameIdx & framemask;
        phys = (IPTR)pciGetPhysical(hc, dmabuffer);
        ptdpriv->ptd_PktCount = 0;
        for(pktidx = 0; pktidx < 8; pktidx++)
            ptdpriv->ptd_PktLength[pktidx] = 0;

        if(is_split) {
            sitd = (struct EhciSiTD *)ptd->ptd_Descriptor;
            ehciSelectSplitMasks(&rtn->rtn_IOReq, &smask, &cmask);

            if (!ehciIsoBandwidthAvailable(ehcihcp, slot, TRUE, NULL, 0, ptdlen, smask, cmask))
                break;

            WRITEMEM32_LE(&sitd->sitd_EPCaps,
                          ESIM_DEVADDR(rtn->rtn_IOReq.iouh_DevAddr) |
                          ESIM_ENDPT(rtn->rtn_IOReq.iouh_Endpoint) |
                          ((rtn->rtn_IOReq.iouh_Dir == UHDIR_IN) ? ESIM_DIRECTION_IN : 0) |
                          ESIM_PORT(rtn->rtn_IOReq.iouh_SplitHubPort) |
                          ESIM_HUB(rtn->rtn_IOReq.iouh_SplitHubAddr));

            /* siTD has no separate completion interrupt bit: bit 31 is IOC
               and the active state is the low status bit.  Split transfers
               behind a USB 2.0 hub never complete without both. */
            WRITEMEM32_LE(&sitd->sitd_Token, ESITF_IOC | ESITF_STATUS_ACTIVE |
                          ((ptdlen & ESITF_LENGTH_MASK) << ESITF_LENGTH_SHIFT));

            ehciSetPointer(ehcihcp, sitd->sitd_BufferPtr[0], sitd->sitd_ExtBufferPtr[0],
                           ((IPTR)phys & ~(IPTR)EITM_BUFFER_OFFSET) | (phys & ESITM_BP0_OFFSET_MASK));
            ehciSetPointer(ehcihcp, sitd->sitd_BufferPtr[1], sitd->sitd_ExtBufferPtr[1],
                           (((IPTR)phys + 4096) & ~(IPTR)EITM_BUFFER_OFFSET) | smask | ((ULONG)cmask << 8));
            WRITEMEM32_LE(&sitd->sitd_BackPointer, EHCI_TERMINATE);

            ptdpriv->ptd_PktCount = 1;
            ptdpriv->ptd_PktLength[0] = (UWORD)ptdlen;

            ptdpriv->ptd_NextPhys = ehcihcp->ehc_IsoAnchor[slot];
            ptd->ptd_NextPTD = NULL;
            WRITEMEM32_LE(&sitd->sitd_Next, ptdpriv->ptd_NextPhys);
            CacheClearE(sitd, sizeof(*sitd), CACRF_ClearD);
        } else {
            itd = (struct EhciITD *) ptd->ptd_Descriptor;
            ULONG per_mf = (ULONG)rtn->rtn_IOReq.iouh_MaxPktSize * multi;

            if(hw_bw) {
                pktcnt = 8;
                ptdpriv->ptd_MfStep = 1;
                for(pktidx = 0; pktidx < 8; pktidx++)
                    ptdpriv->ptd_PktLength[pktidx] = (UWORD)per_mf;
            } else {
                ULONG mfstep = 8;
                ULONG maxtxn;

                if(rtn->rtn_IOReq.iouh_Flags & UHFF_HIGHSPEED) {
                    ULONG iv = rtn->rtn_IOReq.iouh_Interval ?
                               rtn->rtn_IOReq.iouh_Interval : 1;

                    if(iv > 8)
                        iv = 8;
                    mfstep = 1;
                    while((mfstep << 1) <= iv)
                        mfstep <<= 1;
                }

                maxtxn = 8 / mfstep;
                if(!maxtxn)
                    maxtxn = 1;

                if(!ptdlen) {
                    /* Zero-length requests are still scheduled, with a
                       single zero-length transaction and IOC, so they keep
                       their slot and completion.  Dropping them can empty the
                       in-flight queue and stall the stream (cf. pcixhci). */
                    pktcnt = 1;
                } else {
                    pktcnt = (ptdlen + rtn->rtn_IOReq.iouh_MaxPktSize - 1) /
                             rtn->rtn_IOReq.iouh_MaxPktSize;
                    if(pktcnt > maxtxn)
                        pktcnt = (UWORD)maxtxn;
                }

                ptdpriv->ptd_MfStep = (UWORD)mfstep;

                pktoffset = phys & EITM_BUFFER_OFFSET;
                for(pktidx = 0; pktidx < pktcnt; pktidx++) {
                    pktlen = (len > rtn->rtn_IOReq.iouh_MaxPktSize) ?
                             rtn->rtn_IOReq.iouh_MaxPktSize : len;
                    ptdpriv->ptd_PktLength[pktidx] = pktlen;
                    len -= pktlen;
                    pktoffset += pktlen;
                }
            }

            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[0], itd->itd_ExtBufferPtr[0],
                           ((IPTR)phys & ~(IPTR)EITM_BUFFER_OFFSET) |
                           EITM_DEVADDR(rtn->rtn_IOReq.iouh_DevAddr) |
                           EITM_ENDPT(rtn->rtn_IOReq.iouh_Endpoint));
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[1], itd->itd_ExtBufferPtr[1],
                           (((IPTR)phys + 4096) & ~(IPTR)EITM_BUFFER_OFFSET) |
                           EITM_BUFFER_DIR(rtn->rtn_IOReq.iouh_Dir == UHDIR_IN) |
                           EITM_MAXPKTSIZE(rtn->rtn_IOReq.iouh_MaxPktSize));
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[2], itd->itd_ExtBufferPtr[2],
                           (((IPTR)phys + 8192) & ~(IPTR)EITM_BUFFER_OFFSET) |
                           EITM_MULTI(multi));
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[3], itd->itd_ExtBufferPtr[3], ((IPTR)phys + 12288) & ~(IPTR)EITM_BUFFER_OFFSET);
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[4], itd->itd_ExtBufferPtr[4], ((IPTR)phys + 16384) & ~(IPTR)EITM_BUFFER_OFFSET);
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[5], itd->itd_ExtBufferPtr[5], ((IPTR)phys + 20480) & ~(IPTR)EITM_BUFFER_OFFSET);
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[6], itd->itd_ExtBufferPtr[6], ((IPTR)phys + 24576) & ~(IPTR)EITM_BUFFER_OFFSET);

            if (!ehciIsoBandwidthAvailable(ehcihcp, slot, FALSE, ptdpriv->ptd_PktLength, pktcnt, 0, 0, 0))
                break;

            for(pktidx = 0; pktidx < 8; pktidx++)
                itd->itd_Transaction[pktidx] = 0;

            pktoffset = phys & EITM_BUFFER_OFFSET;
            len = ptdlen;
            for(pktidx = 0; pktidx < pktcnt; pktidx++) {
                ULONG trans;
                UWORD mfidx = (UWORD)(pktidx * ptdpriv->ptd_MfStep);
                pktlen = ptdpriv->ptd_PktLength[pktidx];
                {
                    /* For IN the controller accumulates up to Transaction X
                       Length bytes in the microframe; it is the buffer
                       available for the whole microframe (per_mf = Mult *
                       max packet for a high-bandwidth iTD), not one max
                       packet.  The per-microframe span below matches it. */
                    UWORD tlen = hw_bw ? (UWORD)per_mf : pktlen;
                    trans = EITF_STATUS_ACTIVE |
                            ((pktoffset >> 12) << EITF_PAGESELECT_SHIFT) |
                            (pktoffset & EITM_BUFFER_OFFSET) |
                            ((tlen & EITF_LENGTH_MASK) << EITF_LENGTH_SHIFT);
                }
                /* RT-ISO clients rely on the completion interrupt to run
                   ehciHandleIsochTDs().  Without IOC on the last transaction
                   the controller never raises USBINT and no request hook /
                   done hook is ever called. */
                if(pktidx == pktcnt - 1)
                    trans |= EITF_IOC;
                WRITEMEM32_LE(&itd->itd_Transaction[mfidx], trans);
                len -= pktlen;
                pktoffset += pktlen;
            }

            ptdpriv->ptd_PktCount = pktcnt;

            ptdpriv->ptd_NextPhys = ehcihcp->ehc_IsoAnchor[slot];
            ptd->ptd_NextPTD = NULL;
            WRITEMEM32_LE(&itd->itd_Next, ptdpriv->ptd_NextPhys);

            CacheClearE(itd, sizeof(*itd), CACRF_ClearD);
        }

        if(ehcihcp->ehc_IsoTail[slot]) {
            struct PTDNode *tail = ehcihcp->ehc_IsoTail[slot];

            tail->ptd_NextPTD = ptd;
            if(tail->ptd_Flags & PTDF_SITD) {
                struct EhciSiTD *taildesc = (struct EhciSiTD *)tail->ptd_Descriptor;
                WRITEMEM32_LE(&taildesc->sitd_Next, ptd->ptd_Phys);
                CacheClearE(taildesc, sizeof(*taildesc), CACRF_ClearD);
            } else {
                struct EhciITD *taildesc = (struct EhciITD *)tail->ptd_Descriptor;
                WRITEMEM32_LE(&taildesc->itd_Next, ptd->ptd_Phys);
                CacheClearE(taildesc, sizeof(*taildesc), CACRF_ClearD);
            }
        } else {
            WRITEMEM32_LE(&ehcihcp->ehc_EhciFrameList[slot], ptd->ptd_Phys);
            CacheClearE(&ehcihcp->ehc_EhciFrameList[slot], sizeof(ULONG), CACRF_ClearD);
            ehcihcp->ehc_IsoHead[slot] = ptd;
        }

        ehcihcp->ehc_IsoTail[slot] = ptd;

        ptd->ptd_Length = ptdlen;
        ptd->ptd_Flags |= PTDF_ACTIVE;
        scheduled_any = TRUE;
    }

    if (adjusted) {
        rtn->rtn_NextFrame = assign;
        rtn->rtn_Flags |= RTISO_FLAG_CADENCE_VALID;
    }

    return scheduled_any;
}

void ehciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;

    pciusbEHCIDebug("EHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs)
        return;

    for(idx = 0; idx < ptdcount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd && (ptd->ptd_Flags & PTDF_ACTIVE)) {
            ehciUnlinkIsoDescriptor(ehcihcp, ptd);
            ptd->ptd_Flags &= ~(PTDF_ACTIVE|PTDF_BUFFER_VALID);
            ptd->ptd_NextPTD = NULL;
        }
    }

    for(idx = 0; idx < ptdcount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd && ptd->ptd_Chipset) {
            struct EhciPTDPrivate *ptdpriv = ehciPTDPrivate(ptd);
            ehciReleaseBounce(ptdpriv, rtn->rtn_IOReq.iouh_Dir,
                              ptdpriv->ptd_BufferReq.ubr_Length);
        }
    }

    /* Drop the old cadence so a later Start does not inherit a stale frame
       (which could otherwise wait a long time before it becomes current). */
    rtn->rtn_NextFrame = 0;
    rtn->rtn_Flags &= ~(RTISO_FLAG_CADENCE_VALID|RTISO_FLAG_EXPLICIT_FRAME);
}

void ehciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;

    pciusbEHCIDebug("EHCI", "%s()\n", __func__);

    ehciStopIsochIO(hc, rtn);

    if(!rtn->rtn_PTDs)
        return;

    for(idx = 0; idx < ptdcount; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd) {
            if(ptd->ptd_Descriptor) {
                if(ptd->ptd_Flags & PTDF_SITD)
                    ehciFreeSiTD(hc, (struct EhciSiTD *)ptd->ptd_Descriptor);
                else
                    ehciFreeITD(hc, (struct EhciITD *)ptd->ptd_Descriptor);
            }
            if(ptd->ptd_Chipset)
                FreeMem(ptd->ptd_Chipset, sizeof(struct EhciPTDPrivate));
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
