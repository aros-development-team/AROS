/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "usb2otg_intern.h"

AROS_INTH1(FNAME_DEV(PendingInt), struct USB2OTGUnit *, otg_Unit)
{
    AROS_INTFUNC_INIT

    D(bug("[USB2OTG] [0x%p:PEND] Pending Work Interupt\n", otg_Unit));

    /* **************** PROCESS DONE TRANSFERS **************** */

    FNAME_ROOTHUB(PendingIO)(otg_Unit);

//    FNAME_DEV(DoFinishedTDs)(otg_Unit);

    if (otg_Unit->hu_CtrlXFerQueue.lh_Head->ln_Succ)
    {
        D(bug("[USB2OTG] [0x%p:PEND] Process CtrlXFer ..\n", otg_Unit));
//        FNAME_DEV(ScheduleCtrlTDs)(otg_Unit);
    }

    if (otg_Unit->hu_IntXFerQueue.lh_Head->ln_Succ)
    {
        D(bug("[USB2OTG] [0x%p:PEND] Process IntXFer ..\n", otg_Unit));
//        FNAME_DEV(ScheduleIntTDs)(otg_Unit);
    }

    if (otg_Unit->hu_BulkXFerQueue.lh_Head->ln_Succ)
    {
        D(bug("[USB2OTG] [0x%p:PEND] Process BulkXFer ..\n", otg_Unit));
//        FNAME_DEV(ScheduleBulkTDs)(otg_Unit);
    }

    D(bug("[USB2OTG] [0x%p:PEND] finished\n", otg_Unit));

    return FALSE;

    AROS_INTFUNC_EXIT
}

AROS_INTH1(FNAME_DEV(NakTimeoutInt), struct USB2OTGUnit *, otg_Unit)
{
    AROS_INTFUNC_INIT

   // struct IOUsbHWReq *ioreq;

   D(bug("[USB2OTG] [0x%p:NAK] NakTimeout Interupt\n", otg_Unit));

//    ULONG framecnt;
//    uhciUpdateFrameCounter(hc);
//    framecnt = hc->hc_FrameCounter;
#if (0)
    ioreq = (struct IOUsbHWReq *) otg_Unit->hu_TDQueue.lh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT)
        {
/*            uqh = (struct UhciQH *) ioreq->iouh_DriverPrivate1;
            if(uqh)
            {
                KPRINTF(1, ("Examining IOReq=%p with UQH=%p\n", ioreq, uqh));
                devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                linkelem = READMEM32_LE(&uqh->uqh_Element);
                if(linkelem & UHCI_TERMINATE)
                {
                    KPRINTF(1, ("UQH terminated %08lx\n", linkelem));
                    if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                    {
                        // give the thing the chance to exit gracefully
                        KPRINTF(20, ("Terminated? NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                        causeint = TRUE;
                    }
                } else {
                    utd = (struct UhciTD *) (((IPTR)linkelem & UHCI_PTRMASK) - hc->hc_PCIVirtualAdjust - 16); // struct UhciTD starts 16 before physical TD
                    ctrlstatus = READMEM32_LE(&utd->utd_CtrlStatus);
                    if(ctrlstatus & UTCF_ACTIVE)
                    {
                        if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                        {
                            // give the thing the chance to exit gracefully
                            KPRINTF(20, ("NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                            ctrlstatus &= ~UTCF_ACTIVE;
                            WRITEMEM32_LE(&utd->utd_CtrlStatus, ctrlstatus);
                            causeint = TRUE;
                        }
                    } else {
                        if(framecnt > unit->hu_NakTimeoutFrame[devadrep])
                        {
                            // give the thing the chance to exit gracefully
                            KPRINTF(20, ("Terminated? NAK timeout %ld > %ld, IOReq=%p\n", framecnt, unit->hu_NakTimeoutFrame[devadrep], ioreq));
                            causeint = TRUE;
                        }
                    }
                }
            }*/
        }
        ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
    }

//    uhciCheckPortStatusChange(hc);
#endif
    FNAME_ROOTHUB(PendingIO)(otg_Unit);

    otg_Unit->hu_NakTimeoutReq.tr_time.tv_micro = 150 * 1000;
    SendIO((APTR) &otg_Unit->hu_NakTimeoutReq);

    D(bug("[USB2OTG] [0x%p:NAK] processed\n", otg_Unit));

    return FALSE;

    AROS_INTFUNC_EXIT
}
