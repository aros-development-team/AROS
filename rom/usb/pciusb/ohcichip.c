/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved
    $Id$
*/

/* Enable debug level 1000, keeps an eye on TD DoneQueue consistency */
#define DEBUG 1
#define DB_LEVEL 1000

#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/pci.h>
#include <devices/usb_hub.h>

#include <stddef.h>

#include "uhwcmd.h"
#include "ohciproto.h"

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase (hd->hd_HiddPCIDeviceAB)
#undef HiddAttrBase
#define HiddAttrBase (hd->hd_HiddAB)

#ifdef DEBUG_TD

static void PrintTD(const char *txt, ULONG ptd, struct PCIController *hc)
{
    KPrintF("HC 0x%p %s TD list:", hc, txt);

    while (ptd)
    {
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
    struct OhciTD *otd;

    while (nextotd)
    {
        KPRINTF(1, ("FreeTD %p\n", nextotd));
        otd = nextotd;
        nextotd = (struct OhciTD *) otd->otd_Succ;
        ohciFreeTD(hc, otd);
    }
}

static void ohciFreeEDContext(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct OhciED *oed = ioreq->iouh_DriverPrivate1;
    UWORD devadrep;
    UWORD dir;

    KPRINTF(5, ("Freeing EDContext 0x%p IOReq 0x%p\n", oed, ioreq));

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

static void ohciUpdateIntTree(struct PCIController *hc)
{
    struct OhciED *oed;
    struct OhciED *predoed;
    struct OhciED *lastusedoed;
    UWORD cnt;

    // optimize linkage between queue heads
    predoed = lastusedoed = hc->hc_OhciTermED;
    for(cnt = 0; cnt < 5; cnt++)
    {
        oed = hc->hc_OhciIntED[cnt];
        if(oed->oed_Succ != predoed)
        {
            lastusedoed = oed->oed_Succ;
        }
        oed->oed_NextED = lastusedoed->oed_Self;
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);
        predoed = oed;
    }
}

static void ohciHandleFinishedTDs(struct PCIController *hc)
{
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

    KPRINTF(1, ("Checking for work done...\n"));
    Disable();
    donehead = hc->hc_OhciDoneQueue;
    hc->hc_OhciDoneQueue = 0UL;
    Enable();
    if(!donehead)
    {
        KPRINTF(1, ("Nothing to do!\n"));
        return;
    }
    otd = (struct OhciTD *) ((IPTR)donehead - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
    KPRINTF(10, ("DoneHead=%08lx, OTD=%p, Frame=%ld\n", donehead, otd, READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));
    PrintTD("Done", donehead, hc); /* CHECKME: This can give inconsistent printout on cache-incoherent hardware */
    do
    {
        CacheClearE(&otd->otd_Ctrl, 16, CACRF_InvalidateD);
        oed = otd->otd_ED;
        if(!oed)
        {
            /*
             * WATCH OUT!!! Rogue TD is a very bad thing!!!
             * If you see this, there's definitely a bug in DoneQueue processing flow.
             * See below for the complete description.
             */
            KPRINTF(1000, ("Came across a rogue TD 0x%p that already has been freed!\n", otd));
            nexttd = READMEM32_LE(&otd->otd_NextTD) & OHCI_PTRMASK;
            if(!nexttd)
            {
                break;
            }
            otd = (struct OhciTD *) ((IPTR)nexttd - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
            continue;
        }
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_InvalidateD);
        ctrlstatus = READMEM32_LE(&otd->otd_Ctrl);
        KPRINTF(1, ("TD: %08lx - %08lx\n", READMEM32_LE(&otd->otd_BufferPtr),
        		READMEM32_LE(&otd->otd_BufferEnd)));
        if(otd->otd_BufferPtr)
        {
            // FIXME this will blow up if physical memory is ever going to be discontinuous
            len = READMEM32_LE(&otd->otd_BufferPtr) - (READMEM32_LE(&otd->otd_BufferEnd) + 1 - otd->otd_Length);
        } else {
            len = otd->otd_Length;
        }

        ioreq = oed->oed_IOReq;

        KPRINTF(1, ("Examining TD %p for ED %p (IOReq=%p), Status %08lx, len=%ld\n", otd, oed, ioreq, ctrlstatus, len));
        if(!ioreq)
        {
            /* You should never see this (very weird inconsistency), but who knows... */
            KPRINTF(1000, ("Came across a rogue ED 0x%p that already has been replied! TD 0x%p,\n", oed, otd));
            nexttd = READMEM32_LE(&otd->otd_NextTD) & OHCI_PTRMASK;
            if(!nexttd)
            {
                break;
            }
            otd = (struct OhciTD *) ((IPTR)nexttd - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
            continue;
        }

        if (len)
        {
            epcaps = READMEM32_LE(&oed->oed_EPCaps);
            direction_in = ((epcaps & OECM_DIRECTION) == OECF_DIRECTION_TD)
                        ? (ioreq->iouh_SetupData.bmRequestType & URTF_IN)
                        : (epcaps & OECF_DIRECTION_IN);
            CachePostDMA((APTR)(IPTR)READMEM32_LE(&otd->otd_BufferEnd) - len + 1, &len, direction_in ? 0 : DMA_ReadFromRAM);
        }

        ioreq->iouh_Actual += len;
/*
 * CHECKME: This condition may get triggered on control transfers even if terminating TD is not processed yet.
 *          (got triggered by MacMini's keyboard, when someone sends control ED with no data payload,
 *	    and some other ED is being done meanwhile (its final packet generated an interrupt).
 *	    In this case the given control ED can be partially done (setup TD is done, term TD is not).
 *	    iouh_Length is 0, and the whole ED is retired, while still being processed by the HC. Next time
 *	    its terminator TD arrives into done queue.
 *	    This can cause weird things like looping TD list on itself. My modification of ohciFreeTD()
 *	    explicitly clears NextTD to avoid keeping dangling value there, however the problem still can
 *	    appear if this TD is quickly reused by another request.
 *	    Final TDs have OTCM_DELAYINT fields set to zero. HC processes TDs in order, so if we receive
 *	    the final TD, we assume the whole ED's list has been processed.
 *	    This means it should be safe to simply disable this check.
 *	    If this doesn't work for some reason, we need a more complex check which makes sure that all TDs
 *	    are really done (or ED is halted). This can be done by checking OTCM_COMPLETIONCODE field against
 *	    OTCF_CC_INVALID value.
 *						Pavel Fedin <pavel.fedin@mail.ru>
	retire = (ioreq->iouh_Actual == ioreq->iouh_Length);
        if (retire)
        {
            KPRINTF(10, ("TD 0x%p Data transfer done (%lu bytes)\n", otd, ioreq->iouh_Length));
        } */
        retire = FALSE;
        if((ctrlstatus & OTCM_DELAYINT) != OTCF_NOINT)
        {
            KPRINTF(10, ("TD 0x%p Terminator detected\n", otd));
            retire = TRUE;
        }
        switch((ctrlstatus & OTCM_COMPLETIONCODE)>>OTCS_COMPLETIONCODE)
        {
            case (OTCF_CC_NOERROR>>OTCS_COMPLETIONCODE):
                break;

            case (OTCF_CC_CRCERROR>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("CRC Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                /*
                 * CHECKME: Do we really need to set retire flag here?
                 *	    Critical errors are always accompanied by OEHF_HALTED bit.
                 *	    But what if HC thinks it's recoverable error and continues
                 *	    working on this ED? In this case early retirement happens,
                 *	    causing bad things. See long explanation above.
                 */
                retire = TRUE;
                break;

            case (OTCF_CC_BABBLE>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Babble/Bitstuffing Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_WRONGTOGGLE>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Data toggle mismatch length = %ld\n", len));
                break;

            case (OTCF_CC_STALL>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("STALLED!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                retire = TRUE;
                break;

            case (OTCF_CC_TIMEOUT>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("TIMEOUT!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                retire = TRUE;
                break;

            case (OTCF_CC_PIDCORRUPT>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("PID Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_WRONGPID>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Illegal PID!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_OVERFLOW>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Overflow Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                retire = TRUE;
                break;

            case (OTCF_CC_SHORTPKT>>OTCS_COMPLETIONCODE):
                KPRINTF(10, ("Short packet %ld < %ld\n", len, otd->otd_Length));
                if((!ioreq->iouh_Req.io_Error) && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS)))
                {
                    ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                }
                retire = TRUE;
                break;

            case (OTCF_CC_OVERRUN>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Data Overrun Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_UNDERRUN>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Data Underrun Error!\n"));
                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                retire = TRUE;
                break;

            case (OTCF_CC_INVALID>>OTCS_COMPLETIONCODE):
                KPRINTF(200, ("Not touched?!?\n"));
                break;
        }
        if(READMEM32_LE(&oed->oed_HeadPtr) & OEHF_HALTED)
        {
            KPRINTF(100, ("OED halted!\n"));
            retire = TRUE;
        }

        if(retire)
        {
            KPRINTF(50, ("ED 0x%p stopped at TD 0x%p\n", oed, otd));
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            AddHead(&hc->hc_OhciRetireQueue, &ioreq->iouh_Req.io_Message.mn_Node);
        }

	nexttd = READMEM32_LE(&otd->otd_NextTD) & OHCI_PTRMASK;
        KPRINTF(1, ("NextTD=0x%08lx\n", nexttd));
        if(!nexttd)
        {
            break;
        }
        otd = (struct OhciTD *) ((IPTR)nexttd - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
        KPRINTF(1, ("NextOTD = %p\n", otd));
    } while(TRUE);

    ioreq = (struct IOUsbHWReq *) hc->hc_OhciRetireQueue.lh_Head;
    while((nextioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ))
    {
        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        oed = (struct OhciED *) ioreq->iouh_DriverPrivate1;
        if(oed)
        {
            KPRINTF(50, ("HC 0x%p Retiring IOReq=0x%p Command=%ld ED=0x%p, Frame=%ld\n", hc, ioreq, ioreq->iouh_Req.io_Command, oed, READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));

            if(oed->oed_Continue)
            {
                ULONG actual = ioreq->iouh_Actual;
                ULONG oldenables;
                ULONG phyaddr;
                struct OhciTD *predotd = NULL;

                KPRINTF(10, ("Reloading Bulk transfer at %ld of %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                otd = oed->oed_FirstTD;

                phyaddr = (IPTR)pciGetPhysical(hc, oed->oed_Buffer + actual);
                do
                {
                    len = ioreq->iouh_Length - actual;
                    if(len > OHCI_PAGE_SIZE)
                    {
                        len = OHCI_PAGE_SIZE;
                    }
                    if((!otd->otd_Succ) && (actual + len == ioreq->iouh_Length) && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) && ((actual % ioreq->iouh_MaxPktSize) == 0))
                    {
                        // special case -- zero padding would not fit in this run,
                        // and next time, we would forget about it. So rather abort
                        // reload now, so the zero padding goes with the next reload
                        break;
                    }
                    predotd = otd;
                    otd->otd_Length = len;
                    KPRINTF(1, ("TD with %ld bytes: %08x-%08x\n", len, phyaddr, phyaddr+len-1));
                    CONSTWRITEMEM32_LE(&otd->otd_Ctrl, OTCF_CC_INVALID|OTCF_NOINT);
                    if(otd->otd_Succ)
                    {
                        otd->otd_NextTD = otd->otd_Succ->otd_Self;
                    }
                    if(len)
                    {
                        WRITEMEM32_LE(&otd->otd_BufferPtr, (IPTR)CachePreDMA((APTR)(IPTR)phyaddr, &len, (ioreq->iouh_Dir == UHDIR_IN) ? 0 : DMA_ReadFromRAM));
                        phyaddr += len - 1;
                        WRITEMEM32_LE(&otd->otd_BufferEnd, phyaddr);
                        phyaddr++;
                    } else {
                        CONSTWRITEMEM32_LE(&otd->otd_BufferPtr, 0);
                        CONSTWRITEMEM32_LE(&otd->otd_BufferEnd, 0);
                    }
                    CacheClearE(&otd->otd_Ctrl, 16, CACRF_ClearD);
                    actual += len;
                    otd = otd->otd_Succ;
                } while(otd && ((actual < ioreq->iouh_Length) || (len && (ioreq->iouh_Dir == UHDIR_OUT) && (actual == ioreq->iouh_Length) && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) && ((actual % ioreq->iouh_MaxPktSize) == 0))));
                oed->oed_Continue = (actual < ioreq->iouh_Length);
                predotd->otd_NextTD = hc->hc_OhciTermTD->otd_Self;

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

                ohciFreeEDContext(hc, ioreq);
                if(ioreq->iouh_Req.io_Command == UHCMD_INTXFER)
                {
                    updatetree = TRUE;
                }
                // check for successful clear feature and set address ctrl transfers
                if((!ioreq->iouh_Req.io_Error) && (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER))
                {
                    uhwCheckSpecialCtrlTransfers(hc, ioreq);
                }
                ReplyMsg(&ioreq->iouh_Req.io_Message);
            }
        } else {
            KPRINTF(20, ("IOReq=%p has no OED!\n", ioreq));
        }
        ioreq = nextioreq;
    }
    if(updatetree)
    {
        ohciUpdateIntTree(hc);
    }
}

static ULONG ohciHandleAbortedEDs(struct PCIController *hc)
{
    struct IOUsbHWReq *ioreq;
    ULONG restartmask = 0;

    KPRINTF(50, ("Processing abort queue...\n"));

    // We don't need this any more
    ohciDisableInt(hc, OISF_SOF);

    /*
     * If the aborted IORequest was replied in ohciHandleFinishedTDs(),
     * it was already Remove()d from this queue. It's safe to do no checks.
     * io_Error was set earlier.
     */
    while ((ioreq = (struct IOUsbHWReq *)RemHead(&hc->hc_AbortQueue)))
    {
    	KPRINTF(70, ("HC 0x%p Aborted IOReq 0x%p\n", hc, ioreq));
	PrintED("Aborted", ioreq->iouh_DriverPrivate1, hc);

	ohciFreeEDContext(hc, ioreq);
	ReplyMsg(&ioreq->iouh_Req.io_Message);
    }

    /* Restart stopped queues */
    if (hc->hc_Flags & HCF_STOP_CTRL)
    {
    	KPRINTF(50, ("Restarting control transfers\n"));
    	CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CTRL_ED, 0);
    	restartmask |= OCSF_CTRLENABLE;
    }

    if (hc->hc_Flags & HCF_STOP_BULK)
    {
	KPRINTF(50, ("Restarting bulk transfers\n"));
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
    ULONG phyaddr;
    ULONG oldenables;
    ULONG startmask = 0;

    /* *** CTRL Transfers *** */
    KPRINTF(1, ("Scheduling new CTRL transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_CtrlXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint;
        KPRINTF(10, ("New CTRL transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        oed = ohciAllocED(hc);
        if(!oed)
        {
            break;
        }

        setupotd = ohciAllocTD(hc);
        if(!setupotd)
        {
            ohciFreeED(hc, oed);
            break;
        }
        termotd = ohciAllocTD(hc);
        if(!termotd)
        {
            ohciFreeTD(hc, setupotd);
            ohciFreeED(hc, oed);
            break;
        }
        oed->oed_IOReq = ioreq;

        KPRINTF(1, ("SetupTD=%p, TermTD=%p\n", setupotd, termotd));

        // fill setup td
        epcaps = (ioreq->iouh_DevAddr<<OECS_DEVADDR)|(ioreq->iouh_Endpoint<<OECS_ENDPOINT)|(ioreq->iouh_MaxPktSize<<OECS_MAXPKTLEN)|OECF_DIRECTION_TD;

        if(ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            epcaps |= OECF_LOWSPEED;
        }

        WRITEMEM32_LE(&oed->oed_EPCaps, epcaps);

        oed->oed_TailPtr = hc->hc_OhciTermTD->otd_Self;
        oed->oed_HeadPtr = setupotd->otd_Self;
        oed->oed_FirstTD = setupotd;

        setupotd->otd_ED = oed;
        setupotd->otd_Length = 0; // don't increase io_Actual for that transfer
        CONSTWRITEMEM32_LE(&setupotd->otd_Ctrl, OTCF_PIDCODE_SETUP|OTCF_CC_INVALID|OTCF_NOINT);
        len = 8;

        /* CHECKME: As i can understand, setup packet is always sent TO the device. Is this true? */
        oed->oed_SetupData = usbGetBuffer(&ioreq->iouh_SetupData, len, UHDIR_OUT);
        WRITEMEM32_LE(&setupotd->otd_BufferPtr, (IPTR) CachePreDMA(pciGetPhysical(hc, oed->oed_SetupData), &len, DMA_ReadFromRAM));
        WRITEMEM32_LE(&setupotd->otd_BufferEnd, (IPTR) pciGetPhysical(hc, ((UBYTE *)oed->oed_SetupData) + 7));

        KPRINTF(1, ("TD send: %08lx - %08lx\n", READMEM32_LE(&setupotd->otd_BufferPtr),
        		READMEM32_LE(&setupotd->otd_BufferEnd)));

        ctrl = (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? (OTCF_PIDCODE_IN|OTCF_CC_INVALID|OTCF_NOINT) : (OTCF_PIDCODE_OUT|OTCF_CC_INVALID|OTCF_NOINT);

        predotd = setupotd;
        if (ioreq->iouh_Length)
        {
            oed->oed_Buffer = usbGetBuffer(ioreq->iouh_Data, ioreq->iouh_Length, (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT);
            phyaddr = (IPTR)pciGetPhysical(hc, oed->oed_Buffer);
            actual = 0;
            do
            {
                dataotd = ohciAllocTD(hc);
                if(!dataotd)
                {
                    predotd->otd_Succ = NULL;
                    break;
                }
                dataotd->otd_ED = oed;
                predotd->otd_Succ = dataotd;
                predotd->otd_NextTD = dataotd->otd_Self;
                len = ioreq->iouh_Length - actual;
                if(len > OHCI_PAGE_SIZE)
                {
                    len = OHCI_PAGE_SIZE;
                }
                dataotd->otd_Length = len;
                KPRINTF(1, ("TD with %ld bytes\n", len));
                WRITEMEM32_LE(&dataotd->otd_Ctrl, ctrl);
                /*
                 * CHECKME: Here and there we feed phyaddr to CachePreDMA(), however it expects a logical address.
                 * Perhaps the whole thing works only because HIDD_PCIDriver_CPUtoPCI() actually doesn't do any
                 * translation.
                 */
                WRITEMEM32_LE(&dataotd->otd_BufferPtr, (IPTR)CachePreDMA((APTR)(IPTR)phyaddr, &len, (ioreq->iouh_SetupData.bmRequestType & URTF_IN) ? 0 : DMA_ReadFromRAM));
                phyaddr += len - 1;
                WRITEMEM32_LE(&dataotd->otd_BufferEnd, phyaddr);

                KPRINTF(1, ("TD send: %08lx - %08lx\n", READMEM32_LE(&dataotd->otd_BufferPtr),
                		READMEM32_LE(&dataotd->otd_BufferEnd)));

                CacheClearE(&dataotd->otd_Ctrl, 16, CACRF_ClearD);
                phyaddr++;
                actual += len;
                predotd = dataotd;
            } while(actual < ioreq->iouh_Length);

            if(actual != ioreq->iouh_Length)
            {
                // out of TDs
                KPRINTF(200, ("Out of TDs for Ctrl Transfer!\n"));
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

        ctrl ^= (OTCF_PIDCODE_IN^OTCF_PIDCODE_OUT)|OTCF_NOINT|OTCF_DATA1|OTCF_TOGGLEFROMTD;

        termotd->otd_Length = 0;
        termotd->otd_ED = oed;
        termotd->otd_Succ = NULL;
        termotd->otd_NextTD = hc->hc_OhciTermTD->otd_Self;
        CONSTWRITEMEM32_LE(&termotd->otd_Ctrl, ctrl);
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
        oed->oed_Succ = hc->hc_OhciCtrlTailED;
        oed->oed_NextED = oed->oed_Succ->oed_Self;
        oed->oed_Pred = hc->hc_OhciCtrlTailED->oed_Pred;
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

    if (startmask)
    {
    	/*
    	 * If we are going to start the queue but it's not running yet,
    	 * reset current ED pointer to zero. This will cause the HC to
    	 * start over from the head.
    	 */
        oldenables = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
        if(!(oldenables & OCSF_BULKENABLE))
        {
            CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_ED, 0);
        }
    }

    return startmask;
}

static void ohciScheduleIntTDs(struct PCIController *hc)
{
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
    ULONG phyaddr;

    /* *** INT Transfers *** */
    KPRINTF(1, ("Scheduling new INT transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_IntXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        KPRINTF(10, ("New INT transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        oed = ohciAllocED(hc);
        if(!oed)
        {
            break;
        }

        oed->oed_IOReq = ioreq;

        epcaps = (ioreq->iouh_DevAddr<<OECS_DEVADDR)|(ioreq->iouh_Endpoint<<OECS_ENDPOINT)|(ioreq->iouh_MaxPktSize<<OECS_MAXPKTLEN);
        epcaps |= (ioreq->iouh_Dir == UHDIR_IN) ? OECF_DIRECTION_IN : OECF_DIRECTION_OUT;

        if(ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            epcaps |= OECF_LOWSPEED;
        }

        WRITEMEM32_LE(&oed->oed_EPCaps, epcaps);
        oed->oed_TailPtr = hc->hc_OhciTermTD->otd_Self;

        predotd = NULL;
        oed->oed_Buffer = usbGetBuffer(ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_Dir);
        phyaddr = (IPTR)pciGetPhysical(hc, oed->oed_Buffer);
        actual = 0;
        do
        {
            otd = ohciAllocTD(hc);
            if (predotd)
            	predotd->otd_Succ = otd;
            if (!otd)
            {
                break;
            }
            otd->otd_ED = oed;
            if (predotd)
            {
                predotd->otd_NextTD = otd->otd_Self;
            }
            else
            {
                WRITEMEM32_LE(&oed->oed_HeadPtr, READMEM32_LE(&otd->otd_Self)|(unit->hu_DevDataToggle[devadrep] ? OEHF_DATA1 : 0));
                oed->oed_FirstTD = otd;
            }
            len = ioreq->iouh_Length - actual;
            if(len > OHCI_PAGE_SIZE)
            {
                len = OHCI_PAGE_SIZE;
            }
            otd->otd_Length = len;
            KPRINTF(1, ("Control TD 0x%p with %ld bytes\n", otd, len));
            CONSTWRITEMEM32_LE(&otd->otd_Ctrl, OTCF_CC_INVALID|OTCF_NOINT);
            if(len)
            {
                WRITEMEM32_LE(&otd->otd_BufferPtr, (IPTR)CachePreDMA((APTR)(IPTR)phyaddr, &len, (ioreq->iouh_Dir == UHDIR_IN) ? 0 : DMA_ReadFromRAM));
                phyaddr += len - 1;
                WRITEMEM32_LE(&otd->otd_BufferEnd, phyaddr);
                phyaddr++;
            } else {
                CONSTWRITEMEM32_LE(&otd->otd_BufferPtr, 0);
                CONSTWRITEMEM32_LE(&otd->otd_BufferEnd, 0);
            }
            actual += len;
            CacheClearE(&otd->otd_Ctrl, 16, CACRF_ClearD);
            predotd = otd;
        } while(actual < ioreq->iouh_Length);

        if(actual != ioreq->iouh_Length)
        {
            // out of TDs
            KPRINTF(200, ("Out of TDs for Int Transfer!\n"));
            ohciFreeTDChain(hc, oed->oed_FirstTD);
            usbReleaseBuffer(oed->oed_Buffer, ioreq->iouh_Data, 0, 0);
            ohciFreeED(hc, oed);
            break;
        }
        predotd->otd_Succ = NULL;
        predotd->otd_NextTD = hc->hc_OhciTermTD->otd_Self;

        CONSTWRITEMEM32_LE(&predotd->otd_Ctrl, OTCF_CC_INVALID);
        CacheClearE(&predotd->otd_Ctrl, 16, CACRF_ClearD);

        if(ioreq->iouh_Interval >= 31)
        {
            intoed = hc->hc_OhciIntED[4]; // 32ms interval
        } else {
            UWORD cnt = 0;
            do
            {
                intoed = hc->hc_OhciIntED[cnt++];
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
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD devadrep;
    struct OhciED *oed;
    struct OhciTD *otd;
    struct OhciTD *predotd;
    ULONG actual;
    ULONG epcaps;
    ULONG len;
    ULONG phyaddr;
    ULONG oldenables;
    ULONG startmask = 0;

    /* *** BULK Transfers *** */
    KPRINTF(1, ("Scheduling new BULK transfers...\n"));
    ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        KPRINTF(10, ("New BULK transfer to %ld.%ld: %ld bytes\n", ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }

        oed = ohciAllocED(hc);
        if(!oed)
        {
            break;
        }

        oed->oed_IOReq = ioreq;

        epcaps = (ioreq->iouh_DevAddr<<OECS_DEVADDR)|(ioreq->iouh_Endpoint<<OECS_ENDPOINT)|(ioreq->iouh_MaxPktSize<<OECS_MAXPKTLEN);
        epcaps |= (ioreq->iouh_Dir == UHDIR_IN) ? OECF_DIRECTION_IN : OECF_DIRECTION_OUT;

        if(ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            epcaps |= OECF_LOWSPEED;
        }

        WRITEMEM32_LE(&oed->oed_EPCaps, epcaps);
        oed->oed_TailPtr = hc->hc_OhciTermTD->otd_Self;

        predotd = NULL;
        oed->oed_Buffer = usbGetBuffer(ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_Dir);
        phyaddr = (IPTR)pciGetPhysical(hc, oed->oed_Buffer);
        actual = 0;
        do
        {
            if((actual >= OHCI_TD_BULK_LIMIT) && (actual < ioreq->iouh_Length))
            {
                KPRINTF(10, ("Bulk too large, splitting...\n"));
                break;
            }
            otd = ohciAllocTD(hc);
            if(!otd)
            {
                if(predotd != NULL)
                {
                    predotd->otd_Succ = NULL;
                }
                break;
            }
            otd->otd_ED = oed;
            if(predotd)
            {
                predotd->otd_Succ = otd;
                predotd->otd_NextTD = otd->otd_Self;
            } else {
                WRITEMEM32_LE(&oed->oed_HeadPtr, READMEM32_LE(&otd->otd_Self)|(unit->hu_DevDataToggle[devadrep] ? OEHF_DATA1 : 0));
                oed->oed_FirstTD = otd;
            }
            len = ioreq->iouh_Length - actual;
            if(len > OHCI_PAGE_SIZE)
            {
                len = OHCI_PAGE_SIZE;
            }
            otd->otd_Length = len;
            KPRINTF(1, ("TD with %ld bytes: %08x-%08x\n", len, phyaddr, phyaddr+len-1));
            CONSTWRITEMEM32_LE(&otd->otd_Ctrl, OTCF_CC_INVALID|OTCF_NOINT);
            if(len)
            {
                WRITEMEM32_LE(&otd->otd_BufferPtr, (IPTR)CachePreDMA((APTR)(IPTR)phyaddr, &len, (ioreq->iouh_Dir == UHDIR_IN) ? 0 : DMA_ReadFromRAM));
                phyaddr += len - 1;
                WRITEMEM32_LE(&otd->otd_BufferEnd, phyaddr);
                phyaddr++;
            } else {
                CONSTWRITEMEM32_LE(&otd->otd_BufferPtr, 0);
                CONSTWRITEMEM32_LE(&otd->otd_BufferEnd, 0);
            }
            actual += len;
            CacheClearE(&otd->otd_Ctrl, 16, CACRF_ClearD);

            predotd = otd;
        } while((actual < ioreq->iouh_Length) || (len && (ioreq->iouh_Dir == UHDIR_OUT) && (actual == ioreq->iouh_Length) && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) && ((actual % ioreq->iouh_MaxPktSize) == 0)));

        if(!actual)
        {
            // out of TDs
            KPRINTF(200, ("Out of TDs for Bulk Transfer!\n"));
            ohciFreeTDChain(hc, oed->oed_FirstTD);
            usbReleaseBuffer(oed->oed_Buffer, ioreq->iouh_Data, 0, 0);
            ohciFreeED(hc, oed);
            break;
        }
        oed->oed_Continue = (actual < ioreq->iouh_Length);
        predotd->otd_Succ = NULL;
        predotd->otd_NextTD = hc->hc_OhciTermTD->otd_Self;

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
        oed->oed_Succ = hc->hc_OhciBulkTailED;
        oed->oed_NextED = oed->oed_Succ->oed_Self;
        oed->oed_Pred = hc->hc_OhciBulkTailED->oed_Pred;
        oed->oed_Pred->oed_Succ = oed;
        oed->oed_Pred->oed_NextED = oed->oed_Self;
        oed->oed_Succ->oed_Pred = oed;
        CacheClearE(&oed->oed_EPCaps, 16, CACRF_ClearD);
        CacheClearE(&oed->oed_Pred->oed_EPCaps, 16, CACRF_ClearD);
        SYNC;

        KPRINTF(10, ("Activating BULK at %ld\n", READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));
        PrintED("Bulk", oed, hc);

	/* Similar to ohciScheduleCtrlTDs(), but use bulk queue */
        startmask = OCSF_BULKENABLE;
        Enable();
        ioreq = (struct IOUsbHWReq *) hc->hc_BulkXFerQueue.lh_Head;
    }

    if (startmask)
    {
        oldenables = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
        if(!(oldenables & OCSF_BULKENABLE))
        {
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

    ULONG restartmask = 0;

    KPRINTF(1, ("CompleteInt!\n"));

    ohciUpdateFrameCounter(hc);

    /* **************** PROCESS DONE TRANSFERS **************** */

    if (hc->hc_OhciDoneQueue)
        ohciHandleFinishedTDs(hc);

    if (hc->hc_Flags & HCF_ABORT)
    	restartmask = ohciHandleAbortedEDs(hc);

    if ((!(hc->hc_Flags & HCF_STOP_CTRL)) && hc->hc_CtrlXFerQueue.lh_Head->ln_Succ)
        restartmask |= ohciScheduleCtrlTDs(hc);

    if (hc->hc_IntXFerQueue.lh_Head->ln_Succ)
        ohciScheduleIntTDs(hc);

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
    if (restartmask)
    {
    	restartmask |= READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
	WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, restartmask);
	SYNC;
    }

    KPRINTF(1, ("CompleteDone\n"));

    return 0;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(ohciIntCode, struct PCIController *, hc)
{
    AROS_INTFUNC_INIT

    struct PCIDevice *base = hc->hc_Device;
    struct PCIUnit *unit = hc->hc_Unit;
    ULONG intr = 0;
    ULONG donehead;

    CacheClearE(&hc->hc_OhciHCCA->oha_DoneHead, sizeof(hc->hc_OhciHCCA->oha_DoneHead), CACRF_InvalidateD);

    donehead = READMEM32_LE(&hc->hc_OhciHCCA->oha_DoneHead);

    if(donehead)
    {
    	if (donehead & ~1)
    		intr = OISF_DONEHEAD;
    	if(donehead & 1)
    	{
    		intr |= READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS);
    	}
    	donehead &= OHCI_PTRMASK;

    	CONSTWRITEMEM32_LE(&hc->hc_OhciHCCA->oha_DoneHead, 0);

    	KPRINTF(5, ("New Donehead %08lx for old %08lx\n", donehead, hc->hc_OhciDoneQueue));
    } else {
    	intr = READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS);

    	if (intr & OISF_DONEHEAD)
    	{
    		KPRINTF(1, ("!!!!!!!!!!!!!!!!!!!!!!!DoneHead was empty!!!!!!!!!!!!!!!!!!!\n"));
    		CacheClearE(hc->hc_OhciHCCA, sizeof(struct OhciHCCA), CACRF_InvalidateD);
    		donehead = READMEM32_LE(&hc->hc_OhciHCCA->oha_DoneHead) & OHCI_PTRMASK;
    		CONSTWRITEMEM32_LE(&hc->hc_OhciHCCA->oha_DoneHead, 0);

    		KPRINTF(5, ("New Donehead %08lx for old %08lx\n", donehead, hc->hc_OhciDoneQueue));
    	}
    }
    CacheClearE(hc->hc_OhciHCCA, sizeof(struct OhciHCCA), CACRF_ClearD);

    intr &= ~OISF_MASTERENABLE;

    if(intr & hc->hc_PCIIntEnMask)
    {
        KPRINTF(1, ("ohciIntCode(0x%p) interrupts 0x%08lx, mask 0x%08lx\n", hc, intr, hc->hc_PCIIntEnMask));

	// Acknowledge all interrupts, but process only those we want
        WRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, intr);
        //KPRINTF(1, ("INT=%02lx\n", intr));
        intr &= hc->hc_PCIIntEnMask;

        if(intr & OISF_HOSTERROR)
        {
            KPRINTF(200, ("Host ERROR!\n"));
        }
        if(intr & OISF_SCHEDOVERRUN)
        {
            KPRINTF(200, ("Schedule overrun!\n"));
        }
        if (!(hc->hc_Flags & HCF_ONLINE))
        {
            if(READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS) & OISF_HUBCHANGE)
            {
                // if the driver is not online and the controller has a broken
                // hub change interrupt, make sure we don't run into infinite
                // interrupt by disabling the interrupt bit
                ohciDisableInt(hc, OISF_HUBCHANGE);
            }
            return FALSE;
        }
        WRITEREG32_LE(hc->hc_RegBase, OHCI_INTEN, OISF_HUBCHANGE);
        if(intr & OISF_FRAMECOUNTOVER)
        {
            hc->hc_FrameCounter |= 0x7fff;
            hc->hc_FrameCounter++;
            hc->hc_FrameCounter |= READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT) & 0xffff;
            KPRINTF(10, ("HCI 0x%p: Frame Counter Rollover %ld\n", hc, hc->hc_FrameCounter));
        }
        if(intr & OISF_HUBCHANGE)
        {
            UWORD hciport;
            ULONG oldval;
            UWORD portreg = OHCI_PORTSTATUS;
            BOOL clearbits = FALSE;

            if(READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS) & OISF_HUBCHANGE)
            {
                // some OHCI implementations will keep the interrupt bit stuck until
                // all port changes have been cleared, which is wrong according to the
                // OHCI spec. As a workaround we will clear all change bits, which should
                // be no problem as the port changes are reflected in the PortChangeMap
                // array.
                clearbits = TRUE;
            }
            for(hciport = 0; hciport < hc->hc_NumPorts; hciport++, portreg += 4)
            {
                oldval = READREG32_LE(hc->hc_RegBase, portreg);
                if(oldval & OHPF_OVERCURRENTCHG)
                {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_OVER_CURRENT;
                }
                if(oldval & OHPF_RESETCHANGE)
                {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_RESET;
                }
                if(oldval & OHPF_ENABLECHANGE)
                {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_ENABLE;
                }
                if(oldval & OHPF_CONNECTCHANGE)
                {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_CONNECTION;
                }
                if(oldval & OHPF_RESUMEDTX)
                {
                    hc->hc_PortChangeMap[hciport] |= UPSF_PORT_SUSPEND;
                }
                if(clearbits)
                {
                    WRITEREG32_LE(hc->hc_RegBase, portreg, OHPF_CONNECTCHANGE|OHPF_ENABLECHANGE|OHPF_RESUMEDTX|OHPF_OVERCURRENTCHG|OHPF_RESETCHANGE);
                }

                KPRINTF(20, ("PCI Int Port %ld (glob %ld) Change %08lx\n", hciport, hc->hc_PortNum20[hciport] + 1, oldval));
                if(hc->hc_PortChangeMap[hciport])
                {
                    unit->hu_RootPortChanges |= 1UL<<(hc->hc_PortNum20[hciport] + 1);
                }
            }
            uhwCheckRootHubChanges(unit);
            if(clearbits)
            {
                // again try to get rid of any bits that may be causing the interrupt
                WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBSTATUS, OHSF_OVERCURRENTCHG);
                WRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, OISF_HUBCHANGE);
            }
        }
        if(intr & OISF_DONEHEAD)
        {
        	KPRINTF(10, ("DoneHead Frame=%ld\n", READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));

        	if(hc->hc_OhciDoneQueue)
        	{
        		struct OhciTD *donetd = (struct OhciTD *) ((IPTR)donehead - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));

        		CacheClearE(&donetd->otd_Ctrl, 16, CACRF_InvalidateD);
        		while(donetd->otd_NextTD)
        		{
        			donetd = (struct OhciTD *) ((IPTR)donetd->otd_NextTD - hc->hc_PCIVirtualAdjust - offsetof(struct OhciTD, otd_Ctrl));
        			CacheClearE(&donetd->otd_Ctrl, 16, CACRF_InvalidateD);
        		}
        		WRITEMEM32_LE(&donetd->otd_NextTD, hc->hc_OhciDoneQueue);
        		CacheClearE(&donetd->otd_Ctrl, 16, CACRF_ClearD);
        
        		KPRINTF(10, ("Attached old DoneHead 0x%08lx to TD 0x%08lx\n", hc->hc_OhciDoneQueue, donetd->otd_Self));
        	}
        	hc->hc_OhciDoneQueue = donehead;
        }
        if (intr & OISF_SOF)
        {
            /* Aborted EDs are available for freeing */
            hc->hc_Flags |= HCF_ABORT;
        }

        if (intr & (OISF_SOF | OISF_DONEHEAD))
        {
            /*
             * These two are leveraged down to SoftInt.
             * This is done in order to keep queues rotation synchronized.
             */
       	    SureCause(base, &hc->hc_CompleteInt);
       	 }

        KPRINTF(1, ("Exiting ohciIntCode(0x%p)\n", unit));
    }

    /* Unlock interrupts  */
    WRITEREG32_LE(&hc->hc_RegBase, OHCI_INTEN, OISF_MASTERENABLE);

    return FALSE;

    AROS_INTFUNC_EXIT
}

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

    KPRINTF(70, ("HC 0x%p Aborting request 0x%p, command %ld, endpoint 0x%04lx, Frame=%ld\n", hc, ioreq, ioreq->iouh_Req.io_Command, devadrep, READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));
    PrintED("Aborting", oed, hc);

    /* Removing control and bulk EDs requires to stop the appropriate HC queue first (according to specification) */
    switch (ioreq->iouh_Req.io_Command)
    {
    case UHCMD_CONTROLXFER:
    	KPRINTF(50, ("Stopping control queue\n"));
    	hc->hc_Flags |= HCF_STOP_CTRL;
    	disablemask = OCSF_CTRLENABLE;
    	break;
 
    case UHCMD_BULKXFER:
	KPRINTF(50, ("Stopping bulk queue\n"));
    	hc->hc_Flags |= HCF_STOP_BULK;
        disablemask = OCSF_BULKENABLE;
    	break;
    }

    /* Stop selected queue(s) */
    if (disablemask)
    {
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

BOOL ohciInit(struct PCIController *hc, struct PCIUnit *hu) {

    struct PCIDevice *hd = hu->hu_Device;

    struct OhciED *oed;
    struct OhciED *predoed;
    struct OhciTD *otd;
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

    struct TagItem pciActivateMem[] =
    {
            { aHidd_PCIDevice_isMEM,    TRUE },
            { TAG_DONE, 0UL },
    };

    struct TagItem pciActivateBusmaster[] =
    {
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    struct TagItem pciDeactivateBusmaster[] =
    {
            { aHidd_PCIDevice_isMaster, FALSE },
            { TAG_DONE, 0UL },
    };

    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = "OHCI CompleteInt";
    hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (VOID_FUNC)ohciCompleteInt;

    hc->hc_PCIMemSize = OHCI_HCCA_SIZE + OHCI_HCCA_ALIGNMENT + 1;
    hc->hc_PCIMemSize += sizeof(struct OhciED) * OHCI_ED_POOLSIZE;
    hc->hc_PCIMemSize += sizeof(struct OhciTD) * OHCI_TD_POOLSIZE;

    memptr = HIDD_PCIDriver_AllocPCIMem(hc->hc_PCIDriverObject, hc->hc_PCIMemSize);
    hc->hc_PCIMem = (APTR) memptr;
    if (memptr)
    {
        // PhysicalAddress - VirtualAdjust = VirtualAddress
        // VirtualAddress  + VirtualAdjust = PhysicalAddress
        hc->hc_PCIVirtualAdjust = pciGetPhysical(hc, memptr) - (APTR)memptr;
        KPRINTF(10, ("VirtualAdjust 0x%08lx\n", hc->hc_PCIVirtualAdjust));

        // align memory
        memptr = (UBYTE *) (((IPTR)hc->hc_PCIMem + OHCI_HCCA_ALIGNMENT) & (~OHCI_HCCA_ALIGNMENT));
        hc->hc_OhciHCCA = (struct OhciHCCA *) memptr;
        KPRINTF(10, ("HCCA 0x%p\n", hc->hc_OhciHCCA));
        memptr += OHCI_HCCA_SIZE;

        // build up ED pool
        oed = (struct OhciED *) memptr;
        hc->hc_OhciEDPool = oed;
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
        hc->hc_OhciTDPool = otd;
        cnt = OHCI_TD_POOLSIZE - 1;
        do {
            otd->otd_Succ = (otd + 1);
            WRITEMEM32_LE(&otd->otd_Self, (IPTR)(&otd->otd_Ctrl) + hc->hc_PCIVirtualAdjust);
            otd++;
        } while(--cnt);
        otd->otd_Succ = NULL;
        WRITEMEM32_LE(&otd->otd_Self, (IPTR)(&otd->otd_Ctrl) + hc->hc_PCIVirtualAdjust);
        memptr += sizeof(struct OhciTD) * OHCI_TD_POOLSIZE;

        // terminating ED
        hc->hc_OhciTermED = oed = ohciAllocED(hc);
        oed->oed_Succ = NULL;
        oed->oed_Pred = NULL;
        CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
        oed->oed_NextED = 0;

        // terminating TD
        hc->hc_OhciTermTD = otd = ohciAllocTD(hc);
        otd->otd_Succ = NULL;
        otd->otd_NextTD = 0;

        // dummy head & tail Ctrl ED
        hc->hc_OhciCtrlHeadED = predoed = ohciAllocED(hc);
        hc->hc_OhciCtrlTailED = oed = ohciAllocED(hc);
        CONSTWRITEMEM32_LE(&predoed->oed_EPCaps, OECF_SKIP);
        CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
        predoed->oed_Succ = oed;
        predoed->oed_Pred = NULL;
        predoed->oed_NextED = oed->oed_Self;
        oed->oed_Succ = NULL;
        oed->oed_Pred = predoed;
        oed->oed_NextED = 0;

        // dummy head & tail Bulk ED
        hc->hc_OhciBulkHeadED = predoed = ohciAllocED(hc);
        hc->hc_OhciBulkTailED = oed = ohciAllocED(hc);
        CONSTWRITEMEM32_LE(&predoed->oed_EPCaps, OECF_SKIP);
        CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
        predoed->oed_Succ = oed;
        predoed->oed_Pred = NULL;
        predoed->oed_NextED = oed->oed_Self;
        oed->oed_Succ = NULL;
        oed->oed_Pred = predoed;
        oed->oed_NextED = 0;
        // 1 ms INT QH
        hc->hc_OhciIntED[0] = oed = ohciAllocED(hc);
        oed->oed_Succ = hc->hc_OhciTermED;
        oed->oed_Pred = NULL; // who knows...
        CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
        oed->oed_NextED = hc->hc_OhciTermED->oed_Self;
        predoed = oed;
        // make 5 levels of QH interrupts
        for(cnt = 1; cnt < 5; cnt++) {
            hc->hc_OhciIntED[cnt] = oed = ohciAllocED(hc);
            oed->oed_Succ = predoed;
            oed->oed_Pred = NULL; // who knows...
            CONSTWRITEMEM32_LE(&oed->oed_EPCaps, OECF_SKIP);
            oed->oed_NextED = hc->hc_OhciTermED->oed_Self;
            predoed = oed;
        }

        ohciUpdateIntTree(hc);

        // fill in framelist with IntED entry points based on interval
        tabptr = hc->hc_OhciHCCA->oha_IntEDs;
        for(cnt = 0; cnt < 32; cnt++) {
            oed = hc->hc_OhciIntED[4];
            bitcnt = 0;
            do {
                if(cnt & (1UL<<bitcnt)) {
                    oed = hc->hc_OhciIntED[bitcnt];
                    break;
                }
            } while(++bitcnt < 5);
            *tabptr++ = oed->oed_Self;
        }

        // time to initialize hardware...
        OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base0, (IPTR *) &hc->hc_RegBase);
        hc->hc_RegBase = (APTR) (((IPTR) hc->hc_RegBase) & (~0xf));
        KPRINTF(10, ("RegBase = 0x%p\n", hc->hc_RegBase));
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateMem); // enable memory

        hubdesca = READREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA);
        revision = READREG32_LE(hc->hc_RegBase, OHCI_REVISION);
        hc->hc_NumPorts = (hubdesca & OHAM_NUMPORTS)>>OHAS_NUMPORTS;
        KPRINTF(20, ("Found OHCI Controller %p FuncNum = %ld, Rev %02lx, with %ld ports\n",
             hc->hc_PCIDeviceObject, hc->hc_FunctionNum,
             revision & 0xFF,
             hc->hc_NumPorts));

        KPRINTF(20, ("Powerswitching: %s %s\n",
             hubdesca & OHAF_NOPOWERSWITCH ? "Always on" : "Available",
             hubdesca & OHAF_INDIVIDUALPS ? "per port" : "global"));

        control = READREG32_LE(hc->hc_RegBase, OHCI_CONTROL);
        KPRINTF(10, ("OHCI control state: 0x%08lx\n", control));

        // disable BIOS legacy support
        if (control & OCLF_SMIINT)
        {
            KPRINTF(10, ("BIOS still has hands on OHCI, trying to get rid of it\n"));

            cmdstatus = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
            cmdstatus |= OCSF_OWNERCHANGEREQ;
            WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, cmdstatus);
            timeout = 100;
            do {
                control = READREG32_LE(hc->hc_RegBase, OHCI_CONTROL);
                if(!(control & OCLF_SMIINT)) {
                    KPRINTF(10, ("BIOS gave up on OHCI. Pwned!\n"));
                    break;
                }
                uhwDelayMS(10, hu);
            } while(--timeout);
            if(!timeout) {
                KPRINTF(10, ("BIOS didn't release OHCI. Forcing and praying...\n"));
                control &= ~OCLF_SMIINT;
                WRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL, control);
            }
        }

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciDeactivateBusmaster); // no busmaster yet

        KPRINTF(10, ("Resetting OHCI HC\n"));
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, OCSF_HCRESET);
        cnt = 100;
        do {
            if(!(READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS) & OCSF_HCRESET)) {
                break;
            }
            uhwDelayMS(1, hu);
        } while(--cnt);

#ifdef DEBUG
        if(cnt == 0) {
            KPRINTF(20, ("Reset Timeout!\n"));
        } else {
            KPRINTF(20, ("Reset finished after %ld ticks\n", 100-cnt));
        }
#endif

        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateBusmaster); // enable busmaster

        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_PERIODICSTART, 10800); // 10% of 12000
        frameival = READREG32_LE(hc->hc_RegBase, OHCI_FRAMEINTERVAL);
        KPRINTF(10, ("FrameInterval=%08lx\n", frameival));
        frameival &= ~OIVM_BITSPERFRAME;
        frameival |= OHCI_DEF_BITSPERFRAME<<OIVS_BITSPERFRAME;
        frameival ^= OIVF_TOGGLE;
        WRITEREG32_LE(hc->hc_RegBase, OHCI_FRAMEINTERVAL, frameival);

        // make sure nothing is running
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_PERIODIC_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CTRL_HEAD_ED, AROS_LONG2LE(hc->hc_OhciCtrlHeadED->oed_Self));
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CTRL_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_HEAD_ED, AROS_LONG2LE(hc->hc_OhciBulkHeadED->oed_Self));
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_DONEHEAD, 0);

        WRITEREG32_LE(hc->hc_RegBase, OHCI_HCCA, (IPTR)pciGetPhysical(hc, hc->hc_OhciHCCA));

        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, OISF_ALL_INTS);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTDIS, OISF_ALL_INTS);
        SYNC;

        // install reset handler
        hc->hc_ResetInt.is_Code = (VOID_FUNC)OhciResetHandler;
        hc->hc_ResetInt.is_Data = hc;
        AddResetCallback(&hc->hc_ResetInt);

        // add interrupt
        hc->hc_PCIIntHandler.is_Node.ln_Name = "OHCI PCI (pciusb.device)";
        hc->hc_PCIIntHandler.is_Node.ln_Pri = 5;
        hc->hc_PCIIntHandler.is_Node.ln_Type = NT_INTERRUPT;
        hc->hc_PCIIntHandler.is_Code = (VOID_FUNC)ohciIntCode;
        hc->hc_PCIIntHandler.is_Data = hc;
        AddIntServer(INTB_KERNEL + hc->hc_PCIIntLine, &hc->hc_PCIIntHandler);

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
            hubdesca |= OHAF_NOPOWERSWITCH;	/* Required for some IntelMacs */
        else
            hubdesca |= OHAF_NOOVERCURRENT;
        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA, hubdesca);

        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBSTATUS, OHSF_POWERHUB);
        if((hubdesca & OHAF_NOPOWERSWITCH) || (!(hubdesca & OHAF_INDIVIDUALPS))) {
            KPRINTF(20, ("Individual power switching not available, turning on all ports!\n"));
            WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCB, 0);
        } else {
            KPRINTF(20, ("Enabling individual power switching\n"));
            WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCB, ((2<<hc->hc_NumPorts)-2)<<OHBS_PORTPOWERCTRL);
        }

        uhwDelayMS(50, hu);
        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA, hubdesca);

        CacheClearE(hc->hc_OhciHCCA,   sizeof(struct OhciHCCA),          CACRF_ClearD);
        CacheClearE(hc->hc_OhciEDPool, sizeof(struct OhciED) * OHCI_ED_POOLSIZE, CACRF_ClearD);
        CacheClearE(hc->hc_OhciTDPool, sizeof(struct OhciTD) * OHCI_TD_POOLSIZE, CACRF_ClearD);
            
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL, OCLF_PERIODICENABLE|OCLF_CTRLENABLE|OCLF_BULKENABLE|OCLF_ISOENABLE|OCLF_USBOPER);
        SYNC;

        KPRINTF(20, ("ohciInit returns TRUE...\n"));
        return TRUE;
    }

    /*
        FIXME: What would the appropriate debug level be?
    */
    KPRINTF(1000, ("ohciInit returns FALSE...\n"));
    return FALSE;
}

void ohciFree(struct PCIController *hc, struct PCIUnit *hu) {

    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        switch(hc->hc_HCIType)
        {
            case HCITYPE_OHCI:
            {
                KPRINTF(20, ("Shutting down OHCI %p\n", hc));
                CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTDIS, OISF_ALL_INTS);
                CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, OISF_ALL_INTS);

                // disable all ports
                WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCB, 0);
                WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBSTATUS, OHSF_UNPOWERHUB);

                uhwDelayMS(50, hu);
                KPRINTF(20, ("Stopping OHCI %p\n", hc));
                CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL, 0);
                CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, 0);
                SYNC;

                //KPRINTF(20, ("Reset done UHCI %08lx\n", hc));
                uhwDelayMS(10, hu);
                KPRINTF(20, ("Resetting OHCI %p\n", hc));
                CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, OCSF_HCRESET);
                SYNC;
                uhwDelayMS(50, hu);

                KPRINTF(20, ("Shutting down OHCI done.\n"));
                break;
            }
        }

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }

}
