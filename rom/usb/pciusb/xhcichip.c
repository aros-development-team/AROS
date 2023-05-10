/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved

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

#undef base
#define base (hc->hc_Device)

#if defined(DEBUG) && defined(XHCI_LONGDEBUGNAK)
#define XHCI_NAKTOSHIFT         (8)
#else
#define XHCI_NAKTOSHIFT         (3)
#endif

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

// See page 395 "EWE" - to enable rollover events.
void xhciUpdateFrameCounter(struct PCIController *hc)
{
    volatile struct xhci_rrs *rrs = (volatile struct xhci_rrs *)((IPTR)hc->hc_XHCIIntR - 0x20);
    UWORD framecnt;
    
    Disable();
    framecnt = rrs->mfindex & 0x3FFF;
    if(framecnt < (hc->hc_FrameCounter & 0x3FFF))
    {
        hc->hc_FrameCounter |= 0x3FFF;
        hc->hc_FrameCounter++;
        hc->hc_FrameCounter |= framecnt;
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Frame Counter Rollover %ld\n", hc->hc_FrameCounter);
    } else {
        hc->hc_FrameCounter = (hc->hc_FrameCounter & 0xFFFFC000)|framecnt;
    }
    Enable();
}

static UBYTE xhciGetEPID(UBYTE endpoint, UBYTE dir)
{
    UBYTE epid = 1;

    if (endpoint > 0) {
        epid = (endpoint & 0X0F) * 2;
        epid += dir;
    }
    return epid;
}

static int xhciRingEntriesFree(volatile struct pcisusbXHCIRing *ring)
{
    ULONG last = (ring->end & ~RINGENDCFLAG);
    ULONG idx = ring->next;

    return (last > idx) ? last-idx
               : (USB_DEV_MAX - 1) - idx + last;
}

static void xhciInsertTRB(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload, ULONG trflags, ULONG plen)
{
    volatile struct xhci_trb *dst;
    ULONG trbflags = (trflags & ~TRBF_FLAG_C);

    // Set the cycle bit
    if (ring->end & RINGENDCFLAG)
        trbflags |= TRBF_FLAG_C;

    // Get the next available ring entry
    dst = &ring->ring[ring->next];

    //... And populate it
    xhciSetPointer(hc, dst->dbp, payload);
    dst->tparams = (plen & TRB_TPARAMS_DS_TRBLEN_SMASK);
    dst->flags = trbflags;
}

WORD xhciQueueTRB(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload,
                          ULONG plen, ULONG trbflags)
{
    if (trbflags & TRBF_FLAG_IDT)
    {
        pciusbDebugTRB("xHCI", DEBUGFUNCCOLOR_SET "(0x%p, $%02x %02x %02x%02x %02x%02x %02x%02x, %u, $%08x)" DEBUGCOLOR_RESET" \n", ring,
                                                                                                                ((UBYTE *)&payload)[0], ((UBYTE *)&payload)[1], ((UBYTE *)&payload)[3], ((UBYTE *)&payload)[2],
                                                                                                                ((UBYTE *)&payload)[5], ((UBYTE *)&payload)[4], ((UBYTE *)&payload)[7], ((UBYTE *)&payload)[6],
                                                                                                                plen, trbflags);
    }
    else
        pciusbDebugTRB("xHCI", DEBUGFUNCCOLOR_SET "(0x%p, 0x%p, %u, $%08x)" DEBUGCOLOR_RESET" \n", ring, payload, plen, trbflags);

    if (!ring)
    {
        pciusbDebugTRB("xHCI", DEBUGWARNCOLOR_SET "NO RINGSPECIFIED!!" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

    if (xhciRingEntriesFree(ring) > 1)
    {
        if (ring->next >= USB_DEV_MAX - 1)
        {
            /*
             * If this is the last ring element, insert a link
             * back to the ring start - and update the cycle bit
             */
            xhciInsertTRB(hc, ring, (UQUAD)&ring->ring[0], TRBF_FLAG_TRTYPE_LINK | TRBF_FLAG_ENT, 0);
            ring->next = 0;
            if (ring->end & RINGENDCFLAG)
                ring->end &= ~RINGENDCFLAG;
            else
                ring->end |= RINGENDCFLAG;
            pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "Ring Re-Linked!!" DEBUGCOLOR_RESET" \n");
        }

        xhciInsertTRB(hc, ring, payload, trbflags, plen);
        pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "ring %p <idx %d, %dbytes>" DEBUGCOLOR_RESET" \n", ring, ring->next, plen);
        return ring->next++;
    }

    pciusbDebugTRB("xHCI", DEBUGWARNCOLOR_SET "NO SPACE ON RING!!\nnext = %u\nlast = %u" DEBUGCOLOR_RESET" \n", ring->next, (ring->end & ~RINGENDCFLAG));
    for(;;)
        ;
    return -1;
}

WORD xhciQueueData(struct PCIController *hc, volatile struct pcisusbXHCIRing *ring, UQUAD payload,
                          ULONG plen, ULONG pmax, ULONG trbflags, BOOL ioconlast)
{
    ULONG remaining = plen;
    WORD queued, firstqueued = -1;

    do {
        ULONG trblen = remaining, txflags = trbflags;
        if (remaining > pmax)
            trblen = pmax;

        if (ioconlast && ((remaining - trblen) == 0))
            txflags |= TRBF_FLAG_IOC;

        queued = xhciQueueTRB(hc, ring, payload + (plen - remaining), trblen, txflags);
        if (queued == -1)
            return queued;

        if (firstqueued == -1)
            firstqueued = queued;

        remaining -= trblen;
    } while (remaining > 0);

    return firstqueued;
}

ULONG xhciInitEP(struct PCIController *hc, struct pcisusbXHCIDevice *devCtx,
                           UBYTE endpoint,
                           UBYTE dir,
                           ULONG type,
                           ULONG maxpacket)
{
    volatile struct pcisusbXHCIRing *epring;
    ULONG epid;

    pciusbDebugEP("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p, %u, %u, $%08x, %u)" DEBUGCOLOR_RESET" \n", __func__, hc, devCtx, endpoint, dir, type, maxpacket);

    epid = xhciGetEPID(endpoint, dir);

    pciusbDebugEP("xHCI", DEBUGCOLOR_SET "%s: EPID %u" DEBUGCOLOR_RESET" \n", __func__, epid);

    /* Test if already prepared */
    if (devCtx->dc_EPAllocs[epid].dmaa_Ptr != NULL)
        return epid;

    devCtx->dc_EPAllocs[epid].dmaa_Ptr = pciAllocAligned(hc, &devCtx->dc_EPAllocs[epid].dmaa_Entry, sizeof(struct pcisusbXHCIRing), ALIGN_EVTRING_SEG, (1 << 16));
    if (devCtx->dc_EPAllocs[epid].dmaa_Ptr)
    {
        pciusbDebugEP("xHCI", DEBUGCOLOR_SET "Allocated EP Ring @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", devCtx->dc_EPAllocs[epid].dmaa_Ptr, hc->hc_ERS.me_Un.meu_Addr, hc->hc_ERS.me_Length);
#if (0)
        devCtx->dc_EPAllocs[epid].dmaa_DMA = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)devCtx->dc_EPAllocs[epid].dmaa_Ptr);
#else
        devCtx->dc_EPAllocs[epid].dmaa_DMA = devCtx->dc_EPAllocs[epid].dmaa_Ptr;
#endif
        pciusbDebugEP("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", devCtx->dc_EPAllocs[epid].dmaa_DMA);
    }
    else
    {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "Unable to allocate EP Ring Memory" DEBUGCOLOR_RESET" \n");
        return 0;
    }
    epring = (volatile struct pcisusbXHCIRing *)devCtx->dc_EPAllocs[epid].dmaa_Ptr;
    epring->next = 0;
    epring->end |= RINGENDCFLAG;                                                                        // Mark our cycle flag

    volatile struct xhci_inctx *in = (volatile struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;
    in->acf |= (1 << epid);                                                                             // Add/Enable in the Add Context Flags

    UWORD ctxoff = 1;
    if (hc->hc_Flags & HCF_CTX64)
        ctxoff <<= 1;

    struct xhci_slot *islot = (void*)&in[ctxoff];
    if (epid > ((islot->ctx[0] >> 27) & 0xF))
    {
        islot->ctx[0] &= ~(0xF << 27);
        islot->ctx[0] |= (epid << 27);
    }

    struct xhci_ep *ep = (struct xhci_ep *)&in[ctxoff * (epid + 1)];

    pciusbDebugEP("xHCI", DEBUGCOLOR_SET "EP input Ctx @ 0x%p" DEBUGCOLOR_RESET" \n", ep);

    switch (type)
    {
    case UHCMD_ISOXFER:
        ep->ctx[1]   |= EPF_CTX_TYPE_ISOCH_O;
        break;

    case UHCMD_BULKXFER:
        ep->ctx[1]   |= EPF_CTX_TYPE_BULK_O;
        break;
        
    case UHCMD_INTXFER:
        ep->ctx[1]   |= EPF_CTX_TYPE_INTR_O;
        break;
    }
    if (type == UHCMD_CONTROLXFER || (dir))
        ep->ctx[1] |= EPF_CTX_TYPE_CONTROL;
    if (epid > 1)
    {
        if (type == UHCMD_CONTROLXFER)
        {
          ep->length = 8;                                                                               // Avg TRB Length (8 for Control xfer)
        }
        else
        {
            ep->length   = maxpacket;
        }
#if (0)
        ep->ctx[0] |= (3 << 16);                                                                        // Set the interval
#endif
    }
    ep->ctx[1] |= (EP_CTX_CERR_MASK << EPS_CTX_CERR);                                                   // Set CErr's initial value, and max packets
    ep->ctx[1] |= (maxpacket << EPS_CTX_PACKETMAX);

    pciusbDebugEP("xHCI", DEBUGCOLOR_SET "Setting de-queue ptr to 0x%p" DEBUGCOLOR_RESET" \n", devCtx->dc_EPAllocs[epid].dmaa_DMA);

    xhciSetPointer(hc, ep->deq, ((IPTR)devCtx->dc_EPAllocs[epid].dmaa_DMA | EPF_CTX_DEQ_DCS));

    pciusbDebugEP("xHCI", DEBUGCOLOR_SET "%s: Endpoint Ring Initialized @ 0x%p <EPID %u>" DEBUGCOLOR_RESET" \n", __func__, devCtx->dc_EPAllocs[epid].dmaa_Ptr, epid);

    return  epid;
}

/* Shutdown and Interrupt handlers */

static AROS_INTH1(XhciResetHandler, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * Page Size Register (PAGESIZE) - Chapter 5.4.3
 */
static ULONG xhciPageSize(struct PCIController *hc)
{
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)hc->hc_XHCIOpR);
    int bit;
    for (bit = 0; bit < 16; bit++)
    {
        if (hcopr->pagesize & (1 << bit))
            return 1 << (12 + bit);
    }
    return 0;
}

void xhciFinishRequest(struct PCIController *hc, struct PCIUnit *unit, struct IOUsbHWReq *ioreq)
{
    struct pciusbXHCIIODevPrivate *driprivate;
    UWORD devadrep;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    if ((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) != NULL)
    {
        /* Deactivate the endpoint */
        if ((driprivate->dpEPID > 1) && (driprivate->dpDevice))
        {
            struct pcisusbXHCIDevice *devCtx = driprivate->dpDevice;
            struct pcisusbXHCIRing *epRing = devCtx->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr;
            int cnt;

            xhciCmdEndpointStop(hc, driprivate->dpDevice->dc_SlotID, driprivate->dpEPID, TRUE);

            if (driprivate->dpSTRB != (UWORD)-1)
            {
                epRing->ringio[driprivate->dpSTRB] = NULL;
            }

            for (cnt = driprivate->dpTxSTRB; cnt < (driprivate->dpTxETRB + 1); cnt ++)
            {
                epRing->ringio[cnt] = NULL;
            }

            if (driprivate->dpSttTRB != (UWORD)-1)
            {
                epRing->ringio[driprivate->dpSttTRB] = NULL;
            }
                
            FREEPCIMEM(hc, hc->hc_PCIDriverObject, devCtx->dc_EPAllocs[driprivate->dpEPID].dmaa_Entry.me_Un.meu_Addr);
            devCtx->dc_EPAllocs[driprivate->dpEPID].dmaa_Entry.me_Un.meu_Addr = NULL;
            devCtx->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr = NULL;
        }
        FreeMem(driprivate, sizeof(struct pciusbXHCIIODevPrivate));
        ioreq->iouh_DriverPrivate1 = NULL;
    }
    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
    devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
    pciusbDebugEP("xHCI", DEBUGCOLOR_SET "%s: releasing DevEP %02lx" DEBUGCOLOR_RESET" \n", __func__, devadrep);
    unit->hu_DevBusyReq[devadrep] = NULL;
    unit->hu_NakTimeoutFrame[devadrep] = 0;
}

void xhciHandleFinishedTDs(struct PCIController *hc)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq, *nextioreq;
    struct pciusbXHCIIODevPrivate *driprivate;
    UWORD devadrep;
    ULONG actual;
    BOOL transactiondone;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    pciusbDebug("xHCI", DEBUGCOLOR_SET "Checking for Periodic work done..." DEBUGCOLOR_RESET" \n");
    ioreq = (struct IOUsbHWReq *) hc->hc_PeriodicTDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ))
    {
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Examining IOReq=0x%p" DEBUGCOLOR_RESET" \n", ioreq);

#if (0)
        if ((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) != NULL)
        {
#endif
            transactiondone = FALSE;
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);

            if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]))
            {
                pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "xHCI: Periodic NAK timeout (%u)" DEBUGCOLOR_RESET" \n", unit->hu_NakTimeoutFrame[devadrep]);
                ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                transactiondone = TRUE;
            }

            if (transactiondone)
            {
                ioreq->iouh_Actual += actual;
                xhciFreePeriodicContext(hc, unit, ioreq);
                ReplyMsg(&ioreq->iouh_Req.io_Message);
            }
#if (0)
        }
#endif
        ioreq = nextioreq;
    }

    pciusbDebug("xHCI", DEBUGCOLOR_SET "Checking for Standard work done..." DEBUGCOLOR_RESET" \n");
    ioreq = (struct IOUsbHWReq *) hc->hc_TDQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ))
    {
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Examining IOReq=0x%p" DEBUGCOLOR_RESET" \n", ioreq);

        if ((driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) != NULL)
        {
            transactiondone = FALSE;
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);

            if (driprivate->dpCC > 0)
            {
                pciusbDebug("xHCI", DEBUGCOLOR_SET "IOReq Complete (completion code %u)!" DEBUGCOLOR_RESET" \n", driprivate->dpCC);
                transactiondone = TRUE;
                switch (driprivate->dpCC)
                {
                case 1:
                    ioreq->iouh_Req.io_Error = UHIOERR_NO_ERROR;
                    break;

                case 3:
                    ioreq->iouh_Req.io_Error = UHIOERR_BABBLE;
                    break;

                case 6:
                    ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                    break;

                case 17: 
                    ioreq->iouh_Req.io_Error = UHIOERR_BADPARAMS;                                       // Param Error
                    break;

                case 20: 
                    ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;                                      // No Ping Response
                    break;

                default:
                    ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                    break;
                }
            }
            else if(unit->hu_NakTimeoutFrame[devadrep] && (hc->hc_FrameCounter > unit->hu_NakTimeoutFrame[devadrep]))
            {
                pciusbWarn("xHCI", DEBUGWARNCOLOR_SET "xHCI: Async NAK timeout (%u)" DEBUGCOLOR_RESET" \n", unit->hu_NakTimeoutFrame[devadrep]);
                ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                transactiondone = TRUE;
            }

            if (transactiondone)
            {
                xhciFreeAsyncContext(hc, unit, ioreq);
                if((!ioreq->iouh_Req.io_Error) && (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER))
                {
                    uhwCheckSpecialCtrlTransfers(hc, ioreq);
                }
                ReplyMsg(&ioreq->iouh_Req.io_Message);
            }
        }
        ioreq = nextioreq;
    }
}

static AROS_INTH1(xhciCompleteInt, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    xhciUpdateFrameCounter(hc);

    uhwCheckRootHubChanges(hc->hc_Unit);

    Signal(hc->hc_xHCTask, 1L<<hc->hc_DoWorkSignal);

    pciusbDebug("xHCI", DEBUGCOLOR_SET "%s Done" DEBUGCOLOR_RESET" \n", __func__);

    return FALSE;

    AROS_INTFUNC_EXIT
}

static BOOL xhciTRBCycleMatches(ULONG trbflags, ULONG cycle)
{
    if ((trbflags & TRBF_FLAG_C) == (cycle ? 1 : 0))
        return TRUE;
    return FALSE;
}

BOOL xhciIntWorkProcess(struct PCIController *hc, struct IOUsbHWReq *ioreq, ULONG remaining, ULONG ccode)
{
	struct pciusbXHCIIODevPrivate *driprivate = NULL;
    pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "Examining IOReq=0x%p" DEBUGCOLOR_RESET" \n", ioreq);

    if (ioreq && (driprivate = (struct pciusbXHCIIODevPrivate *)ioreq->iouh_DriverPrivate1) != NULL)
    {
        pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "IOReq TRB(s) = #%u:#%u\nIOReq Ring @ 0x%p" DEBUGCOLOR_RESET" \n", driprivate->dpTxSTRB, driprivate->dpTxETRB, driprivate->dpDevice->dc_EPAllocs[driprivate->dpEPID].dmaa_Ptr);

        driprivate->dpCC = ccode;

        ioreq->iouh_Actual += (ioreq->iouh_Length - remaining);
        pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "Remaing work for IO = %u bytes" DEBUGCOLOR_RESET" \n", remaining);

        return TRUE;
    }                        
	return FALSE;
}

static AROS_INTH1(xhciIntCode, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)hc->hc_XHCIOpR);
    BOOL doCompletion = FALSE, checkRHchanges = FALSE;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    ULONG status = AROS_LE2LONG(hcopr->usbsts);
    xhciDumpStatus(status);

    /* First acknowledge the interrupt ..*/
    hcopr->usbsts = AROS_LONG2LE(status);

    /* Check if anything interesting happened.... */
    if (status & XHCIF_USBSTS_HCE)
    {
        pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "Host Error detected" DEBUGCOLOR_RESET" \n");
    }
    if (status & XHCIF_USBSTS_HSE)
    {
        pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "System Error detected" DEBUGCOLOR_RESET" \n");
    }
    if (status & XHCIF_USBSTS_PCD)
    {
        pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "Port Change detected" DEBUGCOLOR_RESET" \n");
    }

    volatile struct xhci_ir *xhciir = (volatile struct xhci_ir *)((IPTR)hc->hc_XHCIIntR);
    xhciDumpIR(xhciir);

    ULONG iman = AROS_LE2LONG(xhciir->iman), tmp;
    xhciir->iman = AROS_LONG2LE(XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP);
    tmp = AROS_LE2LONG(xhciir->iman);

    if ((iman & (XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP)) == (XHCIF_IR_IMAN_IE | XHCIF_IR_IMAN_IP))
    {
        volatile struct pcisusbXHCIRing *ering = (volatile struct pcisusbXHCIRing *)((IPTR)hc->hc_ERSp);
        volatile struct xhci_trb *etrb;
        ULONG idx = ering->next, cycle = (ering->end & RINGENDCFLAG) ? 1 : 0;
        UWORD maxwork = 10;

        pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "Processing events..." DEBUGCOLOR_RESET" \n");

        for (etrb = &ering->ring[idx]; (maxwork > 0) && xhciTRBCycleMatches(etrb->flags, cycle); maxwork--)
        {
            ULONG trbe_type = (etrb->flags >> TRBS_FLAG_TYPE) & TRB_FLAG_TYPE_SMASK;
            ULONG trbe_ccode = (etrb->tparams >> 24) & 0XFF;

            pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "Event Ring 0x%p[%u] = <%sTRB 0x%p, type %u>\n       slot %u" DEBUGCOLOR_RESET" \n",
                                                                                                            ering, idx,
                                                                                                            (etrb->flags & (1 << 2)) ? "Event " : "",
                                                                                                            etrb, trbe_type, ((etrb->flags >> 24) & 0xFF));
            xhciDumpCC(trbe_ccode);

            switch (trbe_type)
            {
            case TRBB_FLAG_ERTYPE_PORT_STATUS_CHANGE:
                {
#if __WORDSIZE == 64
                    struct xhci_trb  *txtrb = (struct xhci_trb  *)(((IPTR)etrb->dbp.addr_hi << 32) | (IPTR)etrb->dbp.addr_lo);
#else
                    struct xhci_trb  *txtrb = (struct xhci_trb  *)etrb->dbp.addr_lo;
#endif
                    volatile struct xhci_pr *xhciports = (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);
                    volatile struct xhci_trb_port_status *evt = (volatile struct xhci_trb_port_status *)&ering->current;

                    UWORD hciport;

                    ULONG origportsc, newportsc = 0;

                    *evt = *(volatile struct xhci_trb_port_status *)etrb;

                    pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "Port Status Change detected <Port=#%u>\nPort Status Change TRB = <%p>" DEBUGCOLOR_RESET" \n", evt->port, txtrb);
                    hciport = evt->port - 1;

                    xhciDumpPort(&xhciports[hciport]);
                    origportsc = AROS_LE2LONG(xhciports[hciport].portsc);

                    // reflect port ownership (shortcut without hc->hc_PortNum[evt->port], as usb 2.0 maps 1:1)
                    hc->hc_Unit->hu_PortOwner[hciport] = HCITYPE_XHCI;

                    if(origportsc & XHCIF_PR_PORTSC_OCC)
                    {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
                        newportsc |= XHCIF_PR_PORTSC_OCC;
                    }
                    if(origportsc & XHCIF_PR_PORTSC_PRC)
                    {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
                        newportsc |= XHCIF_PR_PORTSC_PRC;
                    }
                    if(origportsc & XHCIF_PR_PORTSC_WRC)
                    {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
                        newportsc |= XHCIF_PR_PORTSC_WRC;
                    }
                    if (origportsc & XHCIF_PR_PORTSC_PEC)
                    {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
                        newportsc |= XHCIF_PR_PORTSC_PEC;
                    }
                    if(origportsc & XHCIF_PR_PORTSC_CSC)
                    {
                        hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                        newportsc |= XHCIF_PR_PORTSC_CSC;
                    }
                    if(origportsc & XHCIF_PR_PORTSC_PLC)
                        newportsc |= XHCIF_PR_PORTSC_PLC;
                    if(origportsc & XHCIF_PR_PORTSC_CEC)
                        newportsc |= XHCIF_PR_PORTSC_CEC;

                    xhciports[hciport].portsc = AROS_LONG2LE(newportsc);
                    pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "RH Change $%08lx" DEBUGCOLOR_RESET" \n", hc->hc_PortChangeMap[hciport]);
                    if(hc->hc_PortChangeMap[hciport])
                    {
                        hc->hc_Unit->hu_RootPortChanges |= (1UL << (hciport + 1));
                        if (((origportsc & XHCIF_PR_PORTSC_PED) && (!hc->hc_Devices[hciport])) ||
                            ((!(origportsc & XHCIF_PR_PORTSC_PED)) && (hc->hc_Devices[hciport])))
                        {
                            pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "Signaling port change handler" DEBUGCOLOR_RESET" \n");

                            /* Connect/Disconnect the device */
                            Signal(hc->hc_xHCTask, 1L<<hc->hc_PortChangeSignal);
                        }
                        else
                            checkRHchanges = TRUE;
                    }
                    break;
                }
            case TRBB_FLAG_ERTYPE_TRANSFER:
                {
#if __WORDSIZE == 64
                    struct xhci_trb  *txtrb = (struct xhci_trb  *)(((IPTR)etrb->dbp.addr_hi << 32) | (IPTR)etrb->dbp.addr_lo);
#else
                    struct xhci_trb  *txtrb = (struct xhci_trb  *)etrb->dbp.addr_lo;
#endif
                    struct pcisusbXHCIRing *ring = RINGFROMTRB(txtrb);
                    volatile struct xhci_trb  *evt = &ring->current;
                    ULONG last = txtrb - ring->ring;

                    pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "TRB  = <Ring 0x%p[%u] TRB 0x%p>" DEBUGCOLOR_RESET" \n", ring, last, txtrb);
                    xhciDumpCC(trbe_ccode);

                    *evt = *etrb;
                    ring->end &= RINGENDCFLAG;
                    ring->end |= (last & ~RINGENDCFLAG);

                    doCompletion = xhciIntWorkProcess(hc, (struct IOUsbHWReq *)ring->ringio[last], (evt->tparams & 0XFFFFFF), trbe_ccode);
                    break;
                }
            case TRBB_FLAG_ERTYPE_COMMAND_COMPLETE:
                {
#if __WORDSIZE == 64
                    struct xhci_trb  *txtrb = (struct xhci_trb  *)(((IPTR)etrb->dbp.addr_hi << 32) | (IPTR)etrb->dbp.addr_lo);
#else
                    struct xhci_trb  *txtrb = (struct xhci_trb  *)etrb->dbp.addr_lo;
#endif
                    struct pcisusbXHCIRing *ring = RINGFROMTRB(txtrb);
                    volatile struct xhci_trb  *evt = &ring->current;
                    ULONG last = txtrb - ring->ring;

                    pciusbDebugTRB("xHCI", DEBUGCOLOR_SET "Cmd TRB  = <Cmd Ring 0x%p[%u] TRB 0x%p>" DEBUGCOLOR_RESET" \n", ring, last, txtrb);
                    xhciDumpCC(trbe_ccode);

                    *evt = *etrb;
                    ring->end &= RINGENDCFLAG;
                    ring->end |= (last & ~RINGENDCFLAG);

                    hc->hc_CmdResults[last].flags = evt->flags;
                    hc->hc_CmdResults[last].tparams = evt->tparams;

                    doCompletion = xhciIntWorkProcess(hc, (struct IOUsbHWReq *)ring->ringio[last], (evt->tparams & 0XFFFFFF), trbe_ccode);
                    break;
                }
            default:
                {
                    pciusbDebugTRB("xHCI", DEBUGWARNCOLOR_SET "Unknown event, type %d, completion code %d" DEBUGCOLOR_RESET" \n", trbe_type, trbe_ccode);
                    break;
                }
            }

            ering->end &= RINGENDCFLAG;
            ering->end |= (idx & ~RINGENDCFLAG);

            /* update the hc deque pointer.. */
            volatile struct xhci_ir *ir = (volatile struct xhci_ir *)hc->hc_XHCIIntR;
            xhciSetPointer(hc, ir->erdp, (((IPTR)etrb) | XHCIF_IR_ERDP_EHB));

            /* and adjust the ring index.. */
            if (++idx == USB_DEV_MAX)
            {
                cycle = cycle ? 0 : 1;
                if (cycle)
                    ering->end |= RINGENDCFLAG;
                else
                    ering->end &= ~RINGENDCFLAG;
                idx = 0;
            }
            ering->next = idx;
            etrb = &ering->ring[idx];
        }

        if(doCompletion)
        {
            SureCause(base, &hc->hc_CompleteInt);
        }
    }

    if (checkRHchanges)
        uhwCheckRootHubChanges(hc->hc_Unit);

    pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: Exiting" DEBUGCOLOR_RESET" \n", __func__);

    return FALSE;

    AROS_INTFUNC_EXIT
}

void xhciAbortRequest(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, hc, ioreq);
    Remove(&ioreq->iouh_Req.io_Message.mn_Node);
    ReplyMsg(&ioreq->iouh_Req.io_Message);
}

void xhciReset(struct PCIController *hc, struct PCIUnit *hu)
{
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)hc->hc_XHCIOpR);
    ULONG reg;
    ULONG cnt = 100;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s(0x%p, 0x%p)" DEBUGCOLOR_RESET" \n", __func__, hc, hu);
    
    // Tell the controller to stop if its currently running ...
    reg = AROS_LE2LONG(hcopr->usbcmd);
    if (reg & XHCIF_USBCMD_RS)
    {
        reg &= ~XHCIF_USBCMD_RS;
        hcopr->usbcmd = AROS_LONG2LE(reg);

        // Wait for the controller to indicate it is finished ...
        while ((AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_HCH) && (--cnt > 0))
            uhwDelayMS(1, hu);
    }

    pciusbDebug("xHCI", DEBUGCOLOR_SET "Resetting Controller..." DEBUGCOLOR_RESET" \n");
    hcopr->usbcmd = AROS_LONG2LE(XHCIF_USBCMD_HCRST);

    // Wait for the command to be accepted..
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbcmd) & XHCIF_USBCMD_HCRST) && (--cnt > 0))
        uhwDelayMS(2, hu);

    pciusbDebug("xHCI", DEBUGCOLOR_SET "COMMAND = $%08x, after %ums" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbcmd), (100 - cnt) << 1);

    // Wait for the reset to complete..
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_CNR) && (--cnt > 0))
        uhwDelayMS(2, hu);

    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));
    pciusbDebug("xHCI", DEBUGCOLOR_SET "after %ums" DEBUGCOLOR_RESET" \n", (100 - cnt) << 1);

    pciusbDebug("xHCI", DEBUGCOLOR_SET "Configuring DMA pointers..." DEBUGCOLOR_RESET" \n");

    hcopr->config = AROS_LONG2LE(hc->hc_NumSlots);
    pciusbDebug("xHCI", DEBUGCOLOR_SET "  Setting DCBAA to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMADCBAA);
    xhciSetPointer(hc, hcopr->dcbaap, hc->hc_DMADCBAA);
    xhciDumpStatus(AROS_LE2LONG(hcopr->usbsts));
    pciusbDebug("xHCI", DEBUGCOLOR_SET "  Setting CRCR to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAOPR);
    xhciSetPointer(hc, hcopr->crcr, ((IPTR)hc->hc_DMAOPR | 1));

    volatile struct pcisusbXHCIRing *xring = (volatile struct pcisusbXHCIRing *)hc->hc_OPRp;
    xring->end |= RINGENDCFLAG;

    volatile struct xhci_er_seg *erseg = (volatile struct xhci_er_seg *)hc->hc_ERSTp;
    pciusbDebug("xHCI", DEBUGCOLOR_SET "  Setting Event Segment Pointer to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAERS);
    xhciSetPointer(hc, erseg->ptr, ((IPTR)hc->hc_DMAERS));

    erseg->size = AROS_LONG2LE(USB_DEV_MAX);

    volatile struct xhci_ir *xhciir = (volatile struct xhci_ir *)((IPTR)hc->hc_XHCIIntR);
    xhciir->erstsz = 1;
    pciusbDebug("xHCI", DEBUGCOLOR_SET "  Setting ERDP to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAERS);
    pciusbDebug("xHCI", DEBUGCOLOR_SET "  Setting ERSTBA to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAERST);
    xhciSetPointer(hc, xhciir->erdp, ((IPTR)hc->hc_DMAERS));
    xhciSetPointer(hc, xhciir->erstba, ((IPTR)hc->hc_DMAERST));

    xring = (volatile struct pcisusbXHCIRing *)hc->hc_ERSp;
    xring->end |= RINGENDCFLAG;
    xhciir->iman = XHCIF_IR_IMAN_IE;

    pciusbDebug("xHCI", DEBUGCOLOR_SET "Reset complete..." DEBUGCOLOR_RESET" \n");

    xhciDumpOpR(hcopr);
    xhciDumpIR(xhciir);
}


AROS_UFH0(void, xhciControllerTask)
{
    AROS_USERFUNC_INIT

    volatile struct xhci_pr *xhciports;
    struct PCIController *hc;
    struct Task *thistask;
    struct pcisusbXHCIDevice *devCtx;
    ULONG	sigmask, portsc;
    UWORD	hciport;

    thistask = FindTask(NULL);
    hc = thistask->tc_UserData;

    pciusbDebug("xHCI", DEBUGFUNCCOLOR_SET "%s()" DEBUGCOLOR_RESET" \n", __func__);

    hc->hc_xHCTask = thistask;
    hc->hc_PortChangeSignal = AllocSignal(-1);
    hc->hc_DoWorkSignal = AllocSignal(-1);

    if (hc->hc_ReadySigTask)
        Signal(hc->hc_ReadySigTask, 1L << hc->hc_ReadySignal);

    xhciports = (volatile struct xhci_pr *)((IPTR)hc->hc_XHCIPorts);

    for (;;)
    {
        ULONG xhcictsigs = Wait((1 << hc->hc_DoWorkSignal) | (1 << hc->hc_PortChangeSignal));
#if (1)
        pciusbDebug("xHCI", DEBUGCOLOR_SET "xhciControllerTask @ 0x%p, IDnest %d TDNest %d" DEBUGCOLOR_RESET" \n", thistask, thistask->tc_IDNestCnt, thistask->tc_TDNestCnt);
#endif

        if (xhcictsigs & (1 << hc->hc_DoWorkSignal))
        {
            pciusbDebug("xHCI", DEBUGCOLOR_SET "Processing pending HC work" DEBUGCOLOR_RESET" \n");
            xhciHandleFinishedTDs(hc);

            if(hc->hc_IntXFerQueue.lh_Head->ln_Succ)
            {
                xhciScheduleIntTDs(hc);
            }
            if(hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
            {
                xhciScheduleAsyncTDs(hc, &hc->hc_CtrlXFerQueue, UHCMD_CONTROLXFER);
            }
            if(hc->hc_BulkXFerQueue.lh_Head->ln_Succ)
            {
                xhciScheduleAsyncTDs(hc, &hc->hc_BulkXFerQueue, UHCMD_BULKXFER);
            }
        }
        if (xhcictsigs & (1 << hc->hc_PortChangeSignal))
        {
            for (hciport = 0; hciport < hc->hc_NumPorts; hciport++)
            {
                portsc = AROS_LE2LONG(xhciports[hciport].portsc);
                if ((portsc & XHCIF_PR_PORTSC_PED) && (!hc->hc_Devices[hciport]))
                {
                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Connecting HCI Device on port #%u" DEBUGCOLOR_RESET" \n", hciport+1);
                    devCtx = AllocMem(sizeof(struct pcisusbXHCIDevice), MEMF_ANY|MEMF_CLEAR);
                    if (devCtx)
                    {

                        ULONG maxsize = 8, ctxsize;
                        LONG slotid;
                        UWORD ctxoff = 1;

                        pciusbDebug("xHCI", DEBUGCOLOR_SET "Device Ctx allocated @ 0x%p" DEBUGCOLOR_RESET" \n", devCtx);

                        ctxsize = (MAX_DEVENDPOINTS + 1) * sizeof(struct xhci_inctx);
                        if (hc->hc_Flags & HCF_CTX64)
                        {
                            ctxsize <<= 1;
                            ctxoff <<= 1;
                        }

                        /*
                         * Device/Slot initialization - see Chapter 4.3.3 (page 88) and 4.5.1 (page 96)
                         * Allocate an input context...
                         */
                        
                        devCtx->dc_SlotCtx.dmaa_Ptr = pciAllocAligned(hc, &devCtx->dc_SlotCtx.dmaa_Entry, ctxsize, ALIGN_CTX, xhciPageSize(hc));
                        if (devCtx->dc_SlotCtx.dmaa_Ptr)
                        {
                            pciusbDebug("xHCI", DEBUGCOLOR_SET "Allocated Device Slot Ctx @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", devCtx->dc_SlotCtx.dmaa_Ptr, hc->hc_ERS.me_Un.meu_Addr, hc->hc_ERS.me_Length);
#if (0)
                            devCtx->dc_SlotCtx.dmaa_DMA = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)devCtx->dc_SlotCtx.dmaa_Ptr);
#else
                            devCtx->dc_SlotCtx.dmaa_DMA = devCtx->dc_SlotCtx.dmaa_Ptr;
#endif
                            pciusbDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", devCtx->dc_SlotCtx.dmaa_DMA);
                        }
                        else
                        {
                            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate Device Slot Ctx Memory" DEBUGCOLOR_RESET" \n");
                            //TODO : Free devCtx->dc_IN, and devCtx
                            continue;
                        }

                        devCtx->dc_IN.dmaa_Ptr = pciAllocAligned(hc, &devCtx->dc_IN.dmaa_Entry, ctxsize + sizeof(struct xhci_inctx), ALIGN_CTX, xhciPageSize(hc));
                        if (devCtx->dc_IN.dmaa_Ptr)
                        {
                            pciusbDebug("xHCI", DEBUGCOLOR_SET "Allocated IN @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", devCtx->dc_IN.dmaa_Ptr, devCtx->dc_IN.dmaa_Entry.me_Un.meu_Addr, devCtx->dc_IN.dmaa_Entry.me_Length);
#if (0)
                            devCtx->dc_IN.dmaa_DMA = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)devCtx->dc_IN.dmaa_Ptr,);
#else
                            devCtx->dc_IN.dmaa_DMA = devCtx->dc_IN.dmaa_Ptr;
#endif
                            pciusbDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", devCtx->dc_IN.dmaa_DMA);
                        }
                        else
                        {
                            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate device input ctx" DEBUGCOLOR_RESET" \n");
                            //TODO : Free devCtx
                            continue;
                        }
                        volatile const struct xhci_inctx *devctx;
                        volatile struct xhci_slot *dslot;

                        struct xhci_inctx *inctx = (struct xhci_inctx *)devCtx->dc_IN.dmaa_Ptr;
                        struct xhci_slot *islot = (void*)&inctx[ctxoff];
                        islot->ctx[1] |= (((hciport + 1) & 0xFF) << 16); // Fill in the root hub port number

                        pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: Sending CMD Enable Slot" DEBUGCOLOR_RESET" \n", __func__);
                        slotid = xhciCmdSlotEnable(hc);
                        if (slotid < 0)
                        {
                            pciusbError("xHCI", DEBUGWARNCOLOR_SET "%s: Failed to enable slot!" DEBUGCOLOR_RESET" \n", __func__);
                            //TODO : Free devCtx->dc_Slot, devCtx->dc_IN, and devCtx
                            continue;
                        }
                        pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: slotid = %u" DEBUGCOLOR_RESET" \n", __func__, slotid);
                        ULONG size = (sizeof(struct xhci_slot) * MAX_DEVENDPOINTS);
                        if (hc->hc_Flags & HCF_CTX64)
                            size <<= 1;

                        volatile struct xhci_address *deviceslots = (volatile struct xhci_address *)hc->hc_DCBAAp;
                        xhciSetPointer(hc, deviceslots[slotid], ((IPTR)devCtx->dc_SlotCtx.dmaa_DMA));
                        devCtx->dc_SlotID = slotid;
#if (1)
                        // TODO: Probe port max transfer size
                        devCtx->dx_TxMax = 0;
#endif

                        inctx->acf = 0x01;

                        pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: Device slot configured - configering EP0" DEBUGCOLOR_RESET" \n", __func__);
                        /* initialize endpoint 0 for use .. */
                        ULONG epid = xhciInitEP(hc, devCtx,
                                    0, 0,
                                    UHCMD_CONTROLXFER,
                                    maxsize);

                        /* Pass ownership of the Output Device Context to the xHC */
                        pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: Sending CMD Address Device" DEBUGCOLOR_RESET" \n", __func__);
                        if (1 != xhciCmdDeviceAddress(hc, slotid, devCtx->dc_IN.dmaa_DMA))
                        {
                            pciusbError("xHCI", DEBUGWARNCOLOR_SET "%s: Address Device failed!" DEBUGCOLOR_RESET" \n", __func__);
                            pciusbDebug("xHCI", DEBUGWARNCOLOR_SET "%s: Sending CMD Disable Slot" DEBUGCOLOR_RESET" \n", __func__);
                            if (1 == xhciCmdSlotDisable(hc, slotid))
                            {
                                xhciSetPointer(hc, deviceslots[slotid], (0));
                            }
                            else
                            {
                                pciusbError("xHCI", DEBUGWARNCOLOR_SET "%s: Slot Disable failed!" DEBUGCOLOR_RESET" \n", __func__);
                            }
                            devCtx->dc_SlotID = 0;

                            //TODO : Free Endpoint allocations, devCtx->dc_Slot, devCtx->dc_IN, and devCtx
                            continue;
                        }
                        struct xhci_ep *ep = (struct xhci_ep *)&inctx[ctxoff * (epid + 1)];
                        ep->ctx[1] = ep->ctx[0] = 0;
                        ep->deq.addr_hi = ep->deq.addr_lo = 0;
                        ep->length = 0;
#if (0)
                        //TODO: Set if the device is a hub ..
                        islot->ctx[0] |= (1 << 26);
                        //TODO: If the hub supports Multiple TT's set this ..
                        islot->ctx[0] |= (1 << 25);
#endif
                        hc->hc_Devices[hciport] = devCtx;

                        pciusbDebug("xHCI", DEBUGCOLOR_SET "%s: Device ready!" DEBUGCOLOR_RESET" \n", __func__);
                    }
                    hc->hc_Unit->hu_DevControllers[0] = hc;
                }
                else if ((!(portsc & XHCIF_PR_PORTSC_PED)) && (hc->hc_Devices[hciport]))
                {
                    devCtx = hc->hc_Devices[hciport];
                    hc->hc_Devices[hciport] = NULL;

                    pciusbDebug("xHCI", DEBUGCOLOR_SET "Detaching HCI Device Ctx @ 0x%p" DEBUGCOLOR_RESET" \n", devCtx);

                    //TODO : Free Endpoint allocations, devCtx->dc_Slot, and devCtx->dc_IN

                    FreeMem(devCtx, sizeof(struct pcisusbXHCIDevice));
                }
            }
            uhwCheckRootHubChanges(hc->hc_Unit);
        }
    }    
    AROS_USERFUNC_EXIT
}

BOOL xhciInit(struct PCIController *hc, struct PCIUnit *hu) {

    struct PCIDevice *hd = hu->hu_Device;
    volatile struct xhci_hccapr *xhciregs;
    UBYTE *memptr;
    ULONG xhciUSBLegSup = 0;
    ULONG xhciECPOff;
    ULONG val;
    ULONG cnt;

    struct TagItem pciMemEnableAttrs[] =
    {
            { aHidd_PCIDevice_isMEM,    TRUE },
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    struct TagItem pciDeactivateBusmaster[] =
    {
            { aHidd_PCIDevice_isMaster, FALSE },
            { TAG_DONE, 0UL },
    };

    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = "XHCI CompleteInt";
    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (VOID_FUNC)xhciCompleteInt;

    // Initialize hardware...
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base0, (IPTR *) &hc->hc_RegBase);
    xhciregs = (volatile struct xhci_hccapr *)hc->hc_RegBase;

    pciusbDebug("xHCI", DEBUGCOLOR_SET "CAPLENGTH: 0x%02x" DEBUGCOLOR_RESET" \n", xhciregs->caplength);
    pciusbDebug("xHCI", DEBUGCOLOR_SET "DBOFF: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->dboff));
    pciusbDebug("xHCI", DEBUGCOLOR_SET "RRSOFF: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->rrsoff));

    hc->hc_XHCIOpR = (APTR)((IPTR)xhciregs + xhciregs->caplength);
    pciusbDebug("xHCI", DEBUGCOLOR_SET "Operational Registers @ 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_XHCIOpR);
    hc->hc_XHCIDB = (APTR)((IPTR)xhciregs + AROS_LE2LONG(xhciregs->dboff));
    pciusbDebug("xHCI", DEBUGCOLOR_SET "Doorbells @ 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_XHCIDB);
    hc->hc_XHCIPorts = (APTR)((IPTR)hc->hc_XHCIOpR + 0x400);
    pciusbDebug("xHCI", DEBUGCOLOR_SET "Port Registers @ 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_XHCIPorts);
    hc->hc_XHCIIntR = (APTR)((IPTR)xhciregs + AROS_LE2LONG(xhciregs->rrsoff) + 0x20);
    pciusbDebug("xHCI", DEBUGCOLOR_SET "Interrupt Registers @ 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_XHCIIntR);

    OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciMemEnableAttrs); // activate memory

    xhciECPOff = (AROS_LE2LONG(xhciregs->hcsparams2) >> XHCIS_HCCPARAMS1_ECP) & XHCI_HCCPARAMS1_ECP_SMASK;
    pciusbDebug("xHCI", DEBUGCOLOR_SET "Extended Capabilties Pointer = %04x" DEBUGCOLOR_RESET" \n", xhciECPOff);
    if (xhciECPOff >= 0x40)
    {
        xhciUSBLegSup = READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff);
        pciusbDebug("xHCI", DEBUGCOLOR_SET "xhciUSBLegSup = $%08x" DEBUGCOLOR_RESET" \n", xhciUSBLegSup);
        if (xhciUSBLegSup & XHCIF_USBLEGSUP_BIOSOWNED)
        {
            ULONG cnt, ownershipval = xhciUSBLegSup | XHCIF_USBLEGSUP_OSOWNED;

            pciusbDebug("xHCI", DEBUGCOLOR_SET "Taking ownership of XHCI from BIOS" DEBUGCOLOR_RESET" \n");
takeownership:
            cnt = 100;
            /*
                 * Change the ownership flag and read back to ensure it is written
                 */
            WRITECONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff, ownershipval);
            READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff);

            /* 
                 * Wait for ownership change to take place.
                 * XHCI specification doesn't say how long it can take...
                 */
            while ((READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff) & XHCIF_USBLEGSUP_BIOSOWNED) && (--cnt > 0))
            {
                pciusbDebug("xHCI", DEBUGCOLOR_SET "Waiting for ownership to change..." DEBUGCOLOR_RESET" \n");
                uhwDelayMS(10, hu);
            }
            if ((ownershipval != XHCIF_USBLEGSUP_OSOWNED) &&
                (READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff) & XHCIF_USBLEGSUP_BIOSOWNED))
            {
                pciusbDebug("xHCI", DEBUGCOLOR_SET "Ownership of XHCI still with BIOS" DEBUGCOLOR_RESET" \n");

                /* Try to force ownership */
                ownershipval = XHCIF_USBLEGSUP_OSOWNED;
                goto takeownership;
            }
        }
        else if (xhciUSBLegSup & XHCIF_USBLEGSUP_OSOWNED)
        {
            pciusbDebug("xHCI", DEBUGCOLOR_SET "Ownership already with OS!" DEBUGCOLOR_RESET" \n");
        }
        else
        {
            pciusbDebug("xHCI", DEBUGCOLOR_SET "Forcing ownership of XHCI from (unknown)" DEBUGCOLOR_RESET" \n");
                /* Try to force ownership */
            WRITECONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff, XHCIF_USBLEGSUP_OSOWNED);
            READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff);
        }

        /* Clear the SMI control bits */
        WRITECONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff + 4, 0);
        READCONFIGLONG(hc, hc->hc_PCIDeviceObject, xhciECPOff + 4);
    }

    pciusbDebug("xHCI", DEBUGCOLOR_SET "HCIVERSION: 0x%04x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->hciversion));
    pciusbDebug("xHCI", DEBUGCOLOR_SET "HCSPARAMS1: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->hcsparams1));
    pciusbDebug("xHCI", DEBUGCOLOR_SET "HCSPARAMS2: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->hcsparams2));
    pciusbDebug("xHCI", DEBUGCOLOR_SET "HCSPARAMS3: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->hcsparams3));
    pciusbDebug("xHCI", DEBUGCOLOR_SET "HCCPARAMS: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->hcsparams3));

    ULONG hcsparams1 = AROS_LE2LONG(xhciregs->hcsparams1);
    hc->hc_NumPorts = (ULONG)((hcsparams1 >> 24) & 0XFF);
    hc->hc_NumSlots = (ULONG)(hcsparams1 & 0XFF);

    pciusbDebug("xHCI", DEBUGCOLOR_SET "%d ports, %d slots" DEBUGCOLOR_RESET" \n", hc->hc_NumPorts, hc->hc_NumSlots);

    ULONG hccparams1 = AROS_LE2LONG(xhciregs->hccparams1);
    if (hccparams1 & XHCIF_HCCPARAMS1_CSZ)
        hc->hc_Flags |= HCF_CTX64;
    if (hccparams1 & XHCIF_HCCPARAMS1_AC64)
        hc->hc_Flags |= HCF_ADDR64;
    if (hccparams1 & XHCIF_HCCPARAMS1_PPC)
        hc->hc_Flags |= HCF_PPC;

    pciusbDebug("xHCI", DEBUGCOLOR_SET "%d byte context(s), %ubit addressing" DEBUGCOLOR_RESET" \n", (hc->hc_Flags & HCF_CTX64) ? 64 : 32, (hc->hc_Flags & HCF_ADDR64) ? 64 : 32);

    // Device Context Base Address Array (Chapter 6.1)
    hc->hc_DCBAAp = pciAllocAligned(hc, &hc->hc_DCBAA, sizeof(UQUAD) * (hc->hc_NumSlots + 1), ALIGN_DCBAA, xhciPageSize(hc));
    if (hc->hc_DCBAAp)
    {
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Allocated DCBAA @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", hc->hc_DCBAAp, hc->hc_DCBAA.me_Un.meu_Addr, hc->hc_DCBAA.me_Length);
#if (0)
        hc->hc_DMADCBAA = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)hc->hc_DCBAAp);
#else
        hc->hc_DMADCBAA = hc->hc_DCBAAp;
#endif
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMADCBAA);
    }
    else
    {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate DCBAA DMA Memory" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

    // Event Ring Segment Table (Chapter 6.5)
    hc->hc_ERSTp = pciAllocAligned(hc, &hc->hc_ERST, sizeof(struct xhci_er_seg), ALIGN_EVTRING_TBL, ALIGN_EVTRING_TBL);
    if (hc->hc_ERSTp)
    {
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Allocated ERST @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", hc->hc_ERSTp, hc->hc_ERST.me_Un.meu_Addr, hc->hc_ERST.me_Length);
#if (0)
        hc->hc_DMAERST = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)hc->hc_ERSTp);
#else
        hc->hc_DMAERST = hc->hc_ERSTp;
#endif
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAERST);
    }
    else
    {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate ERST DMA Memory" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

    hc->hc_OPRp = pciAllocAligned(hc, &hc->hc_OPR, sizeof(struct pcisusbXHCIRing), ALIGN_CMDRING_SEG, (1 << 16));
    if (hc->hc_OPRp)
    {
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Allocated OPR @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", hc->hc_OPRp, hc->hc_OPR.me_Un.meu_Addr, hc->hc_OPR.me_Length);
#if (0)
        hc->hc_DMAOPR = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)hc->hc_OPRp);
#else
        hc->hc_DMAOPR = hc->hc_OPRp;
#endif
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAOPR);
    }
    else
    {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate OPR DMA Memory" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

    hc->hc_ERSp = pciAllocAligned(hc, &hc->hc_ERS, sizeof(struct pcisusbXHCIRing), ALIGN_EVTRING_SEG, (1 << 16));
    if (hc->hc_ERSp)
    {
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Allocated ERS @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n", hc->hc_ERSp, hc->hc_ERS.me_Un.meu_Addr, hc->hc_ERS.me_Length);
#if (0)
        hc->hc_DMAERS = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)hc->hc_ERSp);
#else
        hc->hc_DMAERS = hc->hc_ERSp;
#endif
        pciusbDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", hc->hc_DMAERS);
    }
    else
    {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate ERS DMA Memory" DEBUGCOLOR_RESET" \n");
        return FALSE;
    }

#if (1)
    val = AROS_LE2LONG(xhciregs->hcsparams2);
    pciusbDebug("xHCI", DEBUGCOLOR_SET "SPB = %u" DEBUGCOLOR_RESET" \n", (val >> 21 & 0x1f) << 5 | val >> 27);
#else
  /* Chapter 4.20 Scratchpad Buffers */
  reg = grub_xhci_read32(&x->caps->hcsparams2);
  x->spb = (reg >> 21 & 0x1f) << 5 | reg >> 27;
  if (x->spb)
    {
      volatile grub_uint64_t *spba;
      grub_dprintf("xhci", "XHCI init: set up %d scratch pad buffers" DEBUGCOLOR_RESET" \n",
                  x->spb);
      x->spba_dma = xhci_memalign_dma32(ALIGN_SPBA, sizeof(*spba) * x->spb,
                                       x->pagesize);
      if (!x->spba_dma)
       goto fail;

      x->spad_dma = xhci_memalign_dma32(x->pagesize, x->pagesize * x->spb,
                                       x->pagesize);
      if (!x->spad_dma)
       {
         grub_dma_free(x->spba_dma);
         goto fail;
       }

      spba = grub_dma_get_virt(x->spba_dma);
      for (ULONG i = 0; i < x->spb; i++)
       spba[i] = (grub_addr_t)grub_dma_get_phys(x->spad_dma) + (i * x->pagesize);
      grub_arch_sync_dma_caches(x->spba_dma, sizeof(*spba) * x->spb);

      x->devs[0].ptr_low = grub_dma_get_phys(x->spba_dma);
      x->devs[0].ptr_high = 0;
      grub_arch_sync_dma_caches(x->devs_dma, sizeof(x->devs[0]));
      grub_dprintf ("xhci", "XHCI init: Allocated %d scratch buffers of size 0x%x" DEBUGCOLOR_RESET" \n",
                   x->spb, x->pagesize);
    }
#endif

    // install reset handler
    hc->hc_ResetInt.is_Node.ln_Name = "XHCI PCI (pciusb.device)";
    hc->hc_ResetInt.is_Code = (VOID_FUNC)XhciResetHandler;
    hc->hc_ResetInt.is_Data = hc;
    AddResetCallback(&hc->hc_ResetInt);

#if defined(DEBUG) && (DEBUG > 0)
    IPTR PCIIntLine = 0;
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_INTLine, &PCIIntLine);
    pciusbDebug("xHCI", DEBUGCOLOR_SET "IRQ = %u" DEBUGCOLOR_RESET" \n",PCIIntLine);
#endif
    // add interrupt
    hc->hc_PCIIntHandler.is_Node.ln_Name = hc->hc_ResetInt.is_Node.ln_Name;
    hc->hc_PCIIntHandler.is_Node.ln_Pri = 5;
    hc->hc_PCIIntHandler.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_PCIIntHandler.is_Code = (VOID_FUNC)xhciIntCode;
    hc->hc_PCIIntHandler.is_Data = hc;
    PCIXAddInterrupt(hc, &hc->hc_PCIIntHandler);

    for(cnt = 0; cnt < hc->hc_NumPorts; cnt++) {
        hu->hu_PortMapX[cnt] = hc;
        hc->hc_PortNum[cnt] = cnt;
    }

    xhciReset(hc, hu);

    /* Enable interrupts in the xhci */
    volatile struct xhci_hcopr *hcopr = (volatile struct xhci_hcopr *)((IPTR)hc->hc_XHCIOpR);
    val = AROS_LE2LONG(hcopr->usbcmd);
    val |= XHCIF_USBCMD_INTE;
    hcopr->usbcmd = AROS_LONG2LE(val);
    pciusbDebug("xHCI", DEBUGCOLOR_SET "USBCMD = $%08x..." DEBUGCOLOR_RESET" \n",AROS_LE2LONG(hcopr->usbcmd));

    // Wait for the command to be accepted..
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbcmd) & XHCIF_USBCMD_INTE) && (--cnt > 0))
        uhwDelayMS(2, hu);

    pciusbDebug("xHCI", DEBUGCOLOR_SET "COMMAND = $%08x, after %ums" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(hcopr->usbcmd), (100 - cnt) << 1);

    // Wait for the controller to finish..
    cnt = 100;
    while ((AROS_LE2LONG(hcopr->usbsts) & XHCIF_USBSTS_CNR) && (--cnt > 0))
        uhwDelayMS(2, hu);

#if (1)
    struct Library *ps;
    ULONG sigmask = SIGF_SINGLE;
    hc->hc_ReadySignal = SIGB_SINGLE;
    hc->hc_ReadySigTask = FindTask(NULL);
    SetSignal(0, sigmask);
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        struct Task *tmptask;
        if((tmptask = psdSpawnSubTask("xHCI Controller Task", xhciControllerTask, hc)))
        {
            sigmask = Wait(sigmask);
            pciusbDebug("xHCI", DEBUGCOLOR_SET "Port Task @ 0x%p, Sig = %u" DEBUGCOLOR_RESET" \n", hc->hc_xHCTask, hc->hc_PortChangeSignal);
        }
    }
    hc->hc_ReadySigTask = NULL;
#endif
    /* Enable the interrupter to generate interupts */
    volatile struct xhci_ir *xhciir = (volatile struct xhci_ir *)((IPTR)hc->hc_XHCIIntR);
    xhciir->iman = XHCIF_IR_IMAN_IE;

    /* Finally, set the "run" bit */
    val = AROS_LE2LONG(hcopr->usbcmd);
    val |= XHCIF_USBCMD_RS | XHCIF_USBCMD_INTE;
    hcopr->usbcmd = AROS_LONG2LE(val);
    pciusbDebug("xHCI", DEBUGCOLOR_SET "USBCMD = $%08x..." DEBUGCOLOR_RESET" \n",AROS_LE2LONG(hcopr->usbcmd));

    pciusbDebug("xHCI", DEBUGCOLOR_SET "xhciInit returns TRUE..." DEBUGCOLOR_RESET" \n");
    return TRUE;
}

void xhciFree(struct PCIController *hc, struct PCIUnit *hu) {
}
#endif /* PCIUSB_ENABLEXHCI */
