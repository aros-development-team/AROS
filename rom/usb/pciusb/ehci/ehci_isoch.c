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

#define EHCI_FRAME_NUMBER_MASK  0x1fff
#define EHCI_MICROFRAME_BW_MAX  6000

static ULONG ehciGetFrameNumber(struct PCIController *hc)
{
    return (READREG32_LE(hc->hc_RegBase, EHCI_FRAMECOUNT) >> 3) & EHCI_FRAME_NUMBER_MASK;
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
    ULONG slot = ptd->ptd_FrameIdx;
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

        for(idx = 0; idx < ptdcount; idx++) {
            struct PTDNode *ptd = rtn->rtn_PTDs[idx];

            if(!ptd || !(ptd->ptd_Flags & PTDF_ACTIVE))
                continue;

            struct EhciPTDPrivate *ptdpriv = ehciPTDPrivate(ptd);
            struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
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

                CacheClearE(itd, sizeof(*itd), CACRF_InvalidateD);
                for(pkt = 0; pkt < ptdpriv->ptd_PktCount; pkt++) {
                    ULONG trans = READMEM32_LE(&itd->itd_Transaction[pkt]);
                    UWORD residual = (trans >> EITF_LENGTH_SHIFT) & EITF_LENGTH_MASK;
                    UWORD pktlen = ptdpriv->ptd_PktLength[pkt];

                    if(trans & EITF_STATUS_ACTIVE) {
                        error = FALSE;
                        break;
                    }

                    if(trans & (EITF_STATUS_DBE|EITF_STATUS_BABBLE|EITF_STATUS_XACTERR))
                        error = TRUE;

                    if(pktlen > residual)
                        actual += pktlen - residual;
                }

                if(pkt < ptdpriv->ptd_PktCount)
                    continue;

                ioreq->iouh_Actual = actual;
                ioreq->iouh_Req.io_Error = error ? UHIOERR_HOSTERROR : UHIOERR_NO_ERROR;
                ehciUnlinkIsoDescriptor(ehcihcp, ptd);
                ehciFreeITD(hc, itd);
                ptd->ptd_Descriptor = NULL;
                ptd->ptd_NextPTD = NULL;
            }

            rtn->rtn_BufferReq.ubr_Length = ioreq->iouh_Actual;
            rtn->rtn_BufferReq.ubr_Frame = ptd->ptd_FrameIdx;

            if(rtn->rtn_BounceBuffer &&
                    rtn->rtn_BounceBuffer != rtn->rtn_BufferReq.ubr_Buffer) {
                usbReleaseBuffer(rtn->rtn_BounceBuffer, rtn->rtn_BufferReq.ubr_Buffer,
                                 rtn->rtn_BufferReq.ubr_Length, rtn->rtn_IOReq.iouh_Dir);
                rtn->rtn_BounceBuffer = NULL;
            }

            if(urti) {
                if(rtn->rtn_IOReq.iouh_Dir == UHDIR_IN) {
                    if(urti->urti_InReqHook)
                        CallHookPkt(urti->urti_InReqHook, rtn, &rtn->rtn_BufferReq);
                    if(urti->urti_InDoneHook)
                        CallHookPkt(urti->urti_InDoneHook, rtn, &rtn->rtn_BufferReq);
                } else {
                    if(urti->urti_OutDoneHook)
                        CallHookPkt(urti->urti_OutDoneHook, rtn, &rtn->rtn_BufferReq);
                }
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
            } else if (urti) {
                if (ehciQueueIsochIO(hc, rtn) == RC_OK) {
                    if(!ehciStartIsochIO(hc, rtn)) {
                        rtn = next;
                        break;
                    }
                }
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
    rtn->rtn_BounceBuffer = NULL;

    return RC_OK;
}

WORD ehciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
    struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
    struct EhciHCPrivate *ehcihcp = (struct EhciHCPrivate *)hc->hc_CPrivate;
    ULONG interval;

    pciusbEHCIDebug("EHCI", "%s()\n", __func__);

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
        ULONG frameidx = ehciGetFrameNumber(hc);
        ULONG next = rtn->rtn_NextFrame ? rtn->rtn_NextFrame : ((frameidx + interval) & EHCI_FRAME_NUMBER_MASK);
        rtn->rtn_BufferReq.ubr_Frame = (UWORD)next;
    } else {
        rtn->rtn_BufferReq.ubr_Frame &= EHCI_FRAME_NUMBER_MASK;
    }
    rtn->rtn_NextFrame = (rtn->rtn_BufferReq.ubr_Frame + interval) & EHCI_FRAME_NUMBER_MASK;

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
    APTR dmabuffer;
    ULONG offset = 0;
    ULONG remaining;
    UWORD scheduled = 0;
    BOOL scheduled_any = FALSE;

    pciusbEHCIDebug("EHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || !ptdcount)
        return FALSE;

    dmabuffer = rtn->rtn_BufferReq.ubr_Buffer;
#if __WORDSIZE == 64
    if(dmabuffer && !ehcihcp->ehc_64BitCapable) {
        dmabuffer = usbGetBuffer(dmabuffer, rtn->rtn_BufferReq.ubr_Length,
                                 rtn->rtn_IOReq.iouh_Dir);
        if(!dmabuffer) {
            ehciAbortIsochIO(hc, rtn, UHIOERR_OUTOFMEMORY);
            return FALSE;
        }

        if(dmabuffer != rtn->rtn_BufferReq.ubr_Buffer &&
                rtn->rtn_IOReq.iouh_Dir == UHDIR_OUT) {
            CopyMem(rtn->rtn_BufferReq.ubr_Buffer, dmabuffer, rtn->rtn_BufferReq.ubr_Length);
        }
    }
#endif
    rtn->rtn_BounceBuffer = (dmabuffer != rtn->rtn_BufferReq.ubr_Buffer) ? dmabuffer : NULL;
    remaining = rtn->rtn_BufferReq.ubr_Length;

    for(idx = 0; idx < ptdcount; idx++) {
        UWORD ptdidx = (rtn->rtn_NextPTD + idx) % ptdcount;
        struct PTDNode *ptd = rtn->rtn_PTDs[ptdidx];
        struct EhciITD *itd;
        struct EhciSiTD *sitd;
        APTR buffer;
        IPTR phys;
        ULONG len;
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

        if(!remaining)
            break;

        struct EhciPTDPrivate *ptdpriv = ehciPTDPrivate(ptd);
        buffer = (UBYTE *)dmabuffer + offset;
        len = ptd->ptd_Length ? ptd->ptd_Length : remaining;
        if(len > remaining)
            len = remaining;
        ptdlen = len;

        if(!buffer || !len)
            continue;

        is_split = (ptd->ptd_Flags & PTDF_SITD) != 0;
        slot = (rtn->rtn_BufferReq.ubr_Frame + scheduled) & framemask;
        phys = (IPTR)pciGetPhysical(hc, buffer);
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

            WRITEMEM32_LE(&sitd->sitd_Token, ESITF_STATUS_ACTIVE |
                          ((ptdlen & ESITF_LENGTH_MASK) << ESITF_LENGTH_SHIFT));

            ehciSetPointer(ehcihcp, sitd->sitd_BufferPtr[0], sitd->sitd_ExtBufferPtr[0],
                           (phys & EITM_BUFFER_BASE) | (phys & ESITM_BP0_OFFSET_MASK));
            ehciSetPointer(ehcihcp, sitd->sitd_BufferPtr[1], sitd->sitd_ExtBufferPtr[1],
                           (phys & EITM_BUFFER_BASE) | smask | ((ULONG)cmask << 8));
            WRITEMEM32_LE(&sitd->sitd_BackPointer, EHCI_TERMINATE);

            ptdpriv->ptd_PktCount = 1;
            ptdpriv->ptd_PktLength[0] = (UWORD)ptdlen;

            ptdpriv->ptd_NextPhys = ehcihcp->ehc_IsoAnchor[slot];
            ptd->ptd_NextPTD = NULL;
            WRITEMEM32_LE(&sitd->sitd_Next, ptdpriv->ptd_NextPhys);
            CacheClearE(sitd, sizeof(*sitd), CACRF_ClearD);
        } else {
            itd = (struct EhciITD *) ptd->ptd_Descriptor;
            pktcnt = (ptdlen + rtn->rtn_IOReq.iouh_MaxPktSize - 1) / rtn->rtn_IOReq.iouh_MaxPktSize;
            if(pktcnt > 8)
                pktcnt = 8;

            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[0], itd->itd_ExtBufferPtr[0],
                           (phys & EITM_BUFFER_BASE) |
                           EITM_DEVADDR(rtn->rtn_IOReq.iouh_DevAddr) |
                           EITM_ENDPT(rtn->rtn_IOReq.iouh_Endpoint));
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[1], itd->itd_ExtBufferPtr[1],
                           ((phys + 4096) & EITM_BUFFER_BASE) |
                           EITM_MAXPKTSIZE(rtn->rtn_IOReq.iouh_MaxPktSize));
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[2], itd->itd_ExtBufferPtr[2],
                           ((phys + 8192) & EITM_BUFFER_BASE) |
                           EITM_BUFFER_DIR(rtn->rtn_IOReq.iouh_Dir == UHDIR_IN));
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[3], itd->itd_ExtBufferPtr[3], (phys + 12288) & EITM_BUFFER_BASE);
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[4], itd->itd_ExtBufferPtr[4], (phys + 16384) & EITM_BUFFER_BASE);
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[5], itd->itd_ExtBufferPtr[5], (phys + 20480) & EITM_BUFFER_BASE);
            ehciSetPointer(ehcihcp, itd->itd_BufferPtr[6], itd->itd_ExtBufferPtr[6], (phys + 24576) & EITM_BUFFER_BASE);

            pktoffset = phys & EITM_BUFFER_OFFSET;
            for(pktidx = 0; pktidx < pktcnt; pktidx++) {
                pktlen = (len > rtn->rtn_IOReq.iouh_MaxPktSize) ? rtn->rtn_IOReq.iouh_MaxPktSize : len;
                ptdpriv->ptd_PktLength[pktidx] = pktlen;
                len -= pktlen;
                pktoffset += pktlen;
            }

            if (!ehciIsoBandwidthAvailable(ehcihcp, slot, FALSE, ptdpriv->ptd_PktLength, pktcnt, 0, 0, 0))
                break;

            pktoffset = phys & EITM_BUFFER_OFFSET;
            len = ptdlen;
            for(pktidx = 0; pktidx < pktcnt; pktidx++) {
                pktlen = ptdpriv->ptd_PktLength[pktidx];
                WRITEMEM32_LE(&itd->itd_Transaction[pktidx],
                              EITF_STATUS_ACTIVE |
                              ((pktoffset >> 12) << EITF_PAGESELECT_SHIFT) |
                              ((pktlen & EITF_LENGTH_MASK) << EITF_LENGTH_SHIFT));
                len -= pktlen;
                pktoffset += pktlen;
            }

            while(pktidx < 8) {
                itd->itd_Transaction[pktidx++] = 0;
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

        ptd->ptd_FrameIdx = slot;
        ptd->ptd_Length = ptdlen;
        ptd->ptd_Flags |= (PTDF_ACTIVE | PTDF_BUFFER_VALID);
        remaining -= ptdlen;
        offset += ptdlen;
        scheduled++;
        scheduled_any = TRUE;
        rtn->rtn_NextPTD = (ptdidx + 1) % ptdcount;
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

    if(rtn->rtn_BounceBuffer &&
            rtn->rtn_BounceBuffer != rtn->rtn_BufferReq.ubr_Buffer) {
        usbReleaseBuffer(rtn->rtn_BounceBuffer, rtn->rtn_BufferReq.ubr_Buffer,
                         rtn->rtn_BufferReq.ubr_Length, rtn->rtn_IOReq.iouh_Dir);
        rtn->rtn_BounceBuffer = NULL;
    }
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
