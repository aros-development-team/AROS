/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "usb2otg_intern.h"

static void DumpChannelRegs(int channel)
{
    D(bug("CHARBASE=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, CHARBASE))));
    D(bug("SPLITCTRL=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, SPLITCTRL))));
    D(bug("INTR=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, INTR))));
    D(bug("INTRMASK=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, INTRMASK))));
    D(bug("TRANSSIZE=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, TRANSSIZE))));
    D(bug("DMAADR=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, DMAADDR))));
}

static void handle_SOF(struct USB2OTGUnit *USBUnit, struct ExecBase *SysBase, ULONG frnm)
{
    static ULONG last_frame = 0;
    struct IOUsbHWReq *req = NULL, *next = NULL;

    wr32le(USB2OTG_INTR, USB2OTG_INTRCORE_DMASTARTOFFRAME);

    if (frnm < last_frame)
    {
        D(bug("[USB2OTG] SOF, frame wrap %d %d\n", frnm, last_frame));
    }

    if (frnm != last_frame)
    {
        ForeachNodeSafe(&USBUnit->hu_IntXFerQueue, req, next)
        {
            ULONG last_handled = (ULONG)req->iouh_DriverPrivate1;
            ULONG next_to_handle = (ULONG)req->iouh_DriverPrivate2;

            /* Is it time to handle the request? If yes, move it to Scheduled list */
            if (frnm == next_to_handle)
            {
                //D(bug("[USB2OTG] schedule INT request on time, last=%d, next=%d, frnm=%d\n", last_handled, next_to_handle, frnm));
                REMOVE(req);
                ADDTAIL(&USBUnit->hu_IntXFerScheduled, req);
            }
            /*
                If the request is overdued several scenarios are possible:
                1. frnm is larger than "last" and "next"
                    0......last....next...frnm....2047
                2. frnm is smaller than "last" and "next"
                    0..frnm.....last....next......2047
                3. frnm is bigger than "next" but smaller than "last"
                    0..next....frnm......last.....2047
            */
            else if (
                (frnm > next_to_handle && frnm > last_handled) ||
                (frnm < last_handled && frnm > next_to_handle) ||
                (frnm > next_to_handle && frnm < last_handled)
            )
            {
                //D(bug("[USB2OTG] overdued INT request, scheduling. last=%d, next=%d, frnm=%d\n", last_handled, next_to_handle, frnm));
                REMOVE(req);
                ADDTAIL(&USBUnit->hu_IntXFerScheduled, req);
            }
        }

        /* If the Scheduled list is not empty process it now */
        if (!IsListEmpty(&USBUnit->hu_IntXFerScheduled))
            FNAME_DEV(ScheduleIntTDs)(USBUnit);

        last_frame = frnm;
    }
}

void FNAME_DEV(GlobalIRQHandler)(struct USB2OTGUnit *USBUnit, struct ExecBase *SysBase)
{
    volatile unsigned int otg_RegVal;
    struct USB2OTGDevice * USB2OTGBase = USBUnit->hu_USB2OTGBase;
    ULONG frnm = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;

    otg_RegVal = rd32le(USB2OTG_INTR);
    wr32le(USB2OTG_INTR, otg_RegVal);

    if (otg_RegVal & USB2OTG_INTRCORE_DMASTARTOFFRAME)
    {
        handle_SOF(USBUnit, SysBase, frnm);
    }

    if (otg_RegVal & USB2OTG_INTRCORE_HOSTCHANNEL)
    {
        unsigned int otg_ChanVal;
        int chan;

        otg_ChanVal = rd32le(USB2OTG_HOSTINTR);
        wr32le(USB2OTG_HOSTINTR, otg_ChanVal);

        //D(bug("[USB2OTG] HOSTCHANNEL %x frnm %d\n", otg_ChanVal, frnm));

        for (chan = 0; chan < 8; chan++)
        {
            if (otg_ChanVal & (1 << chan))
            {
                struct IOUsbHWReq * req = USBUnit->hu_InProgressXFer[chan];
                uint32_t tmp = rd32le(USB2OTG_CHANNEL_REG(chan, INTR));

                if (req)
                {
                    int do_split = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL)) & 0x80010000;
                    /*
                        Channel closed with ACK but not comleted yet and split is active. Complete split now, do not continue with
                        normal processing of this channel
                    */
                    if (tmp == 0x22 && do_split)
                    {

                        uint32_t split = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));
                        //D(bug("[USB2OTG] Completing split transaction in interrupt. SPLITCTRL=%08x\n", split));

                        split |= 1 << 16;   /* Set "do complete split" */
                        wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), split);

                        /* Clear interrupt flags */
                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);

                        /* Enable channel again */
                        tmp = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                        tmp |= USB2OTG_HOSTCHAR_ENABLE;
                        wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), tmp);
                    }
                    else if (tmp == 0x42 && do_split == 0x80010000)
                    {
                        ULONG last = (ULONG)req->iouh_DriverPrivate1;
                        ULONG thresh = (last + (ULONG)req->iouh_Interval / 2) % 2047;

                        /* Clear interrupt flags */
                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);

                        if ((chan >= CHAN_INT1 && chan <= CHAN_INT3)  &&
                            ((frnm > thresh && frnm > last) ||
                            (frnm < thresh && frnm < last) ||
                            (frnm > thresh && frnm < last)))
                        {
                            //D(bug("[USB2OTG] restarting transaction... %d, %d, %d\n", frnm, req->iouh_DriverPrivate1, req->iouh_DriverPrivate2));

                            /* Disable channel */
                            wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), 0);

                            /* Put transfer back into queue */
                            if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                                ADDHEAD(&USBUnit->hu_CtrlXFerQueue, req);
                            if (req->iouh_Req.io_Command == UHCMD_BULKXFER)
                                ADDHEAD(&USBUnit->hu_BulkXFerQueue, req);
                            if (req->iouh_Req.io_Command == UHCMD_INTXFER)
                                ADDHEAD(&USBUnit->hu_IntXFerQueue, req);

                            /* Mark channel free */
                            USBUnit->hu_InProgressXFer[chan] = NULL;
                        }
                        else
                        {
                            //D(bug("!!! Channel %d, Restarting CSPLIT phase! %d, %d, %d\n", chan, frnm, req->iouh_DriverPrivate1, req->iouh_DriverPrivate2));
                            /* Enable channel again */
                            tmp = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                            tmp |= USB2OTG_HOSTCHAR_ENABLE;
                            wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), tmp);
                        }

                    }
                    else if (tmp == 0x12 && do_split)
                        //&& (chan < CHAN_INT1 || chan > CHAN_INT3))
                    {
                        /* NAK response from split transaction. Restart the request from beginning */
                      /*  D(bug("[USB2OTG] Restarting split transaction\n"));

                        DumpChannelRegs(chan);*/

                        /* Clear interrupt flags */
                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);

                        /* Disable channel */
                        wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), 0);

                        /* Put transfer back into queue */
                        if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                            ADDHEAD(&USBUnit->hu_CtrlXFerQueue, req);
                        if (req->iouh_Req.io_Command == UHCMD_BULKXFER)
                            ADDHEAD(&USBUnit->hu_BulkXFerQueue, req);
                        if (req->iouh_Req.io_Command == UHCMD_INTXFER)
                            ADDHEAD(&USBUnit->hu_IntXFerQueue, req);

                        /* Mark channel free */
                        USBUnit->hu_InProgressXFer[chan] = NULL;
                    }
                    else
                    {
                        /* Ignore NAK on INT channels when reporting closed channel */
                        if (tmp != 0x12 || (chan < CHAN_INT1 && chan > CHAN_INT3))
                            D(bug("[USB2OTG] Channel %d closed. INTR=%08x\n", chan, tmp));

                        if (tmp == 0x23)
                        {
                            /* Determine number of packets involved in last transfer. If it is even, toggle
                               the PID (the OTG was toggling it itself) */
                            int txsize = rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE)) & 524287;
                            int pktcnt = (txsize + req->iouh_MaxPktSize - 1) / req->iouh_MaxPktSize;
                            if (pktcnt & 1)
                            {
                                /* Toggle PID */
                                USBUnit->hu_PIDBits[req->iouh_DevAddr] ^= (2 << (2 * req->iouh_Endpoint));
                            }
                            req->iouh_Actual += txsize;
                            req->iouh_Req.io_Error = 0;
                        }
                        else if (tmp & 0x80)
                        {
                            req->iouh_Actual = 0;
                            req->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                            DumpChannelRegs(chan);
                        }
                        else if (tmp & 0x08)
                        {
                            req->iouh_Actual = 0;
                            req->iouh_Req.io_Error = UHIOERR_STALL;
                            DumpChannelRegs(chan);
                        }
                        else if (tmp & 0x10)
                        {
                            /* In case of INT requests NAK is silently ignored. Just put the request back to the IntXferQueue */
                            if (chan >= CHAN_INT1 && chan <= CHAN_INT3)
                            {
                                ADDTAIL(&USBUnit->hu_IntXFerQueue, req);
                                req = NULL;
                            }
                            else
                            {
                                req->iouh_Actual = 0;
                                req->iouh_Req.io_Error = UHIOERR_NAK;
                                DumpChannelRegs(chan);
                            }
                        }
                        else if (tmp & 0x100)
                        {
                            req->iouh_Actual = 0;
                            req->iouh_Req.io_Error = UHIOERR_BABBLE;
                            DumpChannelRegs(chan);
                        }
                        else if (tmp & 0x400)
                        {
                            req->iouh_Actual = 0;
                            req->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                            DumpChannelRegs(chan);
                        }

                        USBUnit->hu_InProgressXFer[chan] = NULL;
                        if (req) FNAME_DEV(TermIO)(req, USB2OTGBase);

                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), tmp);
                    }
                }
            }
        }
    }

    FNAME_DEV(Cause)(USB2OTGBase, &USBUnit->hu_PendingInt);
}


AROS_INTH1(FNAME_DEV(PendingInt), struct USB2OTGUnit *, otg_Unit)
{
    AROS_INTFUNC_INIT

    //struct USB2OTGDevice * USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    //D(bug("[USB2OTG] [0x%p:PEND] Pending Work Interupt\n", otg_Unit));

    /* **************** PROCESS DONE TRANSFERS **************** */

    FNAME_ROOTHUB(PendingIO)(otg_Unit);

//    FNAME_DEV(DoFinishedTDs)(otg_Unit);

    if (otg_Unit->hu_CtrlXFerQueue.lh_Head->ln_Succ)
    {
        D(bug("[USB2OTG] [0x%p:PEND] Process CtrlXFer ..\n", otg_Unit));
        FNAME_DEV(ScheduleCtrlTDs)(otg_Unit);
    }

    if (otg_Unit->hu_IntXFerQueue.lh_Head->ln_Succ)
    {
//        D(bug("[USB2OTG] [0x%p:PEND] Process IntXFer ..\n", otg_Unit));
        FNAME_DEV(ScheduleIntTDs)(otg_Unit);
    }

    if (otg_Unit->hu_BulkXFerQueue.lh_Head->ln_Succ)
    {
        D(bug("[USB2OTG] [0x%p:PEND] Process BulkXFer ..\n", otg_Unit));
//        FNAME_DEV(ScheduleBulkTDs)(otg_Unit);
    }

    //D(bug("[USB2OTG] [0x%p:PEND] finished\n", otg_Unit));

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
