/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved

    Desc: UHCI chipset driver isochronous/RTIso support functions
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/pci.h>
#include <utility/hooks.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"
#include "uhciproto.h"

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

static inline struct UhciPTDPrivate *uhciPTDPrivate(struct PTDNode *ptd)
{
    return (struct UhciPTDPrivate *)ptd->ptd_Chipset;
}

static void uhciInsertIsoPTD(struct PCIController *hc, struct PTDNode *ptd, ULONG slot)
{
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    struct PTDNode *head = uhcihcp->uhc_IsoHead[slot];
    struct PTDNode *tail = uhcihcp->uhc_IsoTail[slot];
    struct UhciTD *utd = (struct UhciTD *)ptd->ptd_Descriptor;
    struct UhciPTDPrivate *ptdpriv = uhciPTDPrivate(ptd);
    ULONG prev_entry = READMEM32_LE(&uhcihcp->uhc_UhciFrameList[slot]);

    ptd->ptd_NextPTD = NULL;
    if(!head) {
        ptdpriv->ptd_NextPhys = READMEM32_LE(&uhcihcp->uhc_UhciFrameList[slot]);
        WRITEMEM32_LE(&utd->utd_Link, ptdpriv->ptd_NextPhys);
        SYNC;
        CacheClearE(utd, sizeof(*utd), CACRF_ClearD);

        uhcihcp->uhc_IsoHead[slot] = uhcihcp->uhc_IsoTail[slot] = ptd;
        WRITEMEM32_LE(&uhcihcp->uhc_UhciFrameList[slot], ptd->ptd_Phys);
        SYNC;
        CacheClearE(&uhcihcp->uhc_UhciFrameList[slot], sizeof(ULONG), CACRF_ClearD);
        pciusbUHCIDebug("UHCI", "ISO insert slot=%ld head prev=%08lx new=%08lx\n",
                        slot, prev_entry, ptd->ptd_Phys);
    } else {
        struct UhciTD *tailtd = (struct UhciTD *)tail->ptd_Descriptor;
        ULONG nextphys = READMEM32_LE(&tailtd->utd_Link);

        ptdpriv->ptd_NextPhys = uhciPTDPrivate(head)->ptd_NextPhys;
        WRITEMEM32_LE(&utd->utd_Link, nextphys);
        SYNC;
        CacheClearE(utd, sizeof(*utd), CACRF_ClearD);

        WRITEMEM32_LE(&tailtd->utd_Link, ptd->ptd_Phys);
        SYNC;
        CacheClearE(tailtd, sizeof(*tailtd), CACRF_ClearD);
        tail->ptd_NextPTD = ptd;
        uhcihcp->uhc_IsoTail[slot] = ptd;
        pciusbUHCIDebug("UHCI", "ISO insert slot=%ld tail prev=%08lx new=%08lx\n",
                        slot, nextphys, ptd->ptd_Phys);
    }
}

static void uhciUnlinkIsoPTD(struct PCIController *hc, struct PTDNode *ptd)
{
    struct UhciHCPrivate *uhcihcp = (struct UhciHCPrivate *)hc->hc_CPrivate;
    ULONG slot = ptd->ptd_FrameIdx & (UHCI_FRAMELIST_SIZE - 1);
    struct PTDNode *prev = NULL;
    struct PTDNode *head = uhcihcp->uhc_IsoHead[slot];

    while(head && head != ptd) {
        prev = head;
        head = head->ptd_NextPTD;
    }

    if(!head)
        return;

    struct PTDNode *next = head->ptd_NextPTD;
    struct UhciPTDPrivate *headpriv = uhciPTDPrivate(head);

    if(prev) {
        struct UhciTD *prevtd = (struct UhciTD *)prev->ptd_Descriptor;
        WRITEMEM32_LE(&prevtd->utd_Link, next ? next->ptd_Phys : headpriv->ptd_NextPhys);
        SYNC;
        CacheClearE(prevtd, sizeof(*prevtd), CACRF_ClearD);
        prev->ptd_NextPTD = next;
    } else {
        if(next)
            uhciPTDPrivate(next)->ptd_NextPhys = headpriv->ptd_NextPhys;

        WRITEMEM32_LE(&uhcihcp->uhc_UhciFrameList[slot], next ? next->ptd_Phys : headpriv->ptd_NextPhys);
        SYNC;
        CacheClearE(&uhcihcp->uhc_UhciFrameList[slot], sizeof(ULONG), CACRF_ClearD);
        uhcihcp->uhc_IsoHead[slot] = next;
    }

    if(!next)
        uhcihcp->uhc_IsoTail[slot] = prev;

    head->ptd_NextPTD = NULL;
}

static struct PTDNode *uhciNextIsoPTD(struct RTIsoNode *rtn)
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

void uhciScheduleIsoTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;

    pciusbUHCIDebug("UHCI", "Scheduling new ISO transfers...\n");
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

        if (uhciInitIsochIO(hc, rtn) != RC_OK) {
            pciusbFreeStdIsoNode(hc, rtn);
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            ReplyMsg(&ioreq->iouh_Req.io_Message);
            ioreq = (struct IOUsbHWReq *)hc->hc_IsoXFerQueue.lh_Head;
            continue;
        }

        uhciQueueIsochIO(hc, rtn);
        AddTail((struct List *)&hc->hc_RTIsoHandlers, (struct Node *)&rtn->rtn_Node);
        unit->hu_DevBusyReq[devadrep] = ioreq;
        uhciStartIsochIO(hc, rtn);

        ioreq = (struct IOUsbHWReq *)hc->hc_IsoXFerQueue.lh_Head;
    }
}

WORD uhciInitIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    UWORD idx;
    UWORD ptdcount = rtn->rtn_PTDCount;

    pciusbUHCIDebug("UHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || !ptdcount)
        return UHIOERR_BADPARAMS;

    for(idx = 0; idx < ptdcount; idx++) {
        if(rtn->rtn_PTDs[idx]) {
            struct PTDNode *old = rtn->rtn_PTDs[idx];
            if(old->ptd_Descriptor)
                uhciFreeTD(hc, (struct UhciTD *)old->ptd_Descriptor);
            if(old->ptd_Chipset)
                FreeMem(old->ptd_Chipset, sizeof(struct UhciPTDPrivate));
            FreeMem(old, sizeof(*old));
            rtn->rtn_PTDs[idx] = NULL;
        }
    }

    for(idx = 0; idx < ptdcount; idx++) {
        struct PTDNode *ptd = AllocMem(sizeof(*ptd), MEMF_CLEAR);
        struct UhciPTDPrivate *ptdpriv = ptd ? AllocMem(sizeof(*ptdpriv), MEMF_CLEAR) : NULL;
        struct UhciTD *utd = ptd ? uhciAllocTD(hc) : NULL;

        if(!ptd || !ptdpriv || !utd) {
            if(ptdpriv)
                FreeMem(ptdpriv, sizeof(*ptdpriv));
            if(ptd)
                FreeMem(ptd, sizeof(*ptd));

            for(ULONG freeidx = 0; freeidx < idx; freeidx++) {
                struct PTDNode *old = rtn->rtn_PTDs[freeidx];
                if(old->ptd_Descriptor)
                    uhciFreeTD(hc, (struct UhciTD *)old->ptd_Descriptor);
                if(old->ptd_Chipset)
                    FreeMem(old->ptd_Chipset, sizeof(struct UhciPTDPrivate));
                FreeMem(old, sizeof(*old));
                rtn->rtn_PTDs[freeidx] = NULL;
            }
            return UHIOERR_OUTOFMEMORY;
        }

        ptd->ptd_Descriptor = utd;
        ptd->ptd_Phys = READMEM32_LE(&utd->utd_Self);
        ptd->ptd_Chipset = ptdpriv;
        rtn->rtn_PTDs[idx] = ptd;
    }

    rtn->rtn_NextPTD = 0;
    rtn->rtn_BounceBuffer = NULL;

    return RC_OK;
}

WORD uhciQueueIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);
    struct PTDNode *ptd = uhciNextIsoPTD(rtn);
    struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
    ULONG interval;
    BOOL explicit_frame = FALSE;

    pciusbUHCIDebug("UHCI", "%s(0x%p, 0x%p)\n", __func__, hc, rtn);
    pciusbUHCIDebug("UHCI", "%s: IOReq 0x%p\n", __func__, ioreq);

    if (!ptd)
        return UHIOERR_NAKTIMEOUT;

    struct UhciPTDPrivate *ptdpriv = uhciPTDPrivate(ptd);
    struct IOUsbHWBufferReq *bufreq = &ptdpriv->ptd_BufferReq;
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

        uhciUpdateFrameCounter(hc);
        current_frame = hc->hc_FrameCounter;

        if (rtn->rtn_NextFrame > current_frame)
            next = rtn->rtn_NextFrame;
        else
            next = current_frame + lead;

        bufreq->ubr_Frame = next;
        pciusbUHCIDebug("UHCI", "ISO schedule current=%ld next=%ld lead=%ld interval=%ld\n",
                        current_frame, bufreq->ubr_Frame, lead, interval);
    } else {
        ULONG current_frame;
        ULONG lead = interval * 8;
        LONG delta;

        if (lead < 8)
            lead = 8;

        uhciUpdateFrameCounter(hc);
        current_frame = hc->hc_FrameCounter;
        delta = (LONG)bufreq->ubr_Frame - (LONG)current_frame;
        if (delta <= 0 || delta >= (LONG)(UHCI_FRAMELIST_SIZE - lead)) {
            bufreq->ubr_Frame = current_frame + lead;
            pciusbUHCIDebug("UHCI", "ISO schedule adjusted current=%ld next=%ld lead=%ld interval=%ld\n",
                            current_frame, bufreq->ubr_Frame, lead, interval);
        } else {
            pciusbUHCIDebug("UHCI", "ISO schedule current=%ld interval=%ld\n",
                            bufreq->ubr_Frame, interval);
        }
    }
    if (bufreq->ubr_Frame < hc->hc_FrameCounter) {
        ULONG lead = interval * 8;

        if (lead < 8)
            lead = 8;

        bufreq->ubr_Frame = hc->hc_FrameCounter + lead;
        pciusbUHCIDebug("UHCI", "ISO schedule resync current=%ld next=%ld lead=%ld interval=%ld\n",
                        hc->hc_FrameCounter, bufreq->ubr_Frame, lead, interval);
    }
    rtn->rtn_NextFrame = bufreq->ubr_Frame + interval;

    ptd->ptd_FrameIdx = bufreq->ubr_Frame;
    ptd->ptd_Flags |= PTDF_BUFFER_VALID;
    ptdpriv->ptd_BounceBuffer = NULL;
    rtn->rtn_BufferReq = *bufreq;

    return RC_OK;
}

void uhciHandleIsochTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct RTIsoNode *rtn = (struct RTIsoNode *)hc->hc_RTIsoHandlers.mlh_Head;
    ULONG current_frame;

    uhciUpdateFrameCounter(hc);
    current_frame = hc->hc_FrameCounter;

    while(rtn->rtn_Node.mln_Succ) {
        struct RTIsoNode *next = (struct RTIsoNode *)rtn->rtn_Node.mln_Succ;
        struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
        UWORD idx;
        UWORD limit = rtn->rtn_PTDCount;

        if(!rtn->rtn_PTDs || !limit) {
            rtn = (struct RTIsoNode *) rtn->rtn_Node.mln_Succ;
            continue;
        }

        for(idx = 0; idx < limit; idx++) {
            struct PTDNode *ptd = rtn->rtn_PTDs[idx];
            struct UhciPTDPrivate *ptdpriv;
            struct UhciTD *utd;
            ULONG ctrl;
            ULONG actual;
            ULONG interval;
            BOOL error = FALSE;
            struct IOUsbHWReq *ioreq = pciusbIsoGetIOReq(rtn);

            if(!ptd || !(ptd->ptd_Flags & PTDF_ACTIVE))
                continue;

            ptdpriv = uhciPTDPrivate(ptd);
            utd = (struct UhciTD *)ptd->ptd_Descriptor;
            CacheClearE(utd, sizeof(*utd), CACRF_InvalidateD);
            ctrl = READMEM32_LE(&utd->utd_CtrlStatus);
            if(ctrl & UTCF_ACTIVE) {
                interval = rtn->rtn_IOReq.iouh_Interval ? rtn->rtn_IOReq.iouh_Interval : 1;
                ULONG timeout_window = interval * 4;

                if (timeout_window < 4)
                    timeout_window = 4;

                if (current_frame > (ptd->ptd_FrameIdx + timeout_window)) {
                    error = TRUE;
                    ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                    ioreq->iouh_Actual = 0;
                    ptdpriv->ptd_BufferReq.ubr_Length = 0;
                    ptdpriv->ptd_BufferReq.ubr_Frame = ptd->ptd_FrameIdx;
                    pciusbUHCIDebug("UHCI", "ISO TD timeout ptd=%p frame=%ld current=%ld window=%ld\n",
                                    ptd, ptd->ptd_FrameIdx, current_frame, timeout_window);
                    uhciUnlinkIsoPTD(hc, ptd);
                    if (urti) {
                        if (ioreq->iouh_Dir == UHDIR_IN) {
                            if (urti->urti_InDoneHook)
                                CallHookPkt(urti->urti_InDoneHook, rtn, &ptdpriv->ptd_BufferReq);
                        } else {
                            if (urti->urti_OutDoneHook)
                                CallHookPkt(urti->urti_OutDoneHook, rtn, &ptdpriv->ptd_BufferReq);
                        }
                    }
                    ptd->ptd_Flags &= ~(PTDF_ACTIVE|PTDF_BUFFER_VALID);
                    if (utd) {
                        uhciFreeTD(hc, utd);
                        ptd->ptd_Descriptor = NULL;
                    }
                    if (rtn->rtn_StdReq) {
                        UWORD devadrep = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint +
                                         ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                        Remove((struct Node *)&rtn->rtn_Node);
                        unit->hu_DevBusyReq[devadrep] = NULL;
                        ReplyMsg(&ioreq->iouh_Req.io_Message);
                        uhciFreeIsochIO(hc, rtn);
                        pciusbFreeStdIsoNode(hc, rtn);
                    } else if (urti) {
                        UWORD pending = 0;
                        UWORD target = (rtn->rtn_PTDCount > 1) ? 2 : 1;

                        for (UWORD scan = 0; scan < rtn->rtn_PTDCount; scan++) {
                            struct PTDNode *ptdscan = rtn->rtn_PTDs[scan];
                            if (ptdscan && (ptdscan->ptd_Flags & (PTDF_ACTIVE | PTDF_BUFFER_VALID)))
                                pending++;
                        }

                        while (pending < target) {
                            if (uhciQueueIsochIO(hc, rtn) != RC_OK)
                                break;
                            uhciStartIsochIO(hc, rtn);
                            pending++;
                        }
                    }
                    continue;
                }
                if (current_frame > ptd->ptd_FrameIdx) {
                    pciusbUHCIDebug("UHCI", "ISO TD still active ptd=%p frame=%ld current=%ld ctrl=%08lx\n",
                                    ptd, ptd->ptd_FrameIdx, current_frame, ctrl);
                }
                continue;
            }

            if(ctrl & (UTSF_BITSTUFFERR|UTSF_CRCTIMEOUT|UTSF_BABBLE|UTSF_DATABUFFERERR))
                error = TRUE;

            actual = (ctrl & UTSM_ACTUALLENGTH) >> UTSS_ACTUALLENGTH;
            actual = (actual == 0x7ff) ? 0 : actual + 1;
            ioreq->iouh_Actual = actual;
            ioreq->iouh_Req.io_Error = error ? UHIOERR_HOSTERROR : UHIOERR_NO_ERROR;
            ptdpriv->ptd_BufferReq.ubr_Length = actual;
            ptdpriv->ptd_BufferReq.ubr_Frame = ptd->ptd_FrameIdx;

            if(ptdpriv->ptd_BounceBuffer &&
                    ptdpriv->ptd_BounceBuffer != ptdpriv->ptd_BufferReq.ubr_Buffer) {
                usbReleaseBuffer(ptdpriv->ptd_BounceBuffer, ptdpriv->ptd_BufferReq.ubr_Buffer,
                                 ptdpriv->ptd_BufferReq.ubr_Length, ioreq->iouh_Dir);
                ptdpriv->ptd_BounceBuffer = NULL;
            }

            uhciUnlinkIsoPTD(hc, ptd);

            if(urti) {
                if(ioreq->iouh_Dir == UHDIR_IN) {
                    if(urti->urti_InDoneHook)
                        CallHookPkt(urti->urti_InDoneHook, rtn, &ptdpriv->ptd_BufferReq);
                } else {
                    if(urti->urti_OutDoneHook)
                        CallHookPkt(urti->urti_OutDoneHook, rtn, &ptdpriv->ptd_BufferReq);
                }
            }

            ptd->ptd_Flags &= ~(PTDF_ACTIVE|PTDF_BUFFER_VALID);
            uhciFreeTD(hc, utd);
            ptd->ptd_Descriptor = NULL;

            if (rtn->rtn_StdReq) {
                UWORD devadrep = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint +
                                 ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                Remove((struct Node *)&rtn->rtn_Node);
                unit->hu_DevBusyReq[devadrep] = NULL;
                ioreq->iouh_Actual = actual;
                ReplyMsg(&ioreq->iouh_Req.io_Message);
                uhciFreeIsochIO(hc, rtn);
                pciusbFreeStdIsoNode(hc, rtn);
            } else if (urti) {
                UWORD pending = 0;
                UWORD target = (rtn->rtn_PTDCount > 1) ? 2 : 1;

                for (UWORD scan = 0; scan < rtn->rtn_PTDCount; scan++) {
                    struct PTDNode *ptdscan = rtn->rtn_PTDs[scan];
                    if (ptdscan && (ptdscan->ptd_Flags & (PTDF_ACTIVE | PTDF_BUFFER_VALID)))
                        pending++;
                }

                while (pending < target) {
                    if (uhciQueueIsochIO(hc, rtn) != RC_OK)
                        break;
                    uhciStartIsochIO(hc, rtn);
                    pending++;
                }
            }
        }

        rtn = next;
    }
}

void uhciStartIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    UWORD idx;
    UWORD limit = rtn->rtn_PTDCount;

    pciusbUHCIDebug("UHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || !limit)
        return;

    for(idx = 0; idx < limit; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        struct UhciPTDPrivate *ptdpriv;
        struct UhciTD *utd;
        ULONG ctrlstatus;
        ULONG token;
        ULONG slot;
        ULONG phys;
        ULONG len;
        APTR dmabuffer;

        if(!ptd)
            continue;

        ptdpriv = uhciPTDPrivate(ptd);
        if(!(ptd->ptd_Flags & PTDF_BUFFER_VALID) || (ptd->ptd_Flags & PTDF_ACTIVE))
            continue;

        if(!ptd->ptd_Descriptor) {
            ptd->ptd_Descriptor = uhciAllocTD(hc);
            if(!ptd->ptd_Descriptor)
                continue;
            ptd->ptd_Phys = READMEM32_LE(&((struct UhciTD *)ptd->ptd_Descriptor)->utd_Self);
        }

        dmabuffer = ptdpriv->ptd_BufferReq.ubr_Buffer;
#if __WORDSIZE == 64
        if(dmabuffer) {
            dmabuffer = usbGetBuffer(dmabuffer, ptdpriv->ptd_BufferReq.ubr_Length,
                                     rtn->rtn_IOReq.iouh_Dir);
            if(!dmabuffer)
                continue;

            if(dmabuffer != ptdpriv->ptd_BufferReq.ubr_Buffer &&
                    rtn->rtn_IOReq.iouh_Dir == UHDIR_OUT) {
                CopyMem(ptdpriv->ptd_BufferReq.ubr_Buffer, dmabuffer, ptdpriv->ptd_BufferReq.ubr_Length);
            }
        }
#endif
        ptdpriv->ptd_BounceBuffer = (dmabuffer != ptdpriv->ptd_BufferReq.ubr_Buffer) ? dmabuffer : NULL;

        len = ptdpriv->ptd_BufferReq.ubr_Length;
        if(len > UHCI_ISO_MAXPKTSIZE)
            len = UHCI_ISO_MAXPKTSIZE;
        if(!len)
            phys = 0;
        if(len && !dmabuffer) {
            ptd->ptd_Flags &= ~PTDF_BUFFER_VALID;
            continue;
        }

        utd = (struct UhciTD *)ptd->ptd_Descriptor;
        slot = ptdpriv->ptd_BufferReq.ubr_Frame & (UHCI_FRAMELIST_SIZE - 1);
        if(len)
            phys = (ULONG)(IPTR)pciGetPhysical(hc, dmabuffer);

        ctrlstatus = UTCF_ACTIVE | UTCF_ISOCHRONOUS | UTCF_READYINTEN;
        token = (rtn->rtn_IOReq.iouh_DevAddr << UTTS_DEVADDR) |
                (rtn->rtn_IOReq.iouh_Endpoint << UTTS_ENDPOINT) |
                (((len ? (len - 1) : 0x7ff) & 0x7ff) << UTTS_TRANSLENGTH) |
                ((rtn->rtn_IOReq.iouh_Dir == UHDIR_IN) ? (PID_IN << UTTS_PID) : (PID_OUT << UTTS_PID));

        WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
        WRITEMEM32_LE(&utd->utd_Token, token);
        WRITEMEM32_LE(&utd->utd_BufferPtr, phys);

        ptd->ptd_FrameIdx = ptdpriv->ptd_BufferReq.ubr_Frame;
        ptd->ptd_Length = len;
        ptd->ptd_Flags |= PTDF_ACTIVE;
        ptd->ptd_NextPTD = NULL;

        pciusbUHCIDebug("UHCI", "ISO TD ptd=%p utd=%p frame=%ld slot=%ld len=%ld\n",
                        ptd, utd, ptd->ptd_FrameIdx, slot, len);
        uhciInsertIsoPTD(hc, ptd, slot);
    }
}

void uhciStopIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    UWORD idx;
    UWORD limit = rtn->rtn_PTDCount;

    pciusbUHCIDebug("UHCI", "%s()\n", __func__);

    if(!rtn->rtn_PTDs || !limit)
        return;

    for(idx = 0; idx < limit; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        struct UhciPTDPrivate *ptdpriv = ptd ? uhciPTDPrivate(ptd) : NULL;
        if(ptd && (ptd->ptd_Flags & PTDF_ACTIVE)) {
            uhciUnlinkIsoPTD(hc, ptd);
            ptd->ptd_Flags &= ~(PTDF_ACTIVE|PTDF_BUFFER_VALID);
        }
        if(ptdpriv && ptdpriv->ptd_BounceBuffer &&
                ptdpriv->ptd_BounceBuffer != ptdpriv->ptd_BufferReq.ubr_Buffer) {
            usbReleaseBuffer(ptdpriv->ptd_BounceBuffer, ptdpriv->ptd_BufferReq.ubr_Buffer,
                             ptdpriv->ptd_BufferReq.ubr_Length, rtn->rtn_IOReq.iouh_Dir);
            ptdpriv->ptd_BounceBuffer = NULL;
        }
    }
}

void uhciFreeIsochIO(struct PCIController *hc, struct RTIsoNode *rtn)
{
    UWORD idx;
    UWORD limit = rtn->rtn_PTDCount;

    pciusbUHCIDebug("UHCI", "%s()\n", __func__);

    uhciStopIsochIO(hc, rtn);

    if(!rtn->rtn_PTDs)
        return;

    for(idx = 0; idx < limit; idx++) {
        struct PTDNode *ptd = rtn->rtn_PTDs[idx];
        if(ptd) {
            if(ptd->ptd_Descriptor)
                uhciFreeTD(hc, (struct UhciTD *)ptd->ptd_Descriptor);
            if(ptd->ptd_Chipset)
                FreeMem(ptd->ptd_Chipset, sizeof(struct UhciPTDPrivate));
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
