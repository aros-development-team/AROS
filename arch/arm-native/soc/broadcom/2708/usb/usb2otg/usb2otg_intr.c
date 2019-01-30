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

    struct USB2OTGDevice * USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    D(bug("[USB2OTG] [0x%p:PEND] Pending Work Interupt\n", otg_Unit));

    /* **************** PROCESS DONE TRANSFERS **************** */

    FNAME_ROOTHUB(PendingIO)(otg_Unit);

//    FNAME_DEV(DoFinishedTDs)(otg_Unit);

    if (otg_Unit->hu_CtrlXFerQueue.lh_Head->ln_Succ)
    {
        D(bug("[USB2OTG] [0x%p:PEND] Process CtrlXFer ..\n", otg_Unit));

        if (otg_Unit->hu_InProgressCtrlXFer == NULL)
        {
            struct IOUsbHWReq *req = (struct IOUsbHWReq *)REMHEAD(&otg_Unit->hu_CtrlXFerQueue);
            otg_Unit->hu_InProgressCtrlXFer = req;

            D(bug("[USB2OTG] [0x%p:PEND] CtrlXFer slot empty. Processing IOReq @%p\n", otg_Unit, otg_Unit->hu_InProgressCtrlXFer));

#if 0
            /* Use channel 0 for CTRL transfers */
            D(bug("CHARBASE=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE)));
            D(bug("SPLITCTRL=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_SPLITCTRL)));
            D(bug("INTR=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTR)));
            D(bug("INTRMASK=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTRMASK)));
            D(bug("TRANSSIZE=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_TRANSSIZE)));
            D(bug("DMAADR=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_DMAADDR)));

            D(bug("REQ Data addr=%08x\n", req->iouh_Data));
            D(bug("REQ Data len=%08x\n", 8));
#endif
            // Block all IRQs from channel 0
            uint32_t oldmask = rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTRMASK);
            uint32_t tmp;
            int dir;

            if (req->iouh_SetupData.bmRequestType & URTF_IN) dir = 1; else dir = 0;

            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTRMASK, 0);

            wr32le(USB2OTG_INTRMASK, 1 << 25);
            wr32le(USB2OTG_HOSTINTRMASK, 0);

            D(bug("[USB2OTG] [0x%p:PEND] SETUP stage. Addr: %d, Direction: %s, Buffer %p, size %d\n", otg_Unit,
                req->iouh_DevAddr, "OUT", &req->iouh_SetupData, 8));

            D(bug("[USB2OTG] [0x%p:PEND] bmReqType=%02x, bReq=%02x, wValue=%04x, wIndex=%04x, wLength=%04x\n",
                otg_Unit, req->iouh_SetupData.bmRequestType, req->iouh_SetupData.bRequest,
                AROS_LE2WORD(req->iouh_SetupData.wValue), AROS_LE2WORD(req->iouh_SetupData.wIndex),
                AROS_LE2WORD(req->iouh_SetupData.wLength)));

            CacheClearE(&req->iouh_SetupData, 32, CACRF_ClearD);
            if (req->iouh_Data)
            {
                if (dir)
                    CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_InvalidateD);
                else
                    CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_ClearD);
            }


            /* SETUP phase of the transfer, always OUT type. Send CTRL data */
            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE,
                ((req->iouh_DevAddr & 0x7f) << 22) |
                (1 << 20) |
                ((req->iouh_Endpoint & 0x0f) << 11) |
                ((req->iouh_MaxPktSize & 1023))
            );
            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_TRANSSIZE, (3 << 29) | (1 << 19) | 8);
            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_DMAADDR, 0xc0000000 | (ULONG)&req->iouh_SetupData);
            tmp = rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE);
            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE, tmp | 0x80000000);

            /* Wait for interrupt (masked) */
            do {
                tmp = rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTR);
                D(bug("[INTR Reg] %08x\n", tmp));
            } while ((tmp & USB2OTG_INTRCHAN_HALT) == 0);

            if ((tmp & USB2OTG_INTRCHAN_TRANSFERCOMPLETE) == 0)
            {
                D(bug("[USB2OTG] [0x%p:PEND] Channel closed but transfer failed. INTR=%08x\n", otg_Unit, tmp));

            D(bug("CHARBASE=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE)));
            D(bug("SPLITCTRL=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_SPLITCTRL)));
            D(bug("INTR=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTR)));
            D(bug("INTRMASK=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTRMASK)));
            D(bug("TRANSSIZE=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_TRANSSIZE)));
            D(bug("DMAADR=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_DMAADDR)));
{
                volatile uint32_t *ptr = (volatile uint32_t *)USB2OTG_OTGCTRL;
                for (int i=0; i < 0x108/4; i++)
                    D(bug("%04x: %08x\n", i * 4, AROS_LE2LONG(ptr[i])));
                for (int i=0x400/4; i < 0x800/4; i++)
                    D(bug("%04x: %08x\n", i * 4, AROS_LE2LONG(ptr[i])));
            }
                req->iouh_Actual = 0;
                req->iouh_Req.io_Error = UHIOERR_STALL;
                otg_Unit->hu_InProgressCtrlXFer = NULL;
                FNAME_DEV(TermIO)(req, USB2OTGBase);
            }

            /* Clear interrupts */
            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTR, 0xffffffff);

            /* DATA phase if there is any data to transfer */
            if (req->iouh_Length && req->iouh_Data)
            {
                D(bug("[USB2OTG] [0x%p:PEND] DATA stage. Direction: %s, Buffer %p, size %d\n", otg_Unit,
                    dir ? "IN":"OUT", req->iouh_Data, req->iouh_Length));

                wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE,
                    ((req->iouh_DevAddr & 0x7f) << 22) |
                    (1 << 20) |
                    ((dir & 1) << 15) |
                    ((req->iouh_Endpoint & 0x0f) << 11) |
                    ((req->iouh_MaxPktSize & 1023))
                );
                wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_TRANSSIZE, (2 << 29) | (1 << 19) | req->iouh_Length);
                wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_DMAADDR, 0xc0000000 | (ULONG)req->iouh_Data);
                tmp = rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE);
                wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE, tmp | 0x80000000);

                /* Wait for interrupt (masked) */
                do {
                    tmp = rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTR);
                    D(bug("[INTR Reg] %08x\n", tmp));
                } while ((tmp & USB2OTG_INTRCHAN_HALT) == 0);

                if ((tmp & USB2OTG_INTRCHAN_TRANSFERCOMPLETE) == 0)
                {
                    D(bug("[USB2OTG] [0x%p:PEND] Channel closed but transfer failed. INTR=%08x\n", otg_Unit, tmp));

            D(bug("CHARBASE=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE)));
            D(bug("SPLITCTRL=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_SPLITCTRL)));
            D(bug("INTR=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTR)));
            D(bug("INTRMASK=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTRMASK)));
            D(bug("TRANSSIZE=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_TRANSSIZE)));
            D(bug("DMAADR=%08x\n", rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_DMAADDR)));
{
                volatile uint32_t *ptr = (volatile uint32_t *)USB2OTG_OTGCTRL;
                for (int i=0; i < 0x108/4; i++)
                    D(bug("%04x: %08x\n", i * 4, AROS_LE2LONG(ptr[i])));
                for (int i=0x400/4; i < 0x800/4; i++)
                    D(bug("%04x: %08x\n", i * 4, AROS_LE2LONG(ptr[i])));
            }
                    req->iouh_Actual = 0;
                    req->iouh_Req.io_Error = UHIOERR_STALL;
                    otg_Unit->hu_InProgressCtrlXFer = NULL;
                    FNAME_DEV(TermIO)(req, USB2OTGBase);
                }
            }

            /* Restore interrupts and prepare for final stage */
            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTRMASK, oldmask);

            int sta_dir;
            if (req->iouh_Length == 0 || dir == 0)
                sta_dir = 1;
            else
                sta_dir = 0;

            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE,
                ((req->iouh_DevAddr & 0x7f) << 22) |
                (1 << 20) |
                ((sta_dir & 1) << 15) |
                ((req->iouh_Endpoint & 0x0f) << 11) |
                ((req->iouh_MaxPktSize & 1023))
            );
            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_TRANSSIZE, (2 << 29) | (1 << 19) | 0);

            tmp = rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE);
            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_CHARBASE, tmp | 0x80000000);

            do {
                tmp = rd32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTR);
                D(bug("[INTR Reg] %08x\n", tmp));
            } while ((tmp & USB2OTG_INTRCHAN_HALT) == 0);

            D(bug("[USB2OTG] [0x%p:PEND] Channel closed. INTR=%08x\n", otg_Unit, tmp));

            if (tmp == 0x23)
            {
                req->iouh_Actual = req->iouh_Length;
                req->iouh_Req.io_Error = 0;
            }
            else if (tmp & 0x80)
            {
                req->iouh_Actual = 0;
                req->iouh_Req.io_Error = UHIOERR_TIMEOUT;
            }
            else if (tmp & 0x08)
            {
                req->iouh_Actual = 0;
                req->iouh_Req.io_Error = UHIOERR_STALL;
            }
            else if (tmp & 0x10)
            {
                req->iouh_Actual = 0;
                req->iouh_Req.io_Error = UHIOERR_NAK;
            }
            else if (tmp & 0x100)
            {
                req->iouh_Actual = 0;
                req->iouh_Req.io_Error = UHIOERR_BABBLE;
            }
            otg_Unit->hu_InProgressCtrlXFer = NULL;
            FNAME_DEV(TermIO)(req, USB2OTGBase);
            wr32le(USB2OTG_HOST_CHANBASE + USB2OTG_HOSTCHAN_INTR, tmp);
#if 0
            {
                volatile uint32_t *ptr = (volatile uint32_t *)USB2OTG_OTGCTRL;
                for (int i=0; i < 0x108/4; i++)
                    D(bug("%04x: %08x\n", i * 4, AROS_LE2LONG(ptr[i])));
                for (int i=0x400/4; i < 0x800/4; i++)
                    D(bug("%04x: %08x\n", i * 4, AROS_LE2LONG(ptr[i])));
            }
#endif
        }
            // Restore IRQ mask
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
