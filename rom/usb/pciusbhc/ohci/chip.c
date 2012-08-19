/*
   Copyright © 2002-2009, Chris Hodges. All rights reserved.
   Copyright © 2009-2012, The AROS Development Team. All rights reserved.
   $Id$
 */

#include <devices/usb_hub.h>
#include <hidd/pci.h>

#include <proto/exec.h>
#include <clib/alib_protos.h>
#include <proto/oop.h>

#include "debug.h"
#include "chip.h"
#include "pci.h"

#include "chip_protos.h"
#include "roothub_protos.h"
#include "buffer_protos.h"
#include "cmd_protos.h"
#include "pci_protos.h"

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase (hd->hd_HiddPCIDeviceAB)
#undef HiddAttrBase
#define HiddAttrBase (hd->hd_HiddAB)

ULONG start_masks[] = {OCSF_CTRLENABLE, OCSF_BULKENABLE, 0UL, 0UL};
ULONG current_ed_regs[] = {OHCI_CTRL_ED, OHCI_BULK_ED, 0UL, 0UL};

static ULONG ScheduleED(struct PCIController *hc, UWORD xfer_type,
    struct IOUsbHWReq *ioreq);
static ULONG FillED(struct PCIController *hc, struct EDNode *ed,
    UWORD xfer_type, struct IOUsbHWReq *ioreq, UWORD dir);

/* /// "AddHeadPhy()" */
static void AddHeadED(ULONG * list, struct EDNode *ed)
{
    ed->ed_ED.NextED = *list;
    *list = ed->ed_Self;
    CacheClearE(&ed->ed_ED.EPCaps, 16, CACRF_ClearD);
    CacheClearE(list, 4, CACRF_ClearD);
}
/* \\\ */

/* /// "AllocED()" */
static struct EDNode *AllocED(struct PCIController *hc)
{
    struct EDNode *ed =
        (struct EDNode *)RemHead((struct List *)&hc->hc_FreeEDList);

    if (ed != NULL)
    {
        ed->ed_ED.HeadPtr = 0UL;
        ed->ed_ED.TailPtr = hc->hc_TermTD->td_Self;
    }
    if (ed == NULL)
        KPRINTF(20, ("Out of EDs!\n"));

    return ed;
}
/* \\\ */

/* /// "FreeED()" */
static void FreeED(struct PCIController *hc, struct EDNode *ed)
{
    CONSTWRITEMEM32_LE(&ed->ed_ED.EPCaps, OECF_SKIP);
    SYNC;

    ed->ed_IOReq = NULL;
    ed->ed_Buffer = NULL;
    ed->ed_SetupData = NULL;
    AddTail((struct List *)&hc->hc_FreeEDList, (struct Node *)ed);
    ed->ed_ED.HeadPtr = ed->ed_ED.TailPtr = 0UL;
}
/* \\\ */

/* /// "AllocTD()" */
static struct TDNode *AllocTD(struct PCIController *hc)
{
    struct TDNode *td =
        (struct TDNode *)RemHead((struct List *)&hc->hc_FreeTDList);

    if (td == NULL)
        KPRINTF(20, ("Out of TDs!\n"));

    return td;
}
/* \\\ */

/* /// "FreeTD()" */
static void FreeTD(struct PCIController *hc, struct TDNode *td)
{
    td->td_TD.NextTD = 0UL;
    SYNC;

    td->td_ED = NULL;
    AddTail((struct List *)&hc->hc_FreeTDList, (struct Node *)td);
}
/* \\\ */

/* /// "DisableED()" */
/* note: does not work on EDs in the interrupt tree */
static void DisableED(struct EDNode *ed)
{
    ULONG ctrlstatus, succ_ed_phy, dma_size;
    struct EDNode *pred_ed, *succ_ed;

    // disable ED
    ctrlstatus = READMEM32_LE(&ed->ed_ED.EPCaps);
    ctrlstatus |= OECF_SKIP;
    WRITEMEM32_LE(&ed->ed_ED.EPCaps, ctrlstatus);

    // unlink from schedule
    succ_ed = (struct EDNode *)ed->ed_Node.mln_Succ;
    pred_ed = (struct EDNode *)ed->ed_Node.mln_Pred;
    if (succ_ed->ed_Node.mln_Succ != NULL)
        succ_ed_phy = succ_ed->ed_Self;
    else
        succ_ed_phy = 0L;
    if (pred_ed->ed_Node.mln_Pred != NULL)
        pred_ed->ed_ED.NextED = succ_ed_phy;

    Remove((struct Node *)ed);
    ed->ed_IOReq = NULL;
    dma_size = sizeof(struct EndpointDescriptor);
    CachePreDMA(&ed->ed_ED, &dma_size, 0);
    SYNC;
}
/* \\\ */

/* /// "DisableInt()" */
static void DisableInt(struct PCIController *hc, ULONG mask)
{
    WRITEREG32_LE(hc->hc_RegBase, OHCI_INTDIS, mask);
    hc->hc_PCIIntEnMask &= ~mask;
}
/* \\\ */

/* /// "EnableInt()" */
static void EnableInt(struct PCIController *hc, ULONG mask)
{
    WRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, mask);
    hc->hc_PCIIntEnMask |= mask;
    WRITEREG32_LE(hc->hc_RegBase, OHCI_INTEN, mask);
}
/* \\\ */

#ifdef DEBUG_TD

/* /// "PrintTD()" */
static void PrintTD(const char *txt, ULONG ptd, struct PCIController *hc)
{
    KPrintF("HC 0x%p %s TD list:", hc, txt);

    while (ptd)
    {
        struct TDNode *td =
            (struct TDNode *)((IPTR) ptd - hc->hc_PCIVirtualAdjust -
            offsetof(struct TDNode, td_TD.Ctrl));

        KPrintF(" 0x%p", td);
        ptd = READMEM32_LE(&td->td_TD.NextTD);
    }
    RawPutChar('\n');
}
/* \\\ */

#else
#define PrintTD(txt, ptd, hc)
#endif

#ifdef DEBUG_ED

/* /// "PrintED()" */
static void PrintED(const char *txt, struct EDNode *ed,
    struct PCIController *hc)
{
    struct TDNode *td;

    KPrintF
        ("%s ED 0x%p: EPCaps=%08lx, HeadPtr=%08lx, TailPtr=%08lx,"
        " NextED=%08lx\n",
        txt, ed, READMEM32_LE(&ed->ed_ED.EPCaps),
        READMEM32_LE(&ed->ed_ED.HeadPtr), READMEM32_LE(&ed->ed_ED.TailPtr),
        READMEM32_LE(&ed->ed_ED.NextED));

    KPrintF("...TD list:\n");
    for (td = (struct TDNode *)ed->ed_TDList.mlh_Head; td->td_Node.mln_Succ;
        td = (struct TDNode *)td->td_Node.mln_Succ)
        KPrintF
            ("TD 0x%p: td_TD.Ctrl=%lx BufferPtr=%lx NextTD=%lx"
            " BufferEnd=%lx\n",
            td, td->td_TD.Ctrl, td->td_TD.BufferPtr, td->td_TD.NextTD,
            td->td_TD.BufferEnd);
}
/* \\\ */

#else
#define PrintED(txt, ed, hc)
#endif

/* /// "ResetHandler()" */
static AROS_UFIH1(ResetHandler, struct PCIController *, hc)
{
    AROS_USERFUNC_INIT
        // reset controller
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, OCSF_HCRESET);

    return FALSE;

AROS_USERFUNC_EXIT}
/* \\\ */

/* /// "AddTailTD()" */
static void AddTailTD(struct EDNode *ed, struct TDNode *td)
{
    struct TDNode *old_tail_td = NULL;
    ULONG dma_size, td_phy;

    if ((ed->ed_ED.HeadPtr & OHCI_PTRMASK) != 0UL)
        old_tail_td = (struct TDNode *)ed->ed_TDList.mlh_TailPred;

    td->td_TD.NextTD = ed->ed_ED.TailPtr;
    td->td_ED = ed;

    dma_size = sizeof(struct TransferDescriptor);
    td_phy = (ULONG) (IPTR) CachePreDMA(&td->td_TD, &dma_size, 0);

    if (old_tail_td != NULL)
    {
        old_tail_td->td_TD.NextTD = td_phy;
        dma_size = sizeof(struct TransferDescriptor);
        CachePreDMA(&old_tail_td->td_TD, &dma_size, 0);
    }
    else
    {
        ed->ed_ED.HeadPtr |= td->td_Self;
        dma_size = sizeof(struct EndpointDescriptor);
        CachePreDMA(&ed->ed_ED, &dma_size, 0);
    }
}
/* \\\ */

/* /// "FreeTDChain()" */
static void FreeTDChain(struct PCIController *hc, struct MinList *tdlist)
{
    struct TDNode *td;

    while ((td = (struct TDNode *)RemHead((struct List *)tdlist)) != NULL)
    {
        KPRINTF(1, ("FreeTD %p\n", td));
        FreeTD(hc, td);
    }
}
/* \\\ */

/* /// "FreeEDContext()" */
static void FreeEDContext(struct PCIController *hc, struct EDNode *ed,
    struct IOUsbHWReq *ioreq)
{
    UWORD dir;

    KPRINTF(5, ("Freeing EDContext 0x%p IOReq 0x%p\n", ed, ioreq));

    if (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
        dir =
            (ioreq->
            iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN : UHDIR_OUT;
    else
        dir = ioreq->iouh_Dir;

    usbReleaseBuffer(ed->ed_Buffer, ioreq->iouh_Data, ioreq->iouh_Actual,
        dir);
    usbReleaseBuffer(ed->ed_SetupData, &ioreq->iouh_SetupData, 8, UHDIR_IN);

    Disable();
    FreeTDChain(hc, &ed->ed_TDList);
    FreeED(hc, ed);
    Enable();
}
/* \\\ */

/* /// "UpdateIntTree()" */
static void UpdateIntTree(struct PCIController *hc)
{
    struct EDNode *ed;
    UWORD i, j, k, l;
    ULONG *queue_heads = hc->hc_HCCA->ha_IntEDs;

    // initialise every queue head to point at the terminal ED by default
    for (i = 0; i < 32; i++)
    {
        queue_heads[i] = hc->hc_TermED->ed_Self;
    }

    // put each ED in the right number of queues for its interval level.
    // we balance the tree by incrementing the slot we start at for each ED
    for (i = 0; i < INT_LIST_COUNT; i++)
    {
        ed = (struct EDNode *)hc->hc_EDLists[INT_XFER + i].mlh_Head;
        for (j = 0; ed->ed_Node.mln_Succ != NULL; j++)
        {
            for (k = 0, l = j; k < 1 << (INT_LIST_COUNT - i - 1); k++)
            {
                AddHeadED(&queue_heads[l % 32], ed);
                l += 1 << i;
            }
            ed = (struct EDNode *)ed->ed_Node.mln_Succ;
        }
    }
}
/* \\\ */

/* /// "HandleFinishedTDs()" */
static void HandleFinishedTDs(struct PCIController *hc)
{
    struct IOUsbHWReq *ioreq;
    struct IOUsbHWReq *nextioreq;
    struct EDNode *ed = NULL;
    struct TDNode *td, *nexttd;
    ULONG len;
    ULONG ctrlstatus;
    ULONG epcaps;
    UWORD target;
    BOOL direction_in;
    BOOL updatetree = FALSE;
    ULONG donehead, ptr;
    BOOL retire;
    ULONG oldenables;
    struct PCIUnit *unit = hc->hc_Unit;
    ULONG dma_size;

    KPRINTF(100, ("Checking for work done...\n"));
    Disable();
    donehead = hc->hc_DoneQueue;
    hc->hc_DoneQueue = 0UL;
    Enable();
    if (!donehead)
    {
        KPRINTF(1, ("Nothing to do!\n"));
        return;
    }
    td = (struct TDNode *)((IPTR) donehead - hc->hc_PCIVirtualAdjust -
        offsetof(struct TDNode, td_TD.Ctrl));
    KPRINTF(100, ("DoneHead=%08lx, TD=%p, Frame=%ld\n", donehead, td,
            READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));
    PrintTD("Done", donehead, hc);

    do
    {
        dma_size = sizeof(struct TransferDescriptor);
        CachePostDMA(&td->td_TD, &dma_size, 0);
        ed = td->td_ED;
        if (!ed)
        {
            KPRINTF(1000,
                ("Came across a rogue TD 0x%p that already has been freed!\n",
                    td));
            ptr = READMEM32_LE(&td->td_TD.NextTD) & OHCI_PTRMASK;
            if (!ptr)
            {
                break;
            }
            td = (struct TDNode *)((IPTR) ptr - hc->hc_PCIVirtualAdjust -
                offsetof(struct TDNode, td_TD));
            continue;
        }
        dma_size = sizeof(struct EndpointDescriptor);
        CachePostDMA(&ed->ed_ED, &dma_size, 0);

        ctrlstatus = READMEM32_LE(&td->td_TD.Ctrl);
        KPRINTF(100, ("TD: %08lx - %08lx\n",
                READMEM32_LE(&td->td_TD.BufferPtr),
                READMEM32_LE(&td->td_TD.BufferEnd)));
        if (td->td_TD.BufferPtr)
        {
            // FIXME: this will blow up if physical memory is ever going to
            // be discontinuous
            len =
                READMEM32_LE(&td->td_TD.BufferPtr) -
                (READMEM32_LE(&td->td_TD.BufferEnd) + 1 - td->td_Length);
        }
        else
        {
            len = td->td_Length;
        }

        ioreq = ed->ed_IOReq;

        KPRINTF(100,
            ("Examining TD %p for ED %p (IOReq=%p), Status %08lx, len=%ld\n",
                td, ed, ioreq, ctrlstatus, len));
        if (!ioreq)
        {
            /* You should never see this (very weird inconsistency), but who
             * knows... */
            KPRINTF(1000,
                ("Came across a rogue ED 0x%p that already has been replied! "
                "TD 0x%p,\n",
                    ed, td));
            ptr = READMEM32_LE(&td->td_TD.NextTD) & OHCI_PTRMASK;
            if (!ptr)
            {
                break;
            }
            td = (struct TDNode *)((IPTR) ptr - hc->hc_PCIVirtualAdjust -
                offsetof(struct TDNode, td_TD.Ctrl));
            continue;
        }

        if (len)
        {
            epcaps = READMEM32_LE(&ed->ed_ED.EPCaps);
            direction_in = ((epcaps & OECM_DIRECTION) == OECF_DIRECTION_TD)
                ? (ioreq->iouh_SetupData.bmRequestType & URTF_IN)
                : (epcaps & OECF_DIRECTION_IN);
            // FIXME: CachePostDMA() should be passed a virtual pointer
            CachePostDMA((APTR) (IPTR) READMEM32_LE(&td->td_TD.BufferEnd) -
                len + 1, &len, direction_in ? 0 : DMA_ReadFromRAM);
        }

        ioreq->iouh_Actual += len;
        retire = TRUE;
        switch ((ctrlstatus & OTCM_COMPLETIONCODE))
        {
        case OTCF_CC_CRCERROR:
        case OTCF_CC_BABBLE:
        case OTCF_CC_PIDCORRUPT:
        case OTCF_CC_WRONGPID:
            ioreq->iouh_Req.io_Error = UHIOERR_CRCERROR;
            break;
        case OTCF_CC_STALL:
            ioreq->iouh_Req.io_Error = UHIOERR_STALL;
            break;
        case OTCF_CC_TIMEOUT:
            ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
            break;
        case OTCF_CC_OVERFLOW:
            ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
            break;
        case OTCF_CC_SHORTPKT:
            if ((!ioreq->iouh_Req.io_Error)
                && (!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS)))
            {
                ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
            }
            break;
        case OTCF_CC_OVERRUN:
        case OTCF_CC_UNDERRUN:
            ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
            break;
        case OTCF_CC_NOERROR:
        case OTCF_CC_WRONGTOGGLE:
        case OTCF_CC_INVALID:
        default:
            retire = FALSE;
            break;
        }
        if (retire)
            KPRINTF(200, ("Bad completion code: %d\n",
                    (ctrlstatus & OTCM_COMPLETIONCODE) >>
                    OTCS_COMPLETIONCODE));
        if ((ctrlstatus & OTCM_DELAYINT) != OTCF_NOINT)
        {
            KPRINTF(10, ("TD 0x%p Terminator detected\n", td));
            retire = TRUE;
        }
        if (READMEM32_LE(&ed->ed_ED.HeadPtr) & OEHF_HALTED)
        {
            KPRINTF(100, ("ED halted!\n"));
            retire = TRUE;
        }

        if (retire)
        {
            KPRINTF(50, ("ED 0x%p stopped at TD 0x%p\n", ed, td));
            Remove(&ioreq->iouh_Req.io_Message.mn_Node);
            AddHead(&hc->hc_RetireQueue,
                &ioreq->iouh_Req.io_Message.mn_Node);
        }

        ptr = READMEM32_LE(&td->td_TD.NextTD) & OHCI_PTRMASK;
        KPRINTF(1, ("NextTD=0x%08lx\n", ptr));
        if (!ptr)
        {
            break;
        }
        td = (struct TDNode *)((IPTR) ptr - hc->hc_PCIVirtualAdjust -
            offsetof(struct TDNode, td_TD.Ctrl));
        KPRINTF(1, ("NextTD = %p\n", td));
    }
    while (TRUE);

    ioreq = (struct IOUsbHWReq *)hc->hc_RetireQueue.lh_Head;
    while ((nextioreq =
            (struct IOUsbHWReq *)((struct Node *)ioreq)->ln_Succ))
    {
        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ed = (struct EDNode *)ioreq->iouh_DriverPrivate1;
        if (ed)
        {
            KPRINTF(50,
                ("HC 0x%p Retiring IOReq=0x%p Command=%ld ED=0x%p, Frame=%ld\n",
                    hc, ioreq, ioreq->iouh_Req.io_Command, ed,
                    READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));

            if (ed->ed_Continue)
            {
                // reinitialise physical links in ED and its TD list
                td = (struct TDNode *)ed->ed_TDList.mlh_Head;
                ed->ed_ED.HeadPtr = td->td_Self;
                while (td->td_Node.mln_Succ != NULL)
                {
                    nexttd = (struct TDNode *)td->td_Node.mln_Succ;
                    if (nexttd != (struct TDNode *)&ed->ed_TDList.mlh_Tail)
                        td->td_TD.NextTD = nexttd->td_Self;
                    else
                        td->td_TD.NextTD = hc->hc_TermTD->td_Self;
                    td = nexttd;
                }

                // Refill ED with next data block
                FillED(hc, ed, BULK_XFER, ioreq, ioreq->iouh_Dir);
                PrintED("Continued bulk", ed, hc);

                Disable();
                AddTail(&hc->hc_TDQueue, (struct Node *)ioreq);
                oldenables = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
                oldenables |= OCSF_BULKENABLE;
                WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, oldenables);
                SYNC;
                Enable();
            }
            else
            {
                // disable ED
                if (ioreq->iouh_Req.io_Command == UHCMD_INTXFER)
                {
                    updatetree = TRUE;
                    Remove((struct Node *)ed);
                }
                else
                    DisableED(ed);
                PrintED("Completed", ed, hc);

                target =
                    (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint +
                    ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                unit->hu_DevBusyReq[target] = NULL;
                unit->hu_DevDataToggle[target] =
                    (READMEM32_LE(&ed->
                        ed_ED.HeadPtr) & OEHF_DATA1) ? TRUE : FALSE;
                FreeEDContext(hc, ed, ioreq);

                // check for successful clear feature and set address ctrl
                // transfers
                if ((!ioreq->iouh_Req.io_Error)
                    && (ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER))
                {
                    CheckSpecialCtrlTransfers(hc, ioreq);
                }
                ReplyMsg(&ioreq->iouh_Req.io_Message);
            }
        }
        else
        {
            KPRINTF(20, ("IOReq=%p has no ED!\n", ioreq));
        }
        ioreq = nextioreq;
    }
    if (updatetree)
    {
        UpdateIntTree(hc);
    }
}
/* \\\ */

/* /// "HandleAbortedEDs()" */
static ULONG HandleAbortedEDs(struct PCIController *hc)
{
    struct IOUsbHWReq *ioreq;
    ULONG restartmask = 0;
    UWORD target;
    struct EDNode *ed;
    struct PCIUnit *unit = hc->hc_Unit;

    KPRINTF(50, ("Processing abort queue...\n"));

    // We don't need this any more
    DisableInt(hc, OISF_SOF);

    /*
     * If the aborted IORequest was replied in HandleFinishedTDs(),
     * it was already Remove()d from this queue. It's safe to do no checks.
     * io_Error was set earlier.
     */
    while ((ioreq = (struct IOUsbHWReq *)RemHead(&hc->hc_AbortQueue)))
    {
        KPRINTF(70, ("HC 0x%p Aborted IOReq 0x%p\n", hc, ioreq));
        PrintED("Aborted", ioreq->iouh_DriverPrivate1, hc);

        ed = ioreq->iouh_DriverPrivate1;
        target =
            (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint +
            ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        unit->hu_DevBusyReq[target] = NULL;
        unit->hu_DevDataToggle[target] =
            (READMEM32_LE(&ed->ed_ED.HeadPtr) & OEHF_DATA1) ? TRUE : FALSE;
        FreeEDContext(hc, ed, ioreq);
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

    /* We will accumulate flags and start queues only once, when everything
     * is set up */
    return restartmask;
}
/* \\\ */

/* /// "FillED()" */
static ULONG FillED(struct PCIController *hc, struct EDNode *ed,
    UWORD xfer_type, struct IOUsbHWReq *ioreq, UWORD dir)
{
    BOOL success = TRUE, is_new_td;
    struct TDNode *td;
    ULONG actual;
    ULONG ctrl;
    ULONG len;
    ULONG phyaddr;
    ULONG dma_size;

    if (xfer_type == CTRL_XFER)
    {
        // construct set-up TD
        td = AllocTD(hc);
        if (td != NULL)
        {
            // fill setup td
            td->td_Length = 0;  // don't increase io_Actual for that transfer
            CONSTWRITEMEM32_LE(&td->td_TD.Ctrl,
                OTCF_CC_INVALID | OTCF_TOGGLEFROMTD | OTCF_NOINT |
                OTCF_PIDCODE_SETUP | OTCF_ALLOWSHORTPKT);
            len = 8;

            ed->ed_SetupData =
                usbGetBuffer(&ioreq->iouh_SetupData, len, UHDIR_OUT);
            phyaddr =
                (ULONG) CachePreDMA(ed->ed_SetupData, &len,
                DMA_ReadFromRAM);
            WRITEMEM32_LE(&td->td_TD.BufferPtr, phyaddr);
            WRITEMEM32_LE(&td->td_TD.BufferEnd, phyaddr + len - 1);

            KPRINTF(1, ("TD send: %08lx - %08lx\n",
                    READMEM32_LE(&td->td_TD.BufferPtr),
                    READMEM32_LE(&td->td_TD.BufferEnd)));

            AddTailTD(ed, td);
            AddTail((struct List *)&ed->ed_TDList, (struct Node *)td);
        }
        else
            success = FALSE;
    }

    if (success)
    {
        // put data into a series of TDs
        actual = ioreq->iouh_Actual;
        ctrl =
            OTCF_CC_INVALID | OTCF_NOINT | (dir ==
            UHDIR_IN ? OTCF_PIDCODE_IN : OTCF_PIDCODE_OUT);
        if (xfer_type == CTRL_XFER)
            ctrl |= OTCF_TOGGLEFROMTD | OTCF_DATA1;
        if (xfer_type == CTRL_XFER
            || !(ioreq->iouh_Flags & UHFF_NOSHORTPKT))
            ctrl |= OTCF_ALLOWSHORTPKT;

        ed->ed_Buffer =
            usbGetBuffer(ioreq->iouh_Data, ioreq->iouh_Length, dir);
        if (ed->ed_Buffer == NULL && ioreq->iouh_Data != NULL)
            success = FALSE;
        if (xfer_type == BULK_XFER)
            td = (struct TDNode *)ed->ed_TDList.mlh_Head;
        else
            td = (struct TDNode *)&ed->ed_TDList.mlh_Tail;

        while (success && actual < ioreq->iouh_Length
            && (actual - ioreq->iouh_Actual < OHCI_TD_BULK_LIMIT
                || xfer_type != BULK_XFER))
        {
            // reuse the next old TD or get a new one
            if (td == (struct TDNode *)&ed->ed_TDList.mlh_Tail)
            {
                td = AllocTD(hc);
                if (td == NULL)
                    success = FALSE;
                is_new_td = TRUE;
            }
            else
                is_new_td = FALSE;

            if (success)
            {
                len = ioreq->iouh_Length - actual;
                if (len > OHCI_PAGE_SIZE)
                {
                    len = OHCI_PAGE_SIZE;
                }
                td->td_Length = len;
                KPRINTF(1, ("TD with %ld bytes. Status=%lx\n", len, ctrl));
                WRITEMEM32_LE(&td->td_TD.Ctrl, ctrl);
                phyaddr =
                    (ULONG) (IPTR) CachePreDMA(ed->ed_Buffer + actual, &len,
                    dir == UHDIR_IN ? 0 : DMA_ReadFromRAM);
                WRITEMEM32_LE(&td->td_TD.BufferPtr, phyaddr);
                phyaddr += len - 1;
                WRITEMEM32_LE(&td->td_TD.BufferEnd, phyaddr);

                KPRINTF(1, ("TD send: %08lx - %08lx\n",
                        READMEM32_LE(&td->td_TD.BufferPtr),
                        READMEM32_LE(&td->td_TD.BufferEnd)));

                actual += len;

                if (is_new_td)
                {
                    AddTailTD(ed, td);
                    AddTail((struct List *)&ed->ed_TDList,
                        (struct Node *)td);
                }
                else
                {
                    dma_size = sizeof(struct TransferDescriptor);
                    CachePreDMA(&td->td_TD, &dma_size, 0);
                }
            }
            td = (struct TDNode *)td->td_Node.mln_Succ;
        }
    }

    // construct control-status TD or empty-bulk TD
    if (success)
    {
        if (xfer_type == CTRL_XFER || xfer_type == BULK_XFER
            && dir == UHDIR_OUT && actual == ioreq->iouh_Length
            && (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT))
            && actual % ioreq->iouh_MaxPktSize == 0)
        {
            if (td == (struct TDNode *)&ed->ed_TDList.mlh_Tail)
            {
                td = AllocTD(hc);
                if (td == NULL)
                    success = FALSE;
                is_new_td = TRUE;
            }
            else
                is_new_td = FALSE;

            if (success)
            {
                if (xfer_type == CTRL_XFER)
                {
                    ctrl ^=
                        OTCF_NOINT | OTCF_PIDCODE_IN | OTCF_PIDCODE_OUT |
                        OTCF_ALLOWSHORTPKT;
                    ctrl |= OTCF_TOGGLEFROMTD | OTCF_DATA1;
                }
                else
                    ctrl ^= OTCF_NOINT;

                td->td_Length = 0;
                CONSTWRITEMEM32_LE(&td->td_TD.Ctrl, ctrl);
                CONSTWRITEMEM32_LE(&td->td_TD.BufferPtr, 0);
                CONSTWRITEMEM32_LE(&td->td_TD.BufferEnd, 0);

                if (is_new_td)
                {
                    AddTailTD(ed, td);
                    AddTail((struct List *)&ed->ed_TDList,
                        (struct Node *)td);
                }
                else
                {
                    td->td_TD.NextTD = hc->hc_TermTD->td_Self;
                    dma_size = sizeof(struct TransferDescriptor);
                    CachePreDMA(&td->td_TD, &dma_size, 0);
                }
            }
        }
        else
        {
            if (xfer_type == BULK_XFER)
                ed->ed_Continue = (actual < ioreq->iouh_Length);
            td = (struct TDNode *)td->td_Node.mln_Pred;
            td->td_TD.NextTD = hc->hc_TermTD->td_Self;
            CONSTWRITEMEM32_LE(&td->td_TD.Ctrl, OTCF_CC_INVALID);
            dma_size = sizeof(struct TransferDescriptor);
            CachePreDMA(&td->td_TD, &dma_size, 0);
        }
    }

    if (!success)
    {
        FreeEDContext(hc, ed, ioreq);
    }

    return success;
}
/* \\\ */

/* /// "ScheduleED()" */
static ULONG ScheduleED(struct PCIController *hc, UWORD xfer_type,
    struct IOUsbHWReq *ioreq)
{
    BOOL success = TRUE;
    struct PCIUnit *unit = hc->hc_Unit;
    UWORD target;
    UWORD dir, list_no, list_index, interval;
    struct EDNode *ed;
    struct EDNode *pred_ed;
    ULONG epcaps, dma_size, phy_addr;

    ed = AllocED(hc);
    if (ed == NULL)
        success = FALSE;

    if (success)
    {
        ed->ed_IOReq = ioreq;

        if (xfer_type == CTRL_XFER)
            dir =
                (ioreq->
                iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN :
                UHDIR_OUT;
        else
            dir = ioreq->iouh_Dir;

        target = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint;
        if (xfer_type != CTRL_XFER && dir == UHDIR_IN)
            target |= 0x10;

        epcaps =
            (ioreq->
            iouh_DevAddr << OECS_DEVADDR) | (ioreq->iouh_Endpoint <<
            OECS_ENDPOINT) | (ioreq->iouh_MaxPktSize << OECS_MAXPKTLEN);
        if (xfer_type == CTRL_XFER)
            epcaps |= OECF_DIRECTION_TD;
        else
            epcaps |=
                dir == UHDIR_IN ? OECF_DIRECTION_IN : OECF_DIRECTION_OUT;

        if (ioreq->iouh_Flags & UHFF_LOWSPEED)
        {
            KPRINTF(5, ("*** LOW SPEED ***\n"));
            epcaps |= OECF_LOWSPEED;
        }

        WRITEMEM32_LE(&ed->ed_ED.EPCaps, epcaps);

        if (xfer_type != CTRL_XFER && unit->hu_DevDataToggle[target])
            WRITEMEM32_LE(&ed->ed_ED.HeadPtr, OEHF_DATA1);

        if (!FillED(hc, ed, xfer_type, ioreq, dir))
            success = FALSE;
    }

    if (success)
    {
        Remove(&ioreq->iouh_Req.io_Message.mn_Node);
        ioreq->iouh_DriverPrivate1 = ed;

        // choose logical list to add ED to
        list_index = 0;
        if (xfer_type == INT_XFER)
        {
            interval = ioreq->iouh_Interval;
            if (interval < 32)
            {
                while (interval > 1)
                {
                    interval >>= 1;
                    list_index++;
                }
            }
            else
                list_index = INT_LIST_COUNT - 1;
        }
        list_no = xfer_type + list_index;

        // manage endpoint going busy
        Disable();
        unit->hu_DevBusyReq[target] = ioreq;
        unit->hu_NakTimeoutFrame[target] =
            (ioreq->iouh_Flags & UHFF_NAKTIMEOUT) ? hc->hc_FrameCounter +
            ioreq->iouh_NakTimeout : 0;

        AddTail(&hc->hc_TDQueue, (struct Node *)ioreq);

        // looks good to me, now enqueue this entry
        AddTail((struct List *)&hc->hc_EDLists[list_no], (struct Node *)ed);

        if (xfer_type == INT_XFER)
        {
            UpdateIntTree(hc);
        }
        else
        {
            ed->ed_ED.NextED = 0L;
            dma_size = sizeof(struct EndpointDescriptor);
            phy_addr = (ULONG) (IPTR) CachePreDMA(&ed->ed_ED, &dma_size, 0);

            pred_ed = (struct EDNode *)ed->ed_Node.mln_Pred;
            if (pred_ed->ed_Node.mln_Pred != NULL)
            {
                pred_ed->ed_ED.NextED = phy_addr;
                dma_size = sizeof(struct EndpointDescriptor);
                CachePreDMA(&pred_ed->ed_ED, &dma_size, 0);
            }
            else
                WRITEREG32_LE(hc->hc_RegBase, (xfer_type == CTRL_XFER) ?
                    OHCI_CTRL_HEAD_ED : OHCI_BULK_HEAD_ED, ed->ed_Self);
        }

        SYNC;

        PrintED(xfer_names[xfer_type], ed, hc);

        Enable();
    }

    if (!success)
    {
        FreeEDContext(hc, ed, ioreq);
    }

    return success;
}
/* \\\ */

/* /// "ScheduleXfers()" */
static ULONG ScheduleXfers(struct PCIController *hc, UWORD xfer_type)
{
    BOOL success = TRUE;
    struct PCIUnit *unit = hc->hc_Unit;
    struct IOUsbHWReq *ioreq;
    UWORD target;
    UWORD dir;
    ULONG oldenables;
    ULONG startmask = 0;

    KPRINTF(1, ("Scheduling new %s transfers...\n", xfer_names[xfer_type]));
    ioreq = (struct IOUsbHWReq *)hc->hc_XferQueues[xfer_type].lh_Head;
    while (success && ((struct Node *)ioreq)->ln_Succ)
    {
        if (xfer_type == CTRL_XFER)
            dir =
                (ioreq->
                iouh_SetupData.bmRequestType & URTF_IN) ? UHDIR_IN :
                UHDIR_OUT;
        else
            dir = ioreq->iouh_Dir;

        target = (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint;
        if (xfer_type != CTRL_XFER && dir == UHDIR_IN)
            target |= 0x10;
        KPRINTF(10, ("New %s transfer to %ld.%ld: %ld bytes\n",
                xfer_names[xfer_type], ioreq->iouh_DevAddr,
                ioreq->iouh_Endpoint, ioreq->iouh_Length));
        /* is endpoint already in use or do we have to wait for next
         * transaction */
        if (unit->hu_DevBusyReq[target])
        {
            KPRINTF(5, ("Endpoint %02lx in use!\n", target));
            ioreq = (struct IOUsbHWReq *)((struct Node *)ioreq)->ln_Succ;
            continue;
        }

        success = ScheduleED(hc, xfer_type, ioreq);

        ioreq = (struct IOUsbHWReq *)hc->hc_XferQueues[xfer_type].lh_Head;
    }

    if (success)
    {
        /*
         * If we are going to start the queue but it's not running yet,
         * reset current ED pointer to zero. This will cause the HC to
         * start over from the head.
         */
        startmask = start_masks[xfer_type];
        oldenables = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
        if (!(oldenables & startmask))
        {
            CONSTWRITEREG32_LE(hc->hc_RegBase, current_ed_regs[xfer_type],
                0);
        }
    }

    return startmask;
}
/* \\\ */

/* /// "UpdateFrameCounter()" */
void UpdateFrameCounter(struct PCIController *hc)
{

    Disable();
    hc->hc_FrameCounter =
        (hc->hc_FrameCounter & 0xffff0000) | (READREG32_LE(hc->hc_RegBase,
            OHCI_FRAMECOUNT) & 0xffff);
    Enable();
}
/* \\\ */

/* /// "CompleteInt()" */
static AROS_UFIH1(CompleteInt, struct PCIController *, hc)
{
    AROS_USERFUNC_INIT ULONG restartmask = 0;

    KPRINTF(1, ("CompleteInt!\n"));

    UpdateFrameCounter(hc);

    /* **************** PROCESS DONE TRANSFERS **************** */

    WRITEREG32_LE(&hc->hc_RegBase, OHCI_INTDIS, OISF_DONEHEAD);
    if (hc->hc_DoneQueue)
        HandleFinishedTDs(hc);

    if (hc->hc_Flags & HCF_ABORT)
        restartmask = HandleAbortedEDs(hc);
    WRITEREG32_LE(&hc->hc_RegBase, OHCI_INTEN, OISF_DONEHEAD);

    if ((!(hc->hc_Flags & HCF_STOP_CTRL))
        && hc->hc_XferQueues[CTRL_XFER].lh_Head->ln_Succ)
        restartmask |= ScheduleXfers(hc, CTRL_XFER);

    if (hc->hc_XferQueues[INT_XFER].lh_Head->ln_Succ)
        ScheduleXfers(hc, INT_XFER);

    if ((!(hc->hc_Flags & HCF_STOP_BULK))
        && hc->hc_XferQueues[BULK_XFER].lh_Head->ln_Succ)
        restartmask |= ScheduleXfers(hc, BULK_XFER);

    /*
     * Restart queues. In restartmask we have accumulated which queues need
     * to be started.
     *
     * We do it here only once, after everything is set up, because
     * otherwise HC goes nuts in some cases. For example, the following
     * situation caused TD queue loop: we are simultaneously scheduling two
     * control EDs and one of them completes with error. If we attempt to
     * start the queue right after an ED is scheduled (this is how the code
     * originally worked), it looks like the HC manages to deal with the
     * first ED right before the second one is scheduled. At this moment the
     * first TD is HALTed with HeadPtr pointing to the failed TD, which went
     * to the DoneQueue (which will be picked up only on next ISR round, we
     * are still in ScheduleCtrlEDs()). The second ED is scheduled (first
     * one is not removed yet!) and we re-trigger control queue to start.
     * It causes errorneous TD to reappear on the DoneQueue, effectively
     * looping it. DoneQueue loop causes HandleFinishedTDs() to never exit.
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

AROS_USERFUNC_EXIT}
/* \\\ */

/* /// "IntCode()" */
static AROS_UFIH1(IntCode, struct PCIController *, hc)
{
    AROS_USERFUNC_INIT struct PCIUnit *unit = hc->hc_Unit;
    ULONG intr = 0;
    ULONG donehead;
    ULONG dma_size;

    dma_size = sizeof(struct HCCA);
    CachePostDMA(hc->hc_HCCA, &dma_size, 0);

    donehead = READMEM32_LE(&hc->hc_HCCA->ha_DoneHead);

    if (donehead)
    {
        if (donehead & ~1)
            intr = OISF_DONEHEAD;
        if (donehead & 1)
        {
            intr |= READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS);
        }
        donehead &= OHCI_PTRMASK;

        CONSTWRITEMEM32_LE(&hc->hc_HCCA->ha_DoneHead, 0);
    }
    else
    {
        intr = READREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS);

        if (intr & OISF_DONEHEAD)
        {
            KPRINTF(1, ("DONEHEAD WAS EMPTY!\n"));
            donehead =
                READMEM32_LE(&hc->hc_HCCA->ha_DoneHead) & OHCI_PTRMASK;
            CONSTWRITEMEM32_LE(&hc->hc_HCCA->ha_DoneHead, 0);

            KPRINTF(500, ("New Donehead %08lx for old %08lx\n", donehead,
                    hc->hc_DoneQueue));
        }
    }
    dma_size = sizeof(struct HCCA);
    CachePreDMA(hc->hc_HCCA, &dma_size, 0);

    intr &= ~OISF_MASTERENABLE;

    if (intr & hc->hc_PCIIntEnMask)
    {
        KPRINTF(1, ("IntCode(0x%p) interrupts 0x%08lx, mask 0x%08lx\n", hc,
                intr, hc->hc_PCIIntEnMask));

        if (intr & OISF_HOSTERROR)
        {
            KPRINTF(200, ("Host ERROR!\n"));
        }
        if (intr & OISF_SCHEDOVERRUN)
        {
            KPRINTF(200, ("Schedule overrun!\n"));
        }
        if (!(hc->hc_Flags & HCF_ONLINE))
        {
            if (READREG32_LE(hc->hc_RegBase,
                    OHCI_INTSTATUS) & OISF_HUBCHANGE)
            {
                // if the driver is not online and the controller has a broken
                // hub change interrupt, make sure we don't run into infinite
                // interrupt by disabling the interrupt bit
                DisableInt(hc, OISF_HUBCHANGE);
            }
            return FALSE;
        }
        WRITEREG32_LE(hc->hc_RegBase, OHCI_INTEN, OISF_HUBCHANGE);
        if (intr & OISF_FRAMECOUNTOVER)
        {
            hc->hc_FrameCounter |= 0x7fff;
            hc->hc_FrameCounter++;
            hc->hc_FrameCounter |=
                READMEM16_LE(&hc->hc_HCCA->ha_FrameCount);
            KPRINTF(10, ("HCI 0x%p: Frame Counter Rollover %ld\n", hc,
                    hc->hc_FrameCounter));
        }
        if (intr & OISF_HUBCHANGE)
        {
            UWORD hciport;
            ULONG oldval;
            UWORD portreg = OHCI_PORTSTATUS;
            BOOL clearbits = FALSE;

            if (READREG32_LE(hc->hc_RegBase,
                    OHCI_INTSTATUS) & OISF_HUBCHANGE)
            {
                /* Some OHCI implementations will keep the interrupt bit
                 * stuck until all port changes have been cleared, which is
                 * wrong according to the OHCI spec. As a workaround we will
                 * clear all change bits, which should be no problem as the
                 * port changes are reflected in the PortChangeMap array.
                 */
                clearbits = TRUE;
            }
            for (hciport = 0; hciport < hc->hc_NumPorts;
                hciport++, portreg += 4)
            {
                oldval = READREG32_LE(hc->hc_RegBase, portreg);
                hc->hc_PortChangeMap[hciport] |= TranslatePortFlags(oldval,
                    OHPF_OVERCURRENTCHG | OHPF_RESETCHANGE |
                    OHPF_ENABLECHANGE | OHPF_CONNECTCHANGE |
                    OHPF_RESUMEDTX);
                if (clearbits)
                {
                    WRITEREG32_LE(hc->hc_RegBase, portreg,
                        OHPF_CONNECTCHANGE | OHPF_ENABLECHANGE |
                        OHPF_RESUMEDTX | OHPF_OVERCURRENTCHG |
                        OHPF_RESETCHANGE);
                }

                KPRINTF(20, ("PCI Int Port %ld (glob %ld) Change %08lx\n",
                        hciport, hc->hc_PortNum20[hciport] + 1, oldval));
                if (hc->hc_PortChangeMap[hciport])
                {
                    unit->hu_RootPortChanges |=
                        1UL << (hc->hc_PortNum20[hciport] + 1);
                }
            }
            CheckRootHubChanges(unit);
            if (clearbits)
            {
                // again try to get rid of any bits that may be causing the
                // interrupt
                WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBSTATUS,
                    OHSF_OVERCURRENTCHG);
                WRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS,
                    OISF_HUBCHANGE);
            }
        }
        if (intr & OISF_DONEHEAD)
        {
            KPRINTF(10, ("DoneHead Frame=%ld\n",
                    READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));

            if (hc->hc_DoneQueue)
            {
                struct TDNode *donetd =
                    (struct TDNode *)((IPTR) donehead -
                    hc->hc_PCIVirtualAdjust - offsetof(struct TDNode,
                        td_TD.Ctrl));

                CacheClearE(&donetd->td_TD, 16, CACRF_InvalidateD);
                while (donetd->td_TD.NextTD)
                {
                    donetd =
                        (struct TDNode *)((IPTR) donetd->td_TD.NextTD -
                        hc->hc_PCIVirtualAdjust - offsetof(struct TDNode,
                            td_TD.Ctrl));
                    CacheClearE(&donetd->td_TD, 16, CACRF_InvalidateD);
                }
                WRITEMEM32_LE(&donetd->td_TD.NextTD, hc->hc_DoneQueue);
                CacheClearE(&donetd->td_TD, 16, CACRF_ClearD);

                KPRINTF(10,
                    ("Attached old DoneHead 0x%08lx to TD 0x%08lx\n",
                        hc->hc_DoneQueue, donetd->td_Self));
            }
            hc->hc_DoneQueue = donehead;
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
            Cause(&hc->hc_CompleteInt);
        }

        KPRINTF(1, ("Exiting IntCode(0x%p)\n", unit));
    }

    WRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, intr);

    /* Unlock interrupts  */
    WRITEREG32_LE(&hc->hc_RegBase, OHCI_INTEN, OISF_MASTERENABLE);

    return FALSE;

AROS_USERFUNC_EXIT}
/* \\\ */

/* /// "AbortRequest()" */
void AbortRequest(struct PCIController *hc, struct IOUsbHWReq *ioreq)
{
    struct PCIUnit *unit = hc->hc_Unit;
    struct EDNode *ed = ioreq->iouh_DriverPrivate1;
    UWORD target =
        (ioreq->iouh_DevAddr << 5) + ioreq->iouh_Endpoint +
        ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
    ULONG disablemask = 0;
    ULONG ctrlstatus;

    KPRINTF(70, ("HC 0x%p Aborting request 0x%p, command %ld, "
        "endpoint 0x%04lx, Frame=%ld\n",
        hc, ioreq, ioreq->iouh_Req.io_Command, target,
        READREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT)));
    PrintED("Aborting", ed, hc);

    /* Removing control and bulk EDs requires to stop the appropriate HC
     * queue first (according to specification) */
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
    DisableED(ed);

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
        UpdateIntTree(hc);

    unit->hu_DevDataToggle[target] =
        (READMEM32_LE(&ed->ed_ED.HeadPtr) & OEHF_DATA1) ? TRUE : FALSE;

    /*
     * Request StartOfFrame interrupt. Upon next frame this ED
     * is guaranteed to be out of use and can be freed.
     */
    EnableInt(hc, OISF_SOF);
}
/* \\\ */

/* /// "InitController()" */
BOOL InitController(struct PCIController *hc, struct PCIUnit *hu)
{

    struct PCIDevice *hd = hu->hu_Device;

    struct EDNode *ed;
    struct TDNode *td;
    ULONG *tabptr;
    UBYTE *memptr;
    ULONG hubdesca;
    ULONG cmdstatus;
    ULONG control;
    ULONG timeout;
    ULONG frameival;
    UWORD i;
    ULONG cnt;
    ULONG dma_size;
    ULONG phy_addr;

    struct TagItem pciActivateMem[] = {
        {aHidd_PCIDevice_isMEM, TRUE},
        {TAG_DONE, 0UL},
    };

    struct TagItem pciActivateBusmaster[] = {
        {aHidd_PCIDevice_isMaster, TRUE},
        {TAG_DONE, 0UL},
    };

    struct TagItem pciDeactivateBusmaster[] = {
        {aHidd_PCIDevice_isMaster, FALSE},
        {TAG_DONE, 0UL},
    };

    hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_CompleteInt.is_Node.ln_Name = "OHCI CompleteInt";
    hc->hc_CompleteInt.is_Node.ln_Pri = 0;
    hc->hc_CompleteInt.is_Data = hc;
    hc->hc_CompleteInt.is_Code = (VOID_FUNC) CompleteInt;

    hc->hc_PCIMemSize = OHCI_HCCA_SIZE + OHCI_HCCA_ALIGNMENT + 1;
    hc->hc_PCIMemSize += sizeof(struct EDNode) * OHCI_ED_POOLSIZE;
    hc->hc_PCIMemSize += sizeof(struct TDNode) * OHCI_TD_POOLSIZE;

    memptr =
        HIDD_PCIDriver_AllocPCIMem(hc->hc_PCIDriverObject,
        hc->hc_PCIMemSize);
    hc->hc_PCIMem = (APTR) memptr;
    if (memptr)
    {
        // PhysicalAddress - VirtualAdjust = VirtualAddress
        // VirtualAddress  + VirtualAdjust = PhysicalAddress
        hc->hc_PCIVirtualAdjust =
            pciGetPhysical(hc, memptr) - (APTR) memptr;
        KPRINTF(10, ("VirtualAdjust 0x%08lx\n", hc->hc_PCIVirtualAdjust));

        // align memory
        memptr =
            (UBYTE *) (((IPTR) hc->hc_PCIMem +
                OHCI_HCCA_ALIGNMENT) & (~OHCI_HCCA_ALIGNMENT));
        hc->hc_HCCA = (struct HCCA *)memptr;
        KPRINTF(10, ("HCCA 0x%p\n", hc->hc_HCCA));
        memptr += OHCI_HCCA_SIZE;

        // build up ED pool
        NewList((struct List *)&hc->hc_FreeEDList);
        ed = (struct EDNode *)memptr;
        cnt = OHCI_ED_POOLSIZE;
        do
        {
            // minimal initialization
            AddTail((struct List *)&hc->hc_FreeEDList, (struct Node *)ed);
            NewList((struct List *)&ed->ed_TDList);
            WRITEMEM32_LE(&ed->ed_Self,
                (IPTR) (&ed->ed_ED.EPCaps) + hc->hc_PCIVirtualAdjust);
            ed++;
        }
        while (--cnt);
        memptr += sizeof(struct EDNode) * OHCI_ED_POOLSIZE;

        // build up TD pool
        NewList((struct List *)&hc->hc_FreeTDList);
        td = (struct TDNode *)memptr;
        cnt = OHCI_TD_POOLSIZE - 1;
        do
        {
            AddTail((struct List *)&hc->hc_FreeTDList, (struct Node *)td);
            WRITEMEM32_LE(&td->td_Self,
                (IPTR) (&td->td_TD.Ctrl) + hc->hc_PCIVirtualAdjust);
            td++;
        }
        while (--cnt);
        WRITEMEM32_LE(&td->td_Self,
            (IPTR) (&td->td_TD.Ctrl) + hc->hc_PCIVirtualAdjust);
        memptr += sizeof(struct TDNode) * OHCI_TD_POOLSIZE;

        // terminating ED
        hc->hc_TermED = ed = AllocED(hc);
        ed->ed_Node.mln_Succ = NULL;
        ed->ed_Node.mln_Pred = NULL;
        CONSTWRITEMEM32_LE(&ed->ed_ED.EPCaps, OECF_SKIP);
        ed->ed_ED.NextED = 0L;

        // terminating TD
        hc->hc_TermTD = td = AllocTD(hc);
        td->td_Node.mln_Succ = NULL;
        td->td_Node.mln_Pred = NULL;
        td->td_TD.NextTD = 0;

        for (cnt = 0; cnt < XFER_COUNT + INT_LIST_COUNT - 1; cnt++)
            NewList((struct List *)&hc->hc_EDLists[cnt]);

        UpdateIntTree(hc);

        // fill in framelist with IntED entry points based on interval
        tabptr = hc->hc_HCCA->ha_IntEDs;
        for (cnt = 0; cnt < 32; cnt++)
        {
            *tabptr++ = hc->hc_TermED->ed_Self;
        }

        // time to initialize hardware...
        OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base0,
            (IPTR *) & hc->hc_RegBase);
        hc->hc_RegBase = (APTR) (((IPTR) hc->hc_RegBase) & (~0xf));
        KPRINTF(10, ("RegBase = 0x%p\n", hc->hc_RegBase));

        // enable memory
        OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *)pciActivateMem);

        hubdesca = READREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA);
        hc->hc_NumPorts = (hubdesca & OHAM_NUMPORTS) >> OHAS_NUMPORTS;
        KPRINTF(20, ("Found OHCI Controller %p FuncNum = %ld, Rev %02lx, "
            "with %ld ports\n",
            hc->hc_PCIDeviceObject, hc->hc_FunctionNum,
            READREG32_LE(hc->hc_RegBase, OHCI_REVISION) & 0xFF,
            hc->hc_NumPorts));

        KPRINTF(20, ("Powerswitching: %s %s\n",
            hubdesca & OHAF_NOPOWERSWITCH ? "Always on" : "Available",
            hubdesca & OHAF_INDIVIDUALPS ? "per port" : "global"));

        control = READREG32_LE(hc->hc_RegBase, OHCI_CONTROL);
        KPRINTF(10, ("OHCI control state: 0x%08lx\n", control));

        // disable BIOS legacy support
        if (control & OCLF_SMIINT)
        {
            KPRINTF(10,
                ("BIOS still has hands on OHCI, trying to get rid of it\n"));

            cmdstatus = READREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS);
            cmdstatus |= OCSF_OWNERCHANGEREQ;
            WRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, cmdstatus);
            timeout = 100;
            do
            {
                control = READREG32_LE(hc->hc_RegBase, OHCI_CONTROL);
                if (!(control & OCLF_SMIINT))
                {
                    KPRINTF(10, ("BIOS gave up on OHCI. Pwned!\n"));
                    break;
                }
                DelayMS(10, hu);
            }
            while (--timeout);
            if (!timeout)
            {
                KPRINTF(10,
                    ("BIOS didn't release OHCI. Forcing and praying...\n"));
                control &= ~OCLF_SMIINT;
                WRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL, control);
            }
        }

        OOP_SetAttrs(hc->hc_PCIDeviceObject,
            (struct TagItem *)pciDeactivateBusmaster); // no busmaster yet

        KPRINTF(10, ("Resetting OHCI HC\n"));
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, OCSF_HCRESET);
        cnt = 100;
        do
        {
            if (!(READREG32_LE(hc->hc_RegBase,
                        OHCI_CMDSTATUS) & OCSF_HCRESET))
            {
                break;
            }
            DelayMS(1, hu);
        }
        while (--cnt);

#ifdef DEBUG
        if (cnt == 0)
        {
            KPRINTF(20, ("Reset Timeout!\n"));
        }
        else
        {
            KPRINTF(20, ("Reset finished after %ld ticks\n", 100 - cnt));
        }
#endif

        OOP_SetAttrs(hc->hc_PCIDeviceObject,
            (struct TagItem *)pciActivateBusmaster);   // enable busmaster

        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_FRAMECOUNT, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_PERIODICSTART, 10800);
            // 10% of 12000
        frameival = READREG32_LE(hc->hc_RegBase, OHCI_FRAMEINTERVAL);
        KPRINTF(10, ("FrameInterval=%08lx\n", frameival));
        frameival &= ~OIVM_BITSPERFRAME;
        frameival |= OHCI_DEF_BITSPERFRAME << OIVS_BITSPERFRAME;
        frameival |= OIVF_TOGGLE;
        WRITEREG32_LE(hc->hc_RegBase, OHCI_FRAMEINTERVAL, frameival);

        // make sure nothing is running
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_PERIODIC_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CTRL_HEAD_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CTRL_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_HEAD_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_BULK_ED, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_DONEHEAD, 0);

        dma_size = sizeof(struct HCCA);
        phy_addr =
            (ULONG) (IPTR) CachePreDMA(hc->hc_HCCA, &dma_size,
            DMA_ReadFromRAM);
        WRITEREG32_LE(hc->hc_RegBase, OHCI_HCCA, phy_addr);

        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, OISF_ALL_INTS);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTDIS, OISF_ALL_INTS);
        SYNC;

        // install reset handler
        hc->hc_ResetInt.is_Code = (VOID_FUNC) ResetHandler;
        hc->hc_ResetInt.is_Data = hc;
        AddResetCallback(&hc->hc_ResetInt);

        // add interrupt
        hc->hc_PCIIntHandler.is_Node.ln_Name =
            hu->hu_Device->hd_Library.lib_Node.ln_Name;
        hc->hc_PCIIntHandler.is_Node.ln_Pri = 5;
        hc->hc_PCIIntHandler.is_Code = (VOID_FUNC) IntCode;
        hc->hc_PCIIntHandler.is_Data = hc;
        AddIntServer(INTB_KERNEL + hc->hc_PCIIntLine,
            &hc->hc_PCIIntHandler);

        hc->hc_PCIIntEnMask =
            OISF_DONEHEAD | OISF_RESUMEDTX | OISF_HOSTERROR |
            OISF_FRAMECOUNTOVER | OISF_HUBCHANGE;

        WRITEREG32_LE(hc->hc_RegBase, OHCI_INTEN,
            hc->hc_PCIIntEnMask | OISF_MASTERENABLE);

        /* Reset controller twice (needed for some OHCI chips) */
        for (i = 0; i < 2; i++)
        {
            CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL,
                OCLF_PERIODICENABLE | OCLF_CTRLENABLE | OCLF_BULKENABLE |
                OCLF_ISOENABLE | OCLF_USBRESET);
            SYNC;
            KPRINTF(10, ("POST-RESET FrameInterval=%08lx\n",
                    READREG32_LE(hc->hc_RegBase, OHCI_FRAMEINTERVAL)));
            WRITEREG32_LE(hc->hc_RegBase, OHCI_FRAMEINTERVAL, frameival);
        }

        // make sure the ports are on with chipset quirk workaround
        hubdesca = READREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA);
        hubdesca |= OHAF_NOPOWERSWITCH;
        hubdesca &= ~OHAF_INDIVIDUALPS;
        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA, hubdesca);

        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBSTATUS, OHSF_POWERHUB);

        DelayMS(50, hu);
        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCA, hubdesca);

        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL,
            OCLF_PERIODICENABLE | OCLF_CTRLENABLE | OCLF_BULKENABLE |
            OCLF_ISOENABLE | OCLF_USBOPER);
        SYNC;

        KPRINTF(20, ("Init returns TRUE...\n"));
        return TRUE;
    }

    KPRINTF(1000, ("Init returns FALSE...\n"));
    return FALSE;
}
/* \\\ */

/* /// "FreeController()" */
void FreeController(struct PCIController *hc, struct PCIUnit *hu)
{

    hc = (struct PCIController *)hu->hu_Controllers.lh_Head;
    while (hc->hc_Node.ln_Succ)
    {
        KPRINTF(20, ("Shutting down OHCI %p\n", hc));
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTDIS, OISF_ALL_INTS);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_INTSTATUS, OISF_ALL_INTS);

        // disable all ports
        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBDESCB, 0);
        WRITEREG32_LE(hc->hc_RegBase, OHCI_HUBSTATUS, OHSF_UNPOWERHUB);

        DelayMS(50, hu);
        KPRINTF(20, ("Stopping OHCI %p\n", hc));
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CONTROL, 0);
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, 0);
        SYNC;

        //KPRINTF(20, ("Reset done OHCI %08lx\n", hc));
        DelayMS(10, hu);
        KPRINTF(20, ("Resetting OHCI %p\n", hc));
        CONSTWRITEREG32_LE(hc->hc_RegBase, OHCI_CMDSTATUS, OCSF_HCRESET);
        SYNC;
        DelayMS(50, hu);

        KPRINTF(20, ("Shutting down OHCI done.\n"));

        hc = (struct PCIController *)hc->hc_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "TranslatePortFlags()" */
UWORD TranslatePortFlags(ULONG flags, ULONG mask)
{
    UWORD new_flags = 0;

    flags &= mask;

    if (flags & OHPF_PORTPOWER)
        new_flags |= UPSF_PORT_POWER;
    if (flags & OHPF_OVERCURRENT)
        new_flags |= UPSF_PORT_OVER_CURRENT;
    if (flags & OHPF_PORTCONNECTED)
        new_flags |= UPSF_PORT_CONNECTION;
    if (flags & OHPF_PORTENABLE)
        new_flags |= UPSF_PORT_ENABLE;
    if (flags & OHPF_LOWSPEED)
        new_flags |= UPSF_PORT_LOW_SPEED;
    if (flags & OHPF_PORTRESET)
        new_flags |= UPSF_PORT_RESET;
    if (flags & OHPF_PORTSUSPEND)
        new_flags |= UPSF_PORT_SUSPEND;
    if (flags & OHPF_OVERCURRENTCHG)
        new_flags |= UPSF_PORT_OVER_CURRENT;
    if (flags & OHPF_RESETCHANGE)
        new_flags |= UPSF_PORT_RESET;
    if (flags & OHPF_ENABLECHANGE)
        new_flags |= UPSF_PORT_ENABLE;
    if (flags & OHPF_CONNECTCHANGE)
        new_flags |= UPSF_PORT_CONNECTION;
    if (flags & OHPF_RESUMEDTX)
        new_flags |= UPSF_PORT_SUSPEND;

    return new_flags;
}
/* \\\ */
