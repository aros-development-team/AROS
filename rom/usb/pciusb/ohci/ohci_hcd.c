/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <hidd/pci.h>
#include <devices/usb_hub.h>

#include <stddef.h>

#include "uhwcmd.h"
#include "ohciproto.h"

#ifdef base
#undef base
#endif
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

#define NewList NEWLIST

static inline struct OhciPTDPrivate *ohciPTDPrivate(struct PTDNode *ptd)
{
    return (struct OhciPTDPrivate *)ptd->ptd_Chipset;
}

#ifdef DEBUG_TD

static void PrintTD(const char *txt, ULONG ptd, struct PCIController *hc)
{
    KPrintF("HC 0x%p %s TD list:", hc, txt);

    while (ptd) {
        struct OhciTD *otd = (struct OhciTD *)((IPTR)ptd - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));

        KPrintF(" 0x%p", otd);
        ptd = READMEM32_LE(&otd->otd_NextTD);
    }
    RawPutChar('\n');
}

#else
#define PrintTD(txt, ptd, hc)
#endif

#ifdef DEBUG_ED

static void PrintED(const char *txt, struct OhciED *oed, struct PCIController *hc)
{
    struct OhciTD *otd;
    struct OhciIsoTD *oitd;

    KPrintF("%s ED 0x%p: EPCaps=%08lx, HeadPtr=%08lx, TailPtr=%08lx, NextED=%08lx\n", txt, oed,
            READMEM32_LE(&oed->oed_EPCaps),
            READMEM32_LE(&oed->oed_HeadPtr),
            READMEM32_LE(&oed->oed_TailPtr),
            READMEM32_LE(&oed->oed_NextED));

    KPrintF("...TD list:", hc, txt, oed);
    for (otd = oed->oed_FirstTD; otd; otd = otd->otd_Succ)
        KPrintF(" 0x%p", otd);
    RawPutChar('\n');
}

#else
#define PrintED(txt, oed, hc)
#endif


/*
 * Prepare a DMA buffer for the OHCI controller.
 *
 * CachePreDMA/CachePostDMA operate on CPU-visible addresses. OHCI TD fields
 * must contain the bus (PCI) address. Do not mix these address domains.
 */
static inline ULONG ohciPrepareDMABuffer(struct PCIController *hc, struct OhciTD *otd,
                                        APTR cpu_buf, ULONG len, ULONG dma_flags)
{
    ULONG l = len;

    if (l == 0) {
        CONSTWRITEMEM32_LE(&otd->otd_BufferPtr, 0);
        CONSTWRITEMEM32_LE(&otd->otd_BufferEnd, 0);
        return 0;
    }

    APTR dma_buf = CachePreDMA(cpu_buf, &l, dma_flags);
    ULONG phys = (ULONG)(IPTR)pciGetPhysical(hc, dma_buf);

    WRITEMEM32_LE(&otd->otd_BufferPtr, phys);
    WRITEMEM32_LE(&otd->otd_BufferEnd, phys + l - 1);

    return l;
}



/*
 * Isochronous PSW helpers.
 *
 * OHCI ISO PSW Offset field contains the offset of the last byte of each packet
 * relative to BufferPage0, modulo 4 KiB. When the offset wraps, the packet end
 * has crossed into the next page.
 */
static inline UWORD ohciIsoTDStartOffset(struct OhciIsoTD *oitd)
{
    ULONG bufend = READMEM32_LE(&oitd->oitd_BufferEnd);
    ULONG len = (ULONG)oitd->oitd_Length;
    ULONG start = bufend - len + 1;
    return (UWORD)(start & 0xfff);
}

static inline ULONG ohciIsoPSWPktLen(UWORD raw_end, UWORD *prev_raw, ULONG *page_add, LONG *prev_end)
{
    if(raw_end < *prev_raw)
        *page_add += 0x1000;

    ULONG end = (ULONG)raw_end + *page_add;
    ULONG pktlen = end - (ULONG)(*prev_end);

    *prev_raw = raw_end;
    *prev_end = (LONG)end;

    return pktlen;
}
static AROS_INTH1(OhciResetHandler, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    // reset controller
    CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, OCSF_HCRESET);

    return FALSE;

    AROS_INTFUNC_EXIT
}

static void ohciFreeTDChain(struct PCIController *hc, struct OhciTD *nextotd)
{
    struct PCIDevice *base = hc->hc_Device;
    struct OhciTD *otd;

    while (nextotd) {
        pciusbOHCIDebug("OHCI", "FreeTD %p\n", nextotd);
        otd = nextotd;
        nextotd = (struct OhciTD *) otd->otd_Succ;
        ohciFreeTD(hc, otd);
    }
}

static void ohciFreeEDContext(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    struct PCIDevice *base = hc->hc_Device;
    struct PCIUnit *unit = hc->hc_Unit;
    struct OhciED *oed = ioreq->iouh_DriverPrivate1;
    UWORD devadrep;
    UWORD dir;

    pciusbOHCIDebug("OHCI", "Freeing EDContext 0x%p IOReq 0x%p\n", oed, ioreq);

    if (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
        dir = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT;
    else
        dir = ioreq->iouh_Dir;

    usbReleaseBuffer(oed->oed_Buffer, ioreq->iouh_Data, ioreq->iouh_Actual, dir);
    usbReleaseBuffer(oed->oed_SetupData, &ioreq->iouh_SetupData, 8, UHDIR_IN);

    devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
    unit->hu_DevBusyReq[devadrep] = NULL;
    unit->hu_DevDataToggle[devadrep] = (READMEM32_LE(&oed->oed_HeadPtr) & OEHF_DATA1) ? TRUE : FALSE;

    Disable();
    ohciFreeTDChain(hc, oed->oed_FirstTD);
    ohciFreeED(hc, oed);
    Enable();
}

void ohciUpdateIntTree(struct PCIController *hc)
{
    struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
    struct OhciED *oed;
    struct OhciED *predoed;
    struct OhciED *lastusedoed;
    UWORD cnt;

    // optimize linkage between queue heads
    predoed = lastusedoed = ohcihcp->ohc_OhciTermED;
    for(cnt = 0; cnt < 5; cnt++) {
        oed = ohcihcp->ohc_OhciIntED[cnt];
        if(oed->oed_Succ != predoed) {
            lastusedoed = oed->oed_Succ;
        }
        oed->oed_NextED = lastusedoed->oed_Self;
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);
        predoed = oed;
    }
}

static void ohciHandleFinishedTDs(struct PCIController *hc)
{
    struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
    struct PCIDevice *base = hc->hc_Device;
    struct IOUsbHWReq *ioreq;
    struct IOUsbHWReq *nextioreq;
    struct OhciED *oed = NULL;
    struct OhciTD *otd;
    ULONG len;
    ULONG ctrlstatus;
    ULONG epcaps;
    BOOL direction_in;
    BOOL updatetree = FALSE;
    ULONG donehead, nexttd;
    BOOL retire;

    pciusbOHCIDebug("OHCI", "Checking for work done...\n");
    Disable();
    donehead = ohcihcp->ohc_OhciDoneQueue;
    ohcihcp->ohc_OhciDoneQueue = 0UL;
    Enable();
    if(!donehead) {
        pciusbOHCIDebug("OHCI", "Nothing to do!\n");
        return;
    }
    otd = (struct OhciTD *) ((IPTR)donehead - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
    pciusbOHCIDebug("OHCI", "DoneHead=%08lx, OTD=%p, Frame=%ld\n", donehead, otd, READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT));
    PrintTD("Done", donehead, hc); /* CHECKME: This can give inconsistent printout on cache-incoherent hardware */
    do {
        CacheClearE(&otd->otd_Ctrl, 16, CACRF_InvalidateD);
        oed = otd->otd_ED;
        if(!oed) {
            /*
             * WATCH OUT!!! Rogue TD is a very bad thing!!!
             * If you see this, there's definitely a bug in DoneQueue processing flow.
             * See below for the complete description.
             */
            pciusbWarn("OHCI", "Came across a rogue TD 0x%p that already has been freed!\n", otd);
            nexttd = READMEM32_LE(&otd->otd_NextTD) & OHCI_PTRMASK;
            if(!nexttd) {
                break;
            }
            otd = (struct OhciTD *) ((IPTR)nexttd - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
            continue;
        }
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_InvalidateD);
        ctrlstatus = READMEM32_LE(&otd->otd_Ctrl);
        pciusbOHCIDebug("OHCI", "TD: %08lx - %08lx\n", READMEM32_LE(&otd->otd_BufferPtr),
                    READMEM32_LE(&otd->otd_BufferEnd));
        if(READMEM32_LE(&oed->oed_EPCaps) & OECF_ISO) {
            struct OhciIsoTD *oitd = (struct OhciIsoTD *)otd;
            UWORD pktcount;
            UWORD pktidx;
            UWORD base_off;
            UWORD prev_raw;
            ULONG page_add;
            LONG prev_end;

            /*
             * HC writes ISO PSWs beyond the first 16 bytes of the common TD header.
             * Invalidate the PSW area before reading it on cache-incoherent systems.
             */
            CacheClearE(&oitd->oitd_Ctrl, 16 + sizeof(oitd->oitd_Offset), CACRF_InvalidateD);

            pktcount = ((READMEM32_LE(&oitd->oitd_Ctrl) >> OITCS_FRAMECOUNT) & 0x7) + 1;

            len = 0;
            base_off = ohciIsoTDStartOffset(oitd);
            prev_raw = base_off;
            page_add = 0;
            prev_end = (LONG)base_off - 1;

            for(pktidx = 0; pktidx < pktcount; pktidx++) {
                UWORD psw = (UWORD)(READMEM32_LE(&oitd->oitd_Offset[pktidx]) & 0xffff);
                UWORD ccfield = psw & OITM_PSW_CC;
                UWORD pswcc;
                ULONG pktlen;

                if(ccfield == OITM_PSW_CC)
                    continue; /* Not accessed / unused */

                pswcc = ccfield >> OITS_PSW_CC;
                pktlen = ohciIsoPSWPktLen((UWORD)(psw & OITM_PSW_OFFSET), &prev_raw, &page_add, &prev_end);

                switch(pswcc << OTCS_COMPLETIONCODE) {
                case OTCF_CC_NOERROR:
                case OTCF_CC_SHORTPKT:
                    len += pktlen;
                    break;
                default:
                    break;
                }
            }
        } else if(otd->otd_BufferPtr) {
            // FIXME this will blow up if physical memory is ever going to be discontinuous
            len = READMEM32_LE(&otd->otd_BufferPtr) - (READMEM32_LE(&otd->otd_BufferEnd) + 1 - otd->otd_Length);
        } else {
            len = otd->otd_Length;
        }

        ioreq = oed->oed_IOReq;
        struct RTIsoNode *rtn = ioreq ? (struct RTIsoNode *)ioreq->iouh_DriverPrivate2 : NULL;
        BOOL rtiso = rtn && rtn->rtn_RTIso;

        pciusbOHCIDebug("OHCI", "Examining TD %p for ED %p (IOReq=%p), Status %08lx, len=%ld\n", otd, oed, ioreq, ctrlstatus, len);
        if(!ioreq) {
            /* You should never see this (very weird inconsistency), but who knows... */
            pciusbWarn("OHCI", "Came across a rogue ED 0x%p that already has been replied! TD 0x%p,\n", oed, otd);
            nexttd = READMEM32_LE(&otd->otd_NextTD) & OHCI_PTRMASK;
            if(!nexttd) {
                break;
            }
            otd = (struct OhciTD *) ((IPTR)nexttd - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
            continue;
        }

        if((READMEM32_LE(&oed->oed_EPCaps) & OECF_ISO) == 0) {
            /*
             * Cache maintenance must operate on CPU-visible addresses.
             * We defer CachePostDMA() until the IOReq is retired so we can use
             * oed->oed_Buffer (the CPU pointer / bounce buffer), rather than
             * deriving an address from OHCI TD physical fields.
             */
            ioreq->iouh_Actual += len;
        }
        /*
         * CHECKME: This condition may get triggered on control transfers even if terminating TD is not processed yet.
         *          (got triggered by MacMini's keyboard, when someone sends control ED with no data payload,
         *          and some other ED is being done meanwhile (its final packet generated an interrupt).
         *          In this case the given control ED can be partially done (setup TD is done, term TD is not).
         *          iouh_Length is 0, and the whole ED is retired, while still being processed by the HC. Next time
         *          its terminator TD arrives into done queue.
         *          This can cause weird things like looping TD list on itself. My modification of ohciFreeTD()
         *          explicitly clears NextTD to avoid keeping dangling value there, however the problem still can
         *          appear if this TD is quickly reused by another request.
         *          Final TDs have OTCM_DELAYINT fields set to zero. HC processes TDs in order, so if we receive
         *          the final TD, we assume the whole ED's list has been processed.
         *          This means it should be safe to simply disable this check.
         *          If this doesn't work for some reason, we need a more complex check which makes sure that all TDs
         *          are really done (or ED is halted). This can be done by checking OTCM_COMPLETIONCODE field against
         *          OTCF_CC_INVALID value.
         *                                              Pavel Fedin <pavel.fedin@mail.ru>
                retire = (ioreq->iouh_Actual == ioreq->iouh_Length);
                if (retire)
                {
                    pciusbOHCIDebug("OHCI", ("TD 0x%p Data transfer done (%lu bytes)\n", otd, ioreq->iouh_Length));
                } */
        retire = FALSE;
        if((ctrlstatus & OTCM_DELAYINT) != OTCF_NOINT) {
            pciusbOHCIDebug("OHCI", "TD 0x%p Terminator detected\n", otd);
            retire = TRUE;
        }
        if(READMEM32_LE(&oed->oed_EPCaps) & OECF_ISO) {
            struct OhciIsoTD *oitd = (struct OhciIsoTD *)otd;
            struct PTDNode *ptd = NULL;
            struct IOUsbHWBufferReq *bufreq = rtn ? &rtn->rtn_BufferReq : NULL;
            UWORD pktcount;
            UWORD pktidx;
            UWORD base_off;
            UWORD prev_raw;
            ULONG page_add;
            LONG prev_end;
            ULONG frame;

            /* Invalidate ISO PSWs written by the HC */
            CacheClearE(&oitd->oitd_Ctrl, 16 + sizeof(oitd->oitd_Offset), CACRF_InvalidateD);

            pktcount = ((READMEM32_LE(&oitd->oitd_Ctrl) >> OITCS_FRAMECOUNT) & 0x7) + 1;
            base_off = ohciIsoTDStartOffset(oitd);
            prev_raw = base_off;
            page_add = 0;
            prev_end = (LONG)base_off - 1;
            frame = (READMEM32_LE(&oitd->oitd_Ctrl) >> OITCS_STARTINGFRAME) & 0xffff;

            if (rtn && rtn->rtn_PTDs) {
                for (UWORD idx = 0; idx < rtn->rtn_PTDCount; idx++) {
                    struct PTDNode *scan = rtn->rtn_PTDs[idx];
                    if (scan && scan->ptd_Descriptor == oitd) {
                        ptd = scan;
                        bufreq = &ohciPTDPrivate(scan)->ptd_BufferReq;
                        break;
                    }
                }
            }

            if (bufreq) {
                bufreq->ubr_Frame = frame;
                bufreq->ubr_Length = 0;
            }

            ioreq->iouh_Actual = 0;

            for(pktidx = 0; pktidx < pktcount; pktidx++) {
                UWORD psw = (UWORD)(READMEM32_LE(&oitd->oitd_Offset[pktidx]) & 0xffff);
                UWORD ccfield = psw & OITM_PSW_CC;
                UWORD pswcc;
                ULONG pktlen;

                if(ccfield == OITM_PSW_CC)
                    continue; /* Not accessed / unused */

                pswcc = ccfield >> OITS_PSW_CC;
                pktlen = ohciIsoPSWPktLen((UWORD)(psw & OITM_PSW_OFFSET), &prev_raw, &page_add, &prev_end);

                switch(pswcc << OTCS_COMPLETIONCODE) {
                case OTCF_CC_NOERROR:
                    ioreq->iouh_Actual += pktlen;
                    break;

                case OTCF_CC_CRCERROR:
                case OTCF_CC_BABBLE:
                case OTCF_CC_PIDCORRUPT:
                case OTCF_CC_WRONGPID:
                    ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                    retire = TRUE;
                    break;

                case OTCF_CC_TIMEOUT:
                    ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                    retire = TRUE;
                    break;

                case OTCF_CC_OVERFLOW:
                    ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                    retire = TRUE;
                    break;

                case OTCF_CC_SHORTPKT:
                    ioreq->iouh_Actual += pktlen;
                    if((!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS)))
                        ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                    retire = TRUE;
                    break;

                case OTCF_CC_UNDERRUN:
                case OTCF_CC_OVERRUN:
                    ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                    retire = TRUE;
                    break;

                default:
                    break;
                }
            }
            retire = TRUE;
            if (bufreq)
                bufreq->ubr_Length = ioreq->iouh_Actual;

            if (ptd) {
                struct OhciPTDPrivate *ptdpriv = ohciPTDPrivate(ptd);
                if (ptdpriv->ptd_BounceBuffer &&
                        ptdpriv->ptd_BounceBuffer != ptdpriv->ptd_BufferReq.ubr_Buffer) {
                    usbReleaseBuffer(ptdpriv->ptd_BounceBuffer, ptdpriv->ptd_BufferReq.ubr_Buffer,
                                     ptdpriv->ptd_BufferReq.ubr_Length, ioreq->iouh_Dir);
                    ptdpriv->ptd_BounceBuffer = NULL;
                }
            }

            if (rtiso && rtn && rtn->rtn_RTIso) {
                struct IOUsbHWRTIso *urti = rtn->rtn_RTIso;
                struct IOUsbHWBufferReq *donebuf = bufreq ? bufreq : &rtn->rtn_BufferReq;

                if (ioreq->iouh_Dir == UHDIR_IN) {
                    if (urti->urti_InDoneHook)
                        CallHookPkt(urti->urti_InDoneHook, rtn, donebuf);
                } else {
                    if (urti->urti_OutDoneHook)
                        CallHookPkt(urti->urti_OutDoneHook, rtn, donebuf);
                }

                if (ptd)
                    ptd->ptd_Flags &= ~(PTDF_ACTIVE|PTDF_BUFFER_VALID);

                ioreq->iouh_Actual = 0;
                ioreq->iouh_Req.io_Error = 0;

                {
                    UWORD pending = 0;
                    UWORD target = (rtn->rtn_PTDCount > 1) ? 2 : 1;

                    for (UWORD scan = 0; scan < rtn->rtn_PTDCount; scan++) {
                        struct PTDNode *ptdscan = rtn->rtn_PTDs[scan];
                        if (ptdscan && (ptdscan->ptd_Flags & (PTDF_ACTIVE | PTDF_BUFFER_VALID)))
                            pending++;
                    }

                    while (pending < target) {
                        if (ohciQueueIsochIO(hc, rtn) != RC_OK)
                            break;
                        ohciStartIsochIO(hc, rtn);
                        pending++;
                    }
                }

                retire = FALSE;
            }
        } else switch((ctrlstatus & OTCM_COMPLETIONCODE)>>OTCS_COMPLETIONCODE) {
            case (OTCF_CC_NOERROR>>OTCS_COMPLETIONCODE):
                break;

            case (OTCF_CC_CRCERROR>>OTCS_COMPLETIONCODE):
                pciusbError("OHCI", "CRC Error!\n");
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                /*
                 * CHECKME: Do we really need to set retire flag here?
                 *          Critical errors are always accompanied by OEHF_HALTED bit.
                 *          But what if HC thinks it's recoverable error and continues
                 *          working on this ED? In this case early retirement happens,
                 *          causing bad things. See long explanation above.
                 */
                retire = TRUE;
                break;

            case (OTCF_CC_BABBLE>>OTCS_COMPLETIONCODE):
                pciusbError("OHCI", "Babble/Bitstuffing Error!\n");
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_WRONGTOGGLE>>OTCS_COMPLETIONCODE):
                pciusbError("OHCI", "Data toggle mismatch length = %ld\n", len);
                break;

            case (OTCF_CC_STALL>>OTCS_COMPLETIONCODE):
                pciusbError("OHCI", "STALLED!\n");
                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                retire = TRUE;
                break;

            case (OTCF_CC_TIMEOUT>>OTCS_COMPLETIONCODE):
                pciusbError("OHCI", "TIMEOUT!\n");
                ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                retire = TRUE;
                break;

            case (OTCF_CC_PIDCORRUPT>>OTCS_COMPLETIONCODE):
                pciusbError("OHCI", "PID Error!\n");
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_WRONGPID>>OTCS_COMPLETIONCODE):
                pciusbError("OHCI", "Illegal PID!\n");
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_OVERFLOW>>OTCS_COMPLETIONCODE):
                pciusbError("OHCI", "Overflow Error!\n");
                ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                retire = TRUE;
                break;

            case (OTCF_CC_SHORTPKT>>OTCS_COMPLETIONCODE):
                pciusbOHCIDebug("OHCI", "Short packet %ld < %ld\n", len, otd->otd_Length);
                if((!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS))) {
                    ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                }
                retire = TRUE;
                break;

            case (OTCF_CC_OVERRUN>>OTCS_COMPLETIONCODE):
                pciusbError("OHCI", "Data Overrun Error!\n");
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_UNDERRUN>>OTCS_COMPLETIONCODE):
                pciusbError("OHCI", "Data Underrun Error!\n");
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_INVALID>>OTCS_COMPLETIONCODE):
                pciusbOHCIDebugV("OHCI", "Not touched?!?\n");
                break;
            }
        if(READMEM32_LE(&oed->oed_HeadPtr) & OEHF_HALTED) {
            pciusbOHCIDebug("OHCI", "OED halted!\n");
            retire = TRUE;
        }

        if(retire) {
            pciusbOHCIDebug("OHCI", "ED 0x%p stopped at TD 0x%p\n", oed, otd);
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            AddHead(&ohcihcp->ohc_OhciRetireQueue, &ioreq->iouh_Req.io_Message.mn_Node);
        }

        nexttd = READMEM32_LE(&otd->otd_NextTD) & OHCI_PTRMASK;
        pciusbOHCIDebugV("OHCI", "NextTD=0x%08lx\n", nexttd);
        if(!nexttd) {
            break;
        }
        otd = (struct OhciTD *) ((IPTR)nexttd - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
        pciusbOHCIDebugV("OHCI", "NextOTD = %p\n", otd);
    } while(TRUE);

    ioreq = (struct IOUsbHWReq *) ohcihcp->ohc_OhciRetireQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ)) {
        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        oed = (struct OhciED *) ioreq->iouh_DriverPrivate1;
        if(oed) {
            pciusbOHCIDebug("OHCI", "HC 0x%p Retiring IOReq=0x%p Command=%ld ED=0x%p, Frame=%ld\n", hc, ioreq, ioreq->iouh_Req.io_Command, oed, READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT));

            if(oed->oed_Continue) {
                ULONG actual = ioreq->iouh_Actual;
                ULONG oldenables;
                struct OhciTD *predotd = NULL;

                pciusbOHCIDebug("OHCI", "Reloading Bulk transfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length);
                otd = oed->oed_FirstTD;
                do {
                    len = ioreq->iouh_Length - actual;
                    if(len > OHCI_PAGE_SIZE) {
                        len = OHCI_PAGE_SIZE;
                    }
                    if((!otd->otd_Succ) && (actual + len == ioreq->iouh_Length) && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) && ((actual % ioreq->iouh_MaxPktSize) == 0)) {
                        // special case -- zero padding would not fit in this run,
                        // and next time, we would forget about it. So rather abort
                        // reload now, so the zero padding goes with the next reload
                        break;
                    }
                    predotd = otd;
                    otd->otd_Length = len;
                    pciusbOHCIDebug("OHCI", "TD with %ld bytes\n", len);
                    CONSTWRITEMEM32_LE(&otd->otd_Ctrl, OTCF_CC_INVALID|OTCF_NOINT);
                    if(otd->otd_Succ) {
                        otd->otd_NextTD = otd->otd_Succ->otd_Self;
                    }
                    if(len) {
                        len = ohciPrepareDMABuffer(hc, otd,
                                                  (APTR)((UBYTE *)oed->oed_Buffer + actual),
                                                  len,
                                                  (ioreq->iouh_Dir == UHDIR_IN) ? 0 : DMA_ReadFromRAM);
                        otd->otd_Length = len;
                        pciusbOHCIDebug("OHCI", "TD send: %08lx - %08lx\n",
                                        READMEM32_LE(&otd->otd_BufferPtr),
                                        READMEM32_LE(&otd->otd_BufferEnd));
                    } else {
                        CONSTWRITEMEM32_LE(&otd->otd_BufferPtr, 0);
                        CONSTWRITEMEM32_LE(&otd->otd_BufferEnd, 0);
                    }
                    CacheClearE(&otd->otd_Ctrl, 16, CACRF_ClearD);
                    actual += len;
                    otd = otd->otd_Succ;
                } while(otd && ((actual < ioreq->iouh_Length) || (len && (ioreq->iouh_Dir == UHDIR_OUT) && (actual == ioreq->iouh_Length) && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) && ((actual % ioreq->iouh_MaxPktSize) == 0))));
                oed->oed_Continue = (actual < ioreq->iouh_Length);
                predotd->otd_NextTD = ohcihcp->ohc_OhciTermTD->otd_Self;

                CONSTWRITEMEM32_LE(&predotd->otd_Ctrl, OTCF_CC_INVALID);
                CacheClearE(&predotd->otd_Ctrl, 16, CACRF_ClearD);

                Disable();
                AddTail(&hc->hc_TDQueue, &ioreq->iouh_Req.io_Message.mn_Node);

                // keep toggle bit
                ctrlstatus = READMEM32_LE(&oed->oed_HeadPtr) & OEHF_DATA1;
                ctrlstatus |= READMEM32_LE(&oed->oed_FirstTD->otd_Self);
                WRITEMEM32_LE(&oed->oed_HeadPtr, ctrlstatus);
                CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);

                PrintED("Continued bulk", oed, hc);

                oldenables = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
                oldenables |= OCSF_BULKENABLE;
                WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, oldenables);
                SYNC;
                Enable();
            } else {
                // disable ED
                ohciDisableED(oed);
                PrintED("Completed", oed, hc);

                /* Final cache maintenance for completed non-isochronous transfers */
                if(((READMEM32_LE(&oed->oed_EPCaps) & OECF_ISO) == 0) && ioreq->iouh_Actual) {
                    BOOL direction_in;
                    ULONG post_len = (ULONG)ioreq->iouh_Actual;

                    if(ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                        direction_in = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? TRUE : FALSE;
                    else
                        direction_in = (ioreq->iouh_Dir == UHDIR_IN);

                    CachePostDMA(oed->oed_Buffer, &post_len, direction_in ? 0 : DMA_ReadFromRAM);
                }

                {
                    struct RTIsoNode *rtn = (struct RTIsoNode *)ioreq->iouh_DriverPrivate2;
                    BOOL stdiso = rtn && rtn->rtn_StdReq;
                    ohciFreeEDContext(hc, ioreq);
                    if (stdiso) {
                        rtn->rtn_IOReq.iouh_DriverPrivate1 = NULL;
                        ohciFreeIsochIO(hc, rtn);
                        Remove((struct Node *)&rtn->rtn_Node);
                        pciusbFreeStdIsoNode(hc, rtn);
                        ioreq->iouh_DriverPrivate2 = NULL;
                    }
                }
                if(ioreq->iouh_Req.io_Command == UHCMD_INTXFER) {
                    updatetree = TRUE;
                }
                // check for successful clear feature and set address ctrl transfers
                if((!ioreq->iouh_Req.io_Error) && (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)) {
                    uhwCheckSpecialCtrlTransfers(hc, ioreq);
                }
                ReplyMsg(&ioreq->iouh_Req.io_Message);
            }
        } else {
            pciusbOHCIDebug("OHCI", "IOReq=%p has no OED!\n", ioreq);
        }
        ioreq = nextioreq;
    }
    if(updatetree) {
        ohciUpdateIntTree(hc);
    }
}

static ULONG ohciHandleAbortedEDs(struct PCIController *hc)
{
    struct PCIDevice *base = hc->hc_Device;
    struct IOUsbHWReq *ioreq;
    ULONG restartmask = 0;

    pciusbOHCIDebug("OHCI", "Processing abort queue...\n");

    // We don't need this any more
    ohciDisableInt(hc, OISF_SOF);

    /*
     * If the aborted IORequest was replied in ohciHandleFinishedTDs(),
     * it was already Remove()d from this queue. It's safe to do no checks.
     * io_Error was set earlier.
     */
    while ((ioreq = (struct IOUsbHWReq *)RemHead(&hc->hc_AbortQueue))) {
        pciusbOHCIDebug("OHCI", "HC 0x%p Aborted IOReq 0x%p\n", hc, ioreq);
        PrintED("Aborted", ioreq->iouh_DriverPrivate1, hc);

        ohciFreeEDContext(hc, ioreq);
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }

    /* Restart stopped queues */
    if (hc->hc_Flags & HCF_STOP_CTRL) {
        pciusbOHCIDebug("OHCI", "Restarting control transfers\n");
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CTRL_ED, 0);
        restartmask |= OCSF_CTRLENABLE;
    }

    if (hc->hc_Flags & HCF_STOP_BULK) {
        pciusbOHCIDebug("OHCI", "Restarting bulk transfers\n");
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_ED, 0);
        restartmask |= OCSF_BULKENABLE;
    }

    /* Everything is enabled again, aborting done */
    hc->hc_Flags &= ~(HCF_STOP_CTRL | HCF_STOP_BULK | HCF_ABORT);

    /* We will accumulate flags and start queues only once, when everything is set up */
    return restartmask;
}

static ULONG ohciScheduleCtrlTDs(struct PCIController *hc)
{
    struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
    struct PCIDevice *base = hc->hc_Device;
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct OhciED *oed;
    struct OhciTD *setupotd;
    struct OhciTD *dataotd;
    struct OhciTD *termotd;
    struct OhciTD *predotd;
    ULONG actual;
    ULONG epcaps;
    ULONG ctrl;
    ULONG len;
    ULONG oldenables;
    ULONG startmask = 0;

    /* *** CTRL Transfers *** */
    pciusbOHCIDebug("OHCI", "Scheduling new CTRL transfers...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ) {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint;
        pciusbOHCIDebug("OHCI", "New CTRL transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("OHCI", "Endpoint %02lx in use!\n", devadrep);
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        oed = ohciAllocED(hc);
        if(!oed) {
            break;
        }

        setupotd = ohciAllocTD(hc);
        if(!setupotd) {
            ohciFreeED(hc, oed);
            break;
        }
        termotd = ohciAllocTD(hc);
        if(!termotd) {
            ohciFreeTD(hc, setupotd);
            ohciFreeED(hc, oed);
            break;
        }
        oed->oed_IOReq = ioreq;

        pciusbOHCIDebug("OHCI", "SetupTD=%p, TermTD=%p\n", setupotd, termotd);

        // fill setup td
        epcaps = (ioreq->iouh_DevAddr<<OECS_DEVADDR)|(ioreq->iouh_Endpoint<<OECS_ENDPOINT)|(ioreq->iouh_MaxPktSize<<OECS_MAXPKTLEN)|OECF_DIRECTION_TD;

        if(ioreq->iouh_Flags & UHFF_LOWSPEED) {
            pciusbOHCIDebug("OHCI", "*** LOW SPEED ***\n");
            epcaps |= OECF_LOWSPEED;
        }

        WRITEMEM32_LE(&oed->oed_EPCaps, epcaps);

        oed->oed_TailPtr = ohcihcp->ohc_OhciTermTD->otd_Self;
        oed->oed_HeadPtr = setupotd->otd_Self;
        oed->oed_FirstTD = setupotd;

        setupotd->otd_ED = oed;
        setupotd->otd_Length = 0; // don't increase io_Actual for that transfer
        CONSTWRITEMEM32_LE(&setupotd->otd_Ctrl, OTCF_PIDCODE_SETUP|OTCF_CC_INVALID|OTCF_NOINT);
        len = 8;

        /* CHECKME: As i can understand, setup packet is always sent TO the device. Is this true? */
        oed->oed_SetupData = usbGetBuffer(&ioreq->iouh_SetupData, len, UHDIR_OUT);
        ohciPrepareDMABuffer(hc, setupotd, oed->oed_SetupData, len, DMA_ReadFromRAM);

        pciusbOHCIDebug("OHCI", "TD send: %08lx - %08lx\n", READMEM32_LE(&setupotd->otd_BufferPtr),
                READMEM32_LE(&setupotd->otd_BufferEnd));

        ctrl = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? (OTCF_PIDCODE_IN|OTCF_CC_INVALID|OTCF_NOINT) : (OTCF_PIDCODE_OUT|OTCF_CC_INVALID|OTCF_NOINT);

        predotd = setupotd;
        if (ioreq->iouh_Length) {
            oed->oed_Buffer = usbGetBuffer(ioreq->iouh_Data, ioreq->iouh_Length, (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT);
            actual = 0;
            do {
                dataotd = ohciAllocTD(hc);
                if(!dataotd) {
                    predotd->otd_Succ = NULL;
                    break;
                }
                dataotd->otd_ED = oed;
                predotd->otd_Succ = dataotd;
                predotd->otd_NextTD = dataotd->otd_Self;
                len = ioreq->iouh_Length - actual;
                if(len > OHCI_PAGE_SIZE) {
                    len = OHCI_PAGE_SIZE;
                }
                dataotd->otd_Length = len;
                pciusbOHCIDebug("OHCI", "TD with %ld bytes\n", len);
                WRITEMEM32_LE(&dataotd->otd_Ctrl, ctrl);
                len = ohciPrepareDMABuffer(hc, dataotd,
                                          (APTR)((UBYTE *)oed->oed_Buffer + actual),
                                          len,
                                          (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? 0 : DMA_ReadFromRAM);
                dataotd->otd_Length = len;

                pciusbOHCIDebug("OHCI", "TD send: %08lx - %08lx\n", READMEM32_LE(&dataotd->otd_BufferPtr),
                        READMEM32_LE(&dataotd->otd_BufferEnd));

                CacheClearE(&dataotd->otd_Ctrl, 16, CACRF_ClearD);
                actual += len;
                predotd = dataotd;
            } while(actual < ioreq->iouh_Length);

            if(actual != ioreq->iouh_Length) {
                // out of TDs
                pciusbWarn("OHCI", "Out of TDs for Ctrl Transfer!\n");
                dataotd = setupotd->otd_Succ;
                ohciFreeTD(hc, setupotd);
                ohciFreeTDChain(hc, dataotd);
                ohciFreeTD(hc, termotd);
                usbReleaseBuffer(oed->oed_Buffer, ioreq->iouh_Data, 0, 0);
                usbReleaseBuffer(oed->oed_SetupData, &oed->oed_IOReq->iouh_SetupData, 0, 0);
                ohciFreeED(hc, oed);
                break;
            }
            predotd->otd_Succ = termotd;
            predotd->otd_NextTD = termotd->otd_Self;
        } else {
            setupotd->otd_Succ = termotd;
            setupotd->otd_NextTD = termotd->otd_Self;
        }
        CacheClearE(&setupotd->otd_Ctrl, 16, CACRF_ClearD);
        CacheClearE(&predotd->otd_Ctrl, 16, CACRF_ClearD);

        /* Status stage: opposite direction, DATA1, interrupt on completion */
        ULONG term_ctrl = ((ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? OTCF_PIDCODE_OUT : OTCF_PIDCODE_IN) |
                          OTCF_CC_INVALID | OTCF_DATA1 | OTCF_TOGGLEFROMTD;

        termotd->otd_Length = 0;
        termotd->otd_ED = oed;
        termotd->otd_Succ = NULL;
        termotd->otd_NextTD = ohcihcp->ohc_OhciTermTD->otd_Self;
        CONSTWRITEMEM32_LE(&termotd->otd_Ctrl, term_ctrl);
        CONSTWRITEMEM32_LE(&termotd->otd_BufferPtr, 0);
        CONSTWRITEMEM32_LE(&termotd->otd_BufferEnd, 0);
        CacheClearE(&termotd->otd_Ctrl, 16, CACRF_ClearD);

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = oed;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry
        oed->oed_Succ = ohcihcp->ohc_OhciCtrlTailED;
        oed->oed_NextED = oed->oed_Succ->oed_Self;
        oed->oed_Pred = ohcihcp->ohc_OhciCtrlTailED->oed_Pred;
        CacheClearE(&oed->oed_Pred->oed_EPCaps, 16, CACRF_InvalidateD);
        oed->oed_Pred->oed_Succ = oed;
        oed->oed_Pred->oed_NextED = oed->oed_Self;
        oed->oed_Succ->oed_Pred = oed;
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);
        CacheClearE(&oed->oed_Pred->oed_EPCaps, 16, CACRF_ClearD);
        SYNC;

        PrintED("Control", oed, hc);

        /* Control request is queued, we will start the queue */
        startmask =  OCSF_CTRLENABLE;
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    }

    if (startmask) {
        /*
         * If we are going to start the queue but it's not running yet,
         * reset current ED pointer to zero. This will cause the HC to
         * start over from the head.
         */
        oldenables = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
        if(!(oldenables & OCSF_BULKENABLE)) {
            CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_ED, 0);
        }
    }

    return startmask;
}

static void ohciScheduleIntTDs(struct PCIController *hc)
{
    struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
#ifndef base
    struct PCIDevice *base = hc->hc_Device;
#endif
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct OhciED *intoed;
    struct OhciED *oed;
    struct OhciTD *otd;
    struct OhciTD *predotd;
    ULONG actual;
    ULONG epcaps;
    ULONG len;
    /* *** INT Transfers *** */
    pciusbOHCIDebug("OHCI", "Scheduling new INT transfers...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ) {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        pciusbOHCIDebug("OHCI", "New INT transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("OHCI", "Endpoint %02lx in use!\n", devadrep);
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        oed = ohciAllocED(hc);
        if(!oed) {
            break;
        }

        oed->oed_IOReq = ioreq;

        epcaps = (ioreq->iouh_DevAddr<<OECS_DEVADDR)|(ioreq->iouh_Endpoint<<OECS_ENDPOINT)|(ioreq->iouh_MaxPktSize<<OECS_MAXPKTLEN);
        epcaps |= (ioreq->iouh_Dir == UHDIR_IN) ? OECF_DIRECTION_IN : OECF_DIRECTION_OUT;

        if(ioreq->iouh_Flags & UHFF_LOWSPEED) {
            pciusbOHCIDebug("OHCI", "*** LOW SPEED ***\n");
            epcaps |= OECF_LOWSPEED;
        }

        WRITEMEM32_LE(&oed->oed_EPCaps, epcaps);
        oed->oed_TailPtr = ohcihcp->ohc_OhciTermTD->otd_Self;

        predotd = NULL;
        oed->oed_Buffer = usbGetBuffer(ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_Dir);
        actual = 0;
        do {
            otd = ohciAllocTD(hc);
            if (predotd)
                predotd->otd_Succ = otd;
            if (!otd) {
                break;
            }
            otd->otd_ED = oed;
            if (predotd) {
                predotd->otd_NextTD = otd->otd_Self;
            } else {
                WRITEMEM32_LE(&oed->oed_HeadPtr, READMEM32_LE(&otd->otd_Self)|(unit->hu_DevDataToggle[devadrep] ? OEHF_DATA1 : 0));
                oed->oed_FirstTD = otd;
            }
            len = ioreq->iouh_Length - actual;
            if(len > OHCI_PAGE_SIZE) {
                len = OHCI_PAGE_SIZE;
            }
            otd->otd_Length = len;
            pciusbOHCIDebugV("OHCI", "Control TD 0x%p with %ld bytes\n", otd, len);
            CONSTWRITEMEM32_LE(&otd->otd_Ctrl, OTCF_CC_INVALID|OTCF_NOINT);
            if(len) {
                len = ohciPrepareDMABuffer(hc, otd,
                                          (APTR)((UBYTE *)oed->oed_Buffer + actual),
                                          len,
                                          (ioreq->iouh_Dir == UHDIR_IN) ? 0 : DMA_ReadFromRAM);
                otd->otd_Length = len;
                pciusbOHCIDebug("OHCI", "TD send: %08lx - %08lx\n",
                                READMEM32_LE(&otd->otd_BufferPtr),
                                READMEM32_LE(&otd->otd_BufferEnd));
            } else {
                CONSTWRITEMEM32_LE(&otd->otd_BufferPtr, 0);
                CONSTWRITEMEM32_LE(&otd->otd_BufferEnd, 0);
            }
            actual += len;
            CacheClearE(&otd->otd_Ctrl, 16, CACRF_ClearD);
            predotd = otd;
        } while(actual < ioreq->iouh_Length);

        if(actual != ioreq->iouh_Length) {
            // out of TDs
            pciusbWarn("OHCI", "Out of TDs for Int Transfer!\n");
            ohciFreeTDChain(hc, oed->oed_FirstTD);
            usbReleaseBuffer(oed->oed_Buffer, ioreq->iouh_Data, 0, 0);
            ohciFreeED(hc, oed);
            break;
        }
        predotd->otd_Succ = NULL;
        predotd->otd_NextTD = ohcihcp->ohc_OhciTermTD->otd_Self;

        CONSTWRITEMEM32_LE(&predotd->otd_Ctrl, OTCF_CC_INVALID);
        CacheClearE(&predotd->otd_Ctrl, 16, CACRF_ClearD);

        if(ioreq->iouh_Interval >= 31) {
            intoed = ohcihcp->ohc_OhciIntED[4]; // 32ms interval
        } else {
            UWORD cnt = 0;
            do {
                intoed = ohcihcp->ohc_OhciIntED[cnt++];
            } while(ioreq->iouh_Interval >= (1<<cnt));
        }

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = oed;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry (behind Int head)
        oed->oed_Succ = intoed->oed_Succ;
        oed->oed_NextED = intoed->oed_Succ->oed_Self;
        oed->oed_Pred = intoed;
        intoed->oed_Succ = oed;
        intoed->oed_NextED = oed->oed_Self;
        oed->oed_Succ->oed_Pred = oed;
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);
        CacheClearE(&intoed->oed_EPCaps, 16, CACRF_ClearD);
        SYNC;

        PrintED("Int", oed, hc);
        Enable();

        ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    }
}


static ULONG ohciScheduleBulkTDs(struct PCIController *hc)
{
    struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
#ifndef base
    struct PCIDevice *base = hc->hc_Device;
#endif
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct OhciED *oed;
    struct OhciTD *otd;
    struct OhciTD *predotd;
    ULONG actual;
    ULONG epcaps;
    ULONG len;
    ULONG oldenables;
    ULONG startmask = 0;

    /* *** BULK Transfers *** */
    pciusbOHCIDebug("OHCI", "Scheduling new BULK transfers...\n");
    ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ) {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        pciusbOHCIDebug("OHCI", "New BULK transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep]) {
            pciusbWarn("OHCI", "Endpoint %02lx in use!\n", devadrep);
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        oed = ohciAllocED(hc);
        if(!oed) {
            break;
        }

        oed->oed_IOReq = ioreq;

        epcaps = (ioreq->iouh_DevAddr<<OECS_DEVADDR)|(ioreq->iouh_Endpoint<<OECS_ENDPOINT)|(ioreq->iouh_MaxPktSize<<OECS_MAXPKTLEN);
        epcaps |= (ioreq->iouh_Dir == UHDIR_IN) ? OECF_DIRECTION_IN : OECF_DIRECTION_OUT;

        if(ioreq->iouh_Flags & UHFF_LOWSPEED) {
            pciusbOHCIDebug("OHCI", "*** LOW SPEED ***\n");
            epcaps |= OECF_LOWSPEED;
        }

        WRITEMEM32_LE(&oed->oed_EPCaps, epcaps);
        oed->oed_TailPtr = ohcihcp->ohc_OhciTermTD->otd_Self;

        predotd = NULL;
        oed->oed_Buffer = usbGetBuffer(ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_Dir);
        actual = 0;
        do {
            if((actual >= OHCI_TD_BULK_LIMIT) && (actual < ioreq->iouh_Length)) {
                pciusbWarn("OHCI", "Bulk too large, splitting...\n");
                break;
            }
            otd = ohciAllocTD(hc);
            if(!otd) {
                if(predotd != NULL) {
                    predotd->otd_Succ = NULL;
                }
                break;
            }
            otd->otd_ED = oed;
            if(predotd) {
                predotd->otd_Succ = otd;
                predotd->otd_NextTD = otd->otd_Self;
            } else {
                WRITEMEM32_LE(&oed->oed_HeadPtr, READMEM32_LE(&otd->otd_Self)|(unit->hu_DevDataToggle[devadrep] ? OEHF_DATA1 : 0));
                oed->oed_FirstTD = otd;
            }
            len = ioreq->iouh_Length - actual;
            if(len > OHCI_PAGE_SIZE) {
                len = OHCI_PAGE_SIZE;
            }
            otd->otd_Length = len;
            pciusbOHCIDebug("OHCI", "TD with %ld bytes\n", len);
            CONSTWRITEMEM32_LE(&otd->otd_Ctrl, OTCF_CC_INVALID|OTCF_NOINT);
            if(len) {
                len = ohciPrepareDMABuffer(hc, otd,
                                          (APTR)((UBYTE *)oed->oed_Buffer + actual),
                                          len,
                                          (ioreq->iouh_Dir == UHDIR_IN) ? 0 : DMA_ReadFromRAM);
                otd->otd_Length = len;
                pciusbOHCIDebug("OHCI", "TD send: %08lx - %08lx\n",
                                READMEM32_LE(&otd->otd_BufferPtr),
                                READMEM32_LE(&otd->otd_BufferEnd));
            } else {
                CONSTWRITEMEM32_LE(&otd->otd_BufferPtr, 0);
                CONSTWRITEMEM32_LE(&otd->otd_BufferEnd, 0);
            }
            actual += len;
            CacheClearE(&otd->otd_Ctrl, 16, CACRF_ClearD);

            predotd = otd;
        } while((actual < ioreq->iouh_Length) || (len && (ioreq->iouh_Dir == UHDIR_OUT) && (actual == ioreq->iouh_Length) && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) && ((actual % ioreq->iouh_MaxPktSize) == 0)));

        if(!actual) {
            // out of TDs
            pciusbError("OHCI", "Out of TDs for Bulk Transfer!\n");
            ohciFreeTDChain(hc, oed->oed_FirstTD);
            usbReleaseBuffer(oed->oed_Buffer, ioreq->iouh_Data, 0, 0);
            ohciFreeED(hc, oed);
            break;
        }
        oed->oed_Continue = (actual < ioreq->iouh_Length);
        predotd->otd_Succ = NULL;
        predotd->otd_NextTD = ohcihcp->ohc_OhciTermTD->otd_Self;

        CONSTWRITEMEM32_LE(&predotd->otd_Ctrl, OTCF_CC_INVALID);
        CacheClearE(&predotd->otd_Ctrl, 16, CACRF_ClearD);

        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = oed;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;
        unit->hu_NakTimeoutFrame[devadrep] = (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter + ioreq->iouh_NakTimeout : 0;

        Disable();
        AddTail(&hc->hc_TDQueue, (struct Node *) ioreq);

        // looks good to me, now enqueue this entry
        oed->oed_Succ = ohcihcp->ohc_OhciBulkTailED;
        oed->oed_NextED = oed->oed_Succ->oed_Self;
        oed->oed_Pred = ohcihcp->ohc_OhciBulkTailED->oed_Pred;
        oed->oed_Pred->oed_Succ = oed;
        oed->oed_Pred->oed_NextED = oed->oed_Self;
        oed->oed_Succ->oed_Pred = oed;
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);
        CacheClearE(&oed->oed_Pred->oed_EPCaps, 16, CACRF_ClearD);
        SYNC;

        pciusbOHCIDebug("OHCI", "Activating BULK at %ld\n", READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT));
        PrintED("Bulk", oed, hc);

        /* Similar to ohciScheduleCtrlTDs(), but use bulk queue */
        startmask = OCSF_BULKENABLE;
        Enable();
        ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    }

    if (startmask) {
        oldenables = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
        if(!(oldenables & OCSF_BULKENABLE)) {
            CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_ED, 0);
        }
    }
    return startmask;
}

void ohciUpdateFrameCounter(struct PCIController *hc)
{

    Disable();
    hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xffff0000)|(READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT) & 0xffff);
    Enable();
}

static AROS_INTH1(ohciCompleteInt, struct PCIController *,hc)
{
    AROS_INTFUNC_INIT

#ifndef base
    struct PCIDevice *base = hc->hc_Device;
#endif
    struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
    ULONG restartmask = 0;

    pciusbOHCIDebugV("OHCI", "CompleteInt!\n");

    ohciUpdateFrameCounter(hc);

    /* **************** PROCESS DONE TRANSFERS **************** */

    if (ohcihcp->ohc_OhciDoneQueue)
        ohciHandleFinishedTDs(hc);

    if (hc->hc_Flags & HCF_ABORT)
        restartmask = ohciHandleAbortedEDs(hc);

    if ((!(hc->hc_Flags & HCF_STOP_CTRL)) && hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
        restartmask |= ohciScheduleCtrlTDs(hc);

    if (hc->hc_IntXFerQueue.lh_Head->ln_Succ)
        ohciScheduleIntTDs(hc);

    if (hc->hc_IsoXFerQueue.lh_Head->ln_Succ)
        ohciScheduleIsoTDs(hc);

    if ((!(hc->hc_Flags & HCF_STOP_BULK)) && hc->hc_BulkXFerQueue.lh_Head->ln_Succ)
        restartmask |= ohciScheduleBulkTDs(hc);

    /*
     * Restart queues. In restartmask we have accumulated which queues need to be started.
     *
     * We do it here only once, after everything is set up, because otherwise HC goes nuts
     * in some cases. For example, the following situation caused TD queue loop: we are
     * simultaneously scheduling two control EDs and one of them completes with error. If
     * we attempt to start the queue right after a ED is scheduled (this is how the code
     * originally worked), it looks like the HC manages to deal with the first ED right
     * before the second one is scheduled. At this moment the first TD is HALTed with
     * oed_HeadPtr pointing to the failed TD, which went to the DoneQueue (which will be
     * picked up only on next ISR round, we are still in ohciSchedileCtrlEDs()). The
     * second ED is scheduled (first one is not removed yet!) and we re-trigger control
     * queue to start. It causes errorneous TD to reappear on the DoneQueue, effectively
     * looping it. DoneQueue loop causes ohciHandleFinishedTDs() to never exit.
     * Restarting queues here in this manner actually fixed the problem.
     */
    if (restartmask) {
        restartmask |= READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
        WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, restartmask);
        SYNC;
    }

    pciusbOHCIDebugV("OHCI", "CompleteDone\n");

    return 0;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(ohciIntCode, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
#ifndef base
    struct PCIDevice *base = hc->hc_Device;
#endif
    struct PCIUnit *unit = hc->hc_Unit;
    ULONG intr = 0;
    ULONG donehead;

    pciusbOHCIDebugV("OHCI", "ohciIntCode(0x%p)\n", hc);

    CacheClearE(&ohcihcp->ohc_OhciHCCA->oha_DoneHead, sizeof(ohcihcp->ohc_OhciHCCA->oha_DoneHead), CACRF_InvalidateD);

    donehead = READMEM32_LE(&ohcihcp->ohc_OhciHCCA->oha_DoneHead);

    if(donehead) {
        if (donehead & ~1)
            intr = OISF_DONEHEAD;
        if(donehead & 1) {
            intr |= READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS);
        }
        donehead &= OHCI_PTRMASK;

        CONSTWRITEMEM32_LE(&ohcihcp->ohc_OhciHCCA->oha_DoneHead, 0);

        pciusbOHCIDebugV("OHCI", "New Donehead %08lx for old %08lx\n", donehead, ohcihcp->ohc_OhciDoneQueue);
    } else {
        intr = READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS);

        if (intr & OISF_DONEHEAD) {
            pciusbOHCIDebug("OHCI", "!!!!!!!!!!!!!!!!!!!!!!!DoneHead was empty!!!!!!!!!!!!!!!!!!!\n");
            CacheClearE(ohcihcp->ohc_OhciHCCA, sizeof(struct OhciHCCA), CACRF_InvalidateD);
            donehead = READMEM32_LE(&ohcihcp->ohc_OhciHCCA->oha_DoneHead) & OHCI_PTRMASK;
            CONSTWRITEMEM32_LE(&ohcihcp->ohc_OhciHCCA->oha_DoneHead, 0);

            pciusbOHCIDebugV("OHCI", "New Donehead %08lx for old %08lx\n", donehead, ohcihcp->ohc_OhciDoneQueue);
        }
    }
    CacheClearE(ohcihcp->ohc_OhciHCCA, sizeof(struct OhciHCCA), CACRF_ClearD);

    intr &= ~OISF_MASTERENABLE;

    if(intr & hc->hc_PCIIntEnMask) {
        pciusbOHCIDebug("OHCI", "interrupts 0x%08lx, mask 0x%08lx\n", intr, hc->hc_PCIIntEnMask);

        // Acknowledge all interrupts, but process only those we want
        WRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, intr);
        //KPRINTF(1, ("INT=%02lx\n", intr));
        intr &= hc->hc_PCIIntEnMask;

        if(intr & OISF_HOSTERROR) {
            pciusbError("OHCI", "Host ERROR!\n");
        }
        if(intr & OISF_SCHEDOVERRUN) {
            pciusbError("OHCI", "Schedule overrun!\n");
        }
        if (!(hc->hc_Flags & HCF_ONLINE)) {
            if(READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS) & OISF_HUBCHANGE) {
                // if the driver is not online and the controller has a broken
                // hub change interrupt, make sure we don't run into infinite
                // interrupt by disabling the interrupt bit
                ohciDisableInt(hc, OISF_HUBCHANGE);
            }
            return FALSE;
        }
        ohciEnableInt(hc, OISF_HUBCHANGE);
        if(intr & OISF_FRAMECOUNTOVER) {
            hc->hc_FrameCounter |= 0x7fff;
            hc->hc_FrameCounter++;
            hc->hc_FrameCounter |= READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT) & 0xffff;
            pciusbOHCIDebugV("OHCI", "HCI 0x%p: Frame Counter Rollover %ld\n", hc, hc->hc_FrameCounter);
        }
        if(intr & OISF_HUBCHANGE) {
            ohciCheckPortStatusChange(hc);
        }
        if(intr & OISF_DONEHEAD) {
            pciusbOHCIDebugV("OHCI", "DoneHead Frame=%ld\n", READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT));

            if(ohcihcp->ohc_OhciDoneQueue) {
                struct OhciTD *donetd = (struct OhciTD *) ((IPTR)donehead - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));

                CacheClearE(&donetd->otd_Ctrl, 16, CACRF_InvalidateD);
                while(donetd->otd_NextTD) {
                    donetd = (struct OhciTD *) ((IPTR)donetd->otd_NextTD - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
                    CacheClearE(&donetd->otd_Ctrl, 16, CACRF_InvalidateD);
                }
                WRITEMEM32_LE(&donetd->otd_NextTD, ohcihcp->ohc_OhciDoneQueue);
                CacheClearE(&donetd->otd_Ctrl, 16, CACRF_ClearD);

                pciusbOHCIDebugV("OHCI", "Attached old DoneHead 0x%08lx to TD 0x%08lx\n", ohcihcp->ohc_OhciDoneQueue, donetd->otd_Self);
            }
            ohcihcp->ohc_OhciDoneQueue = donehead;
        }
        if (intr & OISF_SOF) {
            /* Aborted EDs are available for freeing */
            hc->hc_Flags |= HCF_ABORT;
        }

        if (intr & (OISF_SOF | OISF_DONEHEAD)) {
            /*
             * These two are leveraged down to SoftInt.
             * This is done in order to keep queues rotation synchronized.
             */
            SureCause(base, &hc->hc_CompleteInt);
        }

        pciusbOHCIDebugV("OHCI", "Exiting ohciIntCode(0x%p)\n", unit);
    }

    /* Unlock interrupts  */
    WRITEREG32_LE(&hc->hc_RegBase, OHCI_INTEN, OISF_MASTERENABLE);

    return FALSE;

    AROS_INTFUNC_EXIT
}

#undef base
#define base (hc->hc_Device)

/*
 * CHECKME: This routine is implemented according to the OHCI specification, however poorly tested.
 * Additionally, disabling and re-enabling the queue seems to create a significant delay. Perhaps
 * this can be optimized. In fact the only thing we really need to is to make sure that the ED to
 * be removed is neither on list nor being processed at the moment. Perhaps it's enough to simply
 * unlink it, set SKIP flag and wait for the next SOF.
 * But be careful, improper TD/ED removal can easily cause DoneQueue loops which are extremely hard
 * to isolate and fix (debug output adds delays which hide the problem). One of danger signs are
 * "Came accross a rogue TD" messages on the debug log. They mean that one of freed TDs reappeared
 * on the DoneQueue. If you run intensive I/O, you can be unlucky enough to reallocate and reuse this
 * TD before is passes DoneQueue, so it will appear there for the second time and create loop.
 */
void ohciAbortRequest(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct OhciED *oed = ioreq->iouh_DriverPrivate1;
    UWORD devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
    ULONG disablemask = 0;
    ULONG ctrlstatus;

    pciusbOHCIDebug("OHCI", "HC 0x%p Aborting request 0x%p, command %ld, endpoint 0x%04lx, Frame=%ld\n", hc, ioreq, ioreq->iouh_Req.io_Command, devadrep, READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT));
    PrintED("Aborting", oed, hc);

    /* Removing control and bulk EDs requires to stop the appropriate HC queue first (according to specification) */
    switch (ioreq->iouh_Req.io_Command) {
    case UHCMD_CONTROLXFER:
        pciusbOHCIDebug("OHCI", "Stopping control queue\n");
        hc->hc_Flags |= HCF_STOP_CTRL;
        disablemask = OCSF_CTRLENABLE;
        break;

    case UHCMD_BULKXFER:
        pciusbOHCIDebug("OHCI", "Stopping bulk queue\n");
        hc->hc_Flags |= HCF_STOP_BULK;
        disablemask = OCSF_BULKENABLE;
        break;
    }

    /* Stop selected queue(s) */
    if (disablemask) {
        ctrlstatus = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
        ctrlstatus &= ~disablemask;
        WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, ctrlstatus);
        SYNC;
    }

    // disable ED
    ohciDisableED(oed);

    /*
     * ...and move to abort queue.
     * We can't reply the request right now because some of its TDs
     * can be used by the HC right now. This means it does something
     * to the data buffer referred to by the request.
     * We reply the request only when the HC stops doing this. Otherwise
     * we may end up in trashed memory.
     */
    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
    AddTail(&hc->hc_AbortQueue, &ioreq->iouh_Req.io_Message.mn_Node);

    if (ioreq->iouh_Req.io_Command == UHCMD_INTXFER)
        ohciUpdateIntTree(hc);

    unit->hu_DevDataToggle[devadrep] = (READMEM32_LE(&oed->oed_HeadPtr) & OEHF_DATA1) ? TRUE : FALSE;

    /*
     * Request StartOfFrame interrupt. Upon next frame this ED
     * is guaranteed to be out of use and can be freed.
     */
    ohciEnableInt(hc, OISF_SOF);
}

BOOL ohciInit(struct PCIController *hc, struct PCIUnit *hu)
{
    struct OhciHCPrivate *ohcihcp;
    struct PCIDevice *hd = hu->hu_Device;

    struct OhciED *oed;
    struct OhciED *predoed;
    struct OhciTD *otd;
    struct OhciIsoTD *oitd;
    ULONG *tabptr;
    UBYTE *memptr;
    ULONG bitcnt;
    ULONG hubdesca;
    ULONG revision;
    ULONG cmdstatus;
    ULONG control;
    ULONG timeout;
    ULONG frameival;

    ULONG cnt;

    struct TagItem pciActivateMem[] = {
        { aHidd_PCIDevice_isMEM,    TRUE },
        { TAG_DONE, 0UL },
    };

    struct TagItem pciActivateBusmaster[] = {
        { aHidd_PCIDevice_isMaster, TRUE },
        { TAG_DONE, 0UL },
    };

    struct TagItem pciDeactivateBusmaster[] = {
        { aHidd_PCIDevice_isMaster, FALSE },
        { TAG_DONE, 0UL },
    };

    ohcihcp = AllocMem(sizeof(struct OhciHCPrivate), MEMF_CLEAR);
    if (!ohcihcp)
        return FALSE;

    NewList(&ohcihcp->ohc_OhciRetireQueue);

    hc->hc_CPrivate = ohcihcp;
    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = "OHCI CompleteInt";
    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (VOID_FUNC)ohciCompleteInt;

    hc->hc_PCIMem.me_Length = OHCI_HCCA_SIZE + OHCI_HCCA_ALIGNMENT + 1;
    hc->hc_PCIMem.me_Length += sizeof(struct OhciED) * OHCI_ED_POOLSIZE;
    hc->hc_PCIMem.me_Length += sizeof(struct OhciTD) * OHCI_TD_POOLSIZE;
    hc->hc_PCIMem.me_Length += sizeof(struct OhciIsoTD) * OHCI_ISO_TD_POOLSIZE;

    memptr = ALLOCPCIMEM(hc, hc->hc_PCIDriverObject, hc->hc_PCIMem.me_Length);
    hc->hc_PCIMem.me_Un.meu_Addr = (APTR) memptr;
    if (memptr) {
        // PhysicalAddress - VirtualAdjust = VirtualAddress
        // VirtualAddress  + VirtualAdjust = PhysicalAddress
        hc->hc_PCIVirtualAdjust = pciGetPhysical(hc, memptr) - (APTR)memptr;
        pciusbOHCIDebug("OHCI", "VirtualAdjust 0x%08lx\n", hc->hc_PCIVirtualAdjust);

        // align memory
        memptr = (UBYTE *) (((IPTR)hc->hc_PCIMem.me_Un.meu_Addr + OHCI_HCCA_ALIGNMENT) & (~OHCI_HCCA_ALIGNMENT));
        ohcihcp->ohc_OhciHCCA = (struct OhciHCCA *) memptr;
        pciusbOHCIDebug("OHCI", "HCCA 0x%p\n", ohcihcp->ohc_OhciHCCA);
        memptr += OHCI_HCCA_SIZE;

        // build up ED pool
        oed = (struct OhciED *) memptr;
        ohcihcp->ohc_OhciEDPool = oed;
        cnt = OHCI_ED_POOLSIZE - 1;
        do {
            // minimal initalization
            oed->oed_Succ = (oed + 1);
            WRITEMEM32_LE(&oed->oed_Self, (IPTR)(&oed->oed_EPCaps) + hc->hc_PCIVirtualAdjust);
            oed++;
        } while(--cnt);
        oed->oed_Succ = NULL;
        WRITEMEM32_LE(&oed->oed_Self, (IPTR)(&oed->oed_EPCaps) + hc->hc_PCIVirtualAdjust);
        memptr += sizeof(struct OhciED) * OHCI_ED_POOLSIZE;

        // build up TD pool
        otd = (struct OhciTD *) memptr;
        ohcihcp->ohc_OhciTDPool = otd;
        cnt = OHCI_TD_POOLSIZE - 1;
        do {
            otd->otd_Succ = (otd + 1);
            WRITEMEM32_LE(&otd->otd_Self, (IPTR)(&otd->otd_Ctrl) + hc->hc_PCIVirtualAdjust);
            otd++;
        } while(--cnt);
        otd->otd_Succ = NULL;
        WRITEMEM32_LE(&otd->otd_Self, (IPTR)(&otd->otd_Ctrl) + hc->hc_PCIVirtualAdjust);
        memptr += sizeof(struct OhciTD) * OHCI_TD_POOLSIZE;

        // build up ISO TD pool
        oitd = (struct OhciIsoTD *) memptr;
        ohcihcp->ohc_OhciIsoTDPool = oitd;
        cnt = OHCI_ISO_TD_POOLSIZE - 1;
        do {
            oitd->oitd_Succ = (oitd + 1);
            WRITEMEM32_LE(&oitd->oitd_Self, (IPTR)(&oitd->oitd_Ctrl) + hc->hc_PCIVirtualAdjust);
            oitd++;
        } while(--cnt);
        oitd->oitd_Succ = NULL;
        WRITEMEM32_LE(&oitd->oitd_Self, (IPTR)(&oitd->oitd_Ctrl) + hc->hc_PCIVirtualAdjust);
        memptr += sizeof(struct OhciIsoTD) * OHCI_ISO_TD_POOLSIZE;

        // terminating ED
        ohcihcp->ohc_OhciTermED = oed = ohciAllocED(hc);
        oed->oed_Succ = NULL;
        oed->oed_Pred = NULL;
        CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
        oed->oed_NextED = 0;

        // terminating TD
        ohcihcp->ohc_OhciTermTD = otd = ohciAllocTD(hc);
        otd->otd_Succ = NULL;
        otd->otd_NextTD = 0;

        // dummy head & tail Ctrl ED
        ohcihcp->ohc_OhciCtrlHeadED = predoed = ohciAllocED(hc);
        ohcihcp->ohc_OhciCtrlTailED = oed = ohciAllocED(hc);
        CONSTWRITEMEM32_LE(&predoed->oed_EPCaps, OECF_SKIP);
        CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
        predoed->oed_Succ = oed;
        predoed->oed_Pred = NULL;
        predoed->oed_NextED = oed->oed_Self;
        oed->oed_Succ = NULL;
        oed->oed_Pred = predoed;
        oed->oed_NextED = 0;

        // dummy head & tail Bulk ED
        ohcihcp->ohc_OhciBulkHeadED = predoed = ohciAllocED(hc);
        ohcihcp->ohc_OhciBulkTailED = oed = ohciAllocED(hc);
        CONSTWRITEMEM32_LE(&predoed->oed_EPCaps, OECF_SKIP);
        CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
        predoed->oed_Succ = oed;
        predoed->oed_Pred = NULL;
        predoed->oed_NextED = oed->oed_Self;
        oed->oed_Succ = NULL;
        oed->oed_Pred = predoed;
        oed->oed_NextED = 0;
        // 1 ms INT QH
        ohcihcp->ohc_OhciIntED[0] = oed = ohciAllocED(hc);
        oed->oed_Succ = ohcihcp->ohc_OhciTermED;
        oed->oed_Pred = NULL; // who knows...
        CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
        oed->oed_NextED = ohcihcp->ohc_OhciTermED->oed_Self;
        predoed = oed;
        // make 5 levels of QH interrupts
        for(cnt = 1; cnt < 5; cnt++) {
            ohcihcp->ohc_OhciIntED[cnt] = oed = ohciAllocED(hc);
            oed->oed_Succ = predoed;
            oed->oed_Pred = NULL; // who knows...
            CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
            oed->oed_NextED = ohcihcp->ohc_OhciTermED->oed_Self;
            predoed = oed;
        }

        ohciUpdateIntTree(hc);

        // fill in framelist with IntED entry points based on interval
        tabptr = ohcihcp->ohc_OhciHCCA->oha_IntEDs;
        for(cnt = 0; cnt < 32; cnt++) {
            oed = ohcihcp->ohc_OhciIntED[4];
            bitcnt = 0;
            do {
                if(cnt & (1UL<<bitcnt)) {
                    oed = ohcihcp->ohc_OhciIntED[bitcnt];
                    break;
                }
            } while(++bitcnt < 5);
            *tabptr++ = oed->oed_Self;
        }

        // time to initialize hardware...
        OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base0, (IPTR *) &hc->hc_RegBase);
        hc->hc_RegBase = (APTR) (((IPTR) hc->hc_RegBase) & (~0xf));
        pciusbOHCIDebug("OHCI", "RegBase = 0x%p\n", hc->hc_RegBase);
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateMem); // enable memory

        hubdesca = READREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA);
        revision = READREG32_LE(hc->hc_RegBase, OHCI_REVISION);
        hc->hc_NumPorts = (hubdesca & OHAM_NUMPORTS)>>OHAS_NUMPORTS;
        pciusbOHCIDebug("OHCI", "Found OHCI Controller %p FuncNum = %ld, Rev %02lx, with %ld ports\n",
                hc->hc_PCIDeviceObject, hc->hc_FunctionNum,
                revision & 0xFF,
                hc->hc_NumPorts);

        pciusbOHCIDebug("OHCI", "Powerswitching: %s %s\n",
                hubdesca & OHAF_NOPOWERSWITCH ? "Always on" : "Available",
                hubdesca & OHAF_INDIVIDUALPS ? "per port" : "global");

        control = READREG32_LE(hc->hc_RegBase, OHCI_CONTROL);
        pciusbOHCIDebug("OHCI", "OHCI control state: 0x%08lx\n", control);

        // disable BIOS legacy support
        if (control & OCLF_SMIINT) {
            pciusbOHCIDebugV("OHCI", "BIOS still has hands on OHCI, trying to get rid of it\n");

            cmdstatus = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
            cmdstatus |= OCSF_OWNERCHANGEREQ;
            WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, cmdstatus);
            timeout = 100;
            do {
                control = READREG32_LE(hc->hc_RegBase, OHCI_CONTROL);
                if(!(control & OCLF_SMIINT)) {
                    pciusbOHCIDebugV("OHCI", "BIOS gave up on OHCI. Pwned!\n");
                    break;
                }
                uhwDelayMS(10, hu->hu_TimerReq);
            } while(--timeout);
            if(!timeout) {
                pciusbOHCIDebugV("OHCI", "BIOS didn't release OHCI. Forcing and praying...\n");
                control &= ~OCLF_SMIINT;
                WRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL, control);
            }
        }

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciDeactivateBusmaster); // no busmaster yet

        pciusbOHCIDebug("OHCI", "Resetting OHCI HC\n");
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, OCSF_HCRESET);
        cnt = 100;
        do {
            if(!(READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS) & OCSF_HCRESET)) {
                break;
            }
            uhwDelayMS(1, hu->hu_TimerReq);
        } while(--cnt);

#ifdef DEBUG
        if(cnt == 0) {
            pciusbOHCIDebugV("OHCI", "Reset Timeout!\n");
        } else {
            pciusbOHCIDebugV("OHCI", "Reset finished after %ld ticks\n", 100-cnt);
        }
#endif

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateBusmaster); // enable busmaster

        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_PERIODICSTART, 10800); // 10% of 12000
        frameival = READREG32_LE(hc->hc_RegBase, OHCI_FRAMEINTERVAL);
        pciusbOHCIDebug("OHCI", "FrameInterval=%08lx\n", frameival);
        frameival &= ~OIVM_BITSPERFRAME;
        frameival |= OHCI_DEF_BITSPERFRAME<<OIVS_BITSPERFRAME;
        frameival ^= OIVF_TOGGLE;
        WRITEREG32_LE(hc->hc_RegBase, OHCI_FRAMEINTERVAL, frameival);

        // make sure nothing is running
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_PERIODIC_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CTRL_HEAD_ED, AROS_LONG2LE(ohcihcp->ohc_OhciCtrlHeadED->oed_Self));
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CTRL_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_HEAD_ED, AROS_LONG2LE(ohcihcp->ohc_OhciBulkHeadED->oed_Self));
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_DONEHEAD, 0);

        WRITEREG32_LE(hc->hc_RegBase, OHCI_HCCA, (IPTR)pciGetPhysical(hc, ohcihcp->ohc_OhciHCCA));

        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, OISF_ALL_INTS);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTDIS, OISF_ALL_INTS);
        SYNC;

        // install reset handler
        hc->hc_ResetInt.is_Node.ln_Name = "OHCI PCI (pciusb.device)";
        hc->hc_ResetInt.is_Code = (VOID_FUNC)OhciResetHandler;
        hc->hc_ResetInt.is_Data = hc;
        AddResetCallback(&hc->hc_ResetInt);

        // add interrupt
        hc->hc_PCIIntHandler.is_Node.ln_Name = hc->hc_ResetInt.is_Node.ln_Name;
        hc->hc_PCIIntHandler.is_Node.ln_Pri = 5;
        hc->hc_PCIIntHandler.is_Node.ln_Type = NT_INTERRUPT;
        hc->hc_PCIIntHandler.is_Code = (VOID_FUNC)ohciIntCode;
        hc->hc_PCIIntHandler.is_Data = hc;
        PCIXAddInterrupt(hc, &hc->hc_PCIIntHandler);

        hc->hc_PCIIntEnMask = OISF_DONEHEAD|OISF_RESUMEDTX|OISF_HOSTERROR|OISF_FRAMECOUNTOVER|OISF_HUBCHANGE;

        WRITEREG32_LE(hc->hc_RegBase, OHCI_INTEN, hc->hc_PCIIntEnMask|OISF_MASTERENABLE);

        /* Don't try to enter RESET state via OHCI_CONTROL - this flakes out
         * some chips, particularly the Sam460ex
         */
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL, OCLF_PERIODICENABLE|OCLF_CTRLENABLE|OCLF_BULKENABLE|OCLF_ISOENABLE|OCLF_USBOPER);
        SYNC;

        // make sure the ports are on with chipset quirk workaround
        hubdesca = READREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA);
        if (hd->hd_Flags & HDF_FORCEPOWER)
            hubdesca |= OHAF_NOPOWERSWITCH;     /* Required for some IntelMacs */
        else
            hubdesca |= OHAF_NOOVERCURRENT;
        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA, hubdesca);

        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBSTATUS, OHSF_POWERHUB);
        if((hubdesca & OHAF_NOPOWERSWITCH) || (!(hubdesca & OHAF_INDIVIDUALPS))) {
            pciusbOHCIDebugV("OHCI", "Individual power switching not available, turning on all ports!\n");
            WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCB, 0);
        } else {
            pciusbOHCIDebugV("OHCI", "Enabling individual power switching\n");
            WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCB, ((2<<hc->hc_NumPorts)-2)<<OHBS_PORTPOWERCTRL);
        }

        uhwDelayMS(50, hu->hu_TimerReq);
        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA, hubdesca);

        CacheClearE(ohcihcp->ohc_OhciHCCA,   sizeof(struct OhciHCCA),          CACRF_ClearD);
        CacheClearE(ohcihcp->ohc_OhciEDPool, sizeof(struct OhciED) * OHCI_ED_POOLSIZE, CACRF_ClearD);
        CacheClearE(ohcihcp->ohc_OhciTDPool, sizeof(struct OhciTD) * OHCI_TD_POOLSIZE, CACRF_ClearD);
        CacheClearE(ohcihcp->ohc_OhciIsoTDPool, sizeof(struct OhciIsoTD) * OHCI_ISO_TD_POOLSIZE, CACRF_ClearD);

        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL, OCLF_PERIODICENABLE|OCLF_CTRLENABLE|OCLF_BULKENABLE|OCLF_ISOENABLE|OCLF_USBOPER);
        SYNC;

        pciusbOHCIDebugV("OHCI", "ohciInit returns TRUE...\n");
        return TRUE;
    }

    pciusbOHCIDebugV("OHCI", "ohciInit returns FALSE...\n");
    return FALSE;
}

void ohciFree(struct PCIController *hc, struct PCIUnit *hu)
{
    pciusbOHCIDebug("OHCI", "Shutting down OHCI %p\n", hc);
    CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTDIS, OISF_ALL_INTS);
    CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, OISF_ALL_INTS);

    // disable all ports
    WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCB, 0);
    WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBSTATUS, OHSF_UNPOWERHUB);

    uhwDelayMS(50, hu->hu_TimerReq);
    pciusbOHCIDebug("OHCI", "Stopping OHCI %p\n", hc);
    CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL, 0);
    CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, 0);
    SYNC;

    //pciusbOHCIDebug("OHCI", ("Reset done UHCI %08lx\n", hc));
    uhwDelayMS(10, hu->hu_TimerReq);
    pciusbOHCIDebug("OHCI", "Resetting OHCI %p\n", hc);
    CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, OCSF_HCRESET);
    SYNC;
    uhwDelayMS(50, hu->hu_TimerReq);

    struct OhciHCPrivate *ohcihcp = (struct OhciHCPrivate *)hc->hc_CPrivate;
    hc->hc_CPrivate = NULL;
    if (ohcihcp)
        FreeMem(ohcihcp, sizeof(struct OhciHCPrivate));

    pciusbOHCIDebug("OHCI", "Shutting down OHCI done.\n");
}
