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
    (bug("Regs for channel %d\n", channel));
    (bug("CHARBASE=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, CHARBASE))));
    (bug("SPLITCTRL=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, SPLITCTRL))));
    (bug("INTR=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, INTR))));
    (bug("INTRMASK=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, INTRMASK))));
    (bug("TRANSSIZE=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, TRANSSIZE))));
    (bug("DMAADR=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, DMAADDR))));
}
static LONG delayed_channel[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static void handle_SOF(struct USB2OTGUnit *USBUnit, struct ExecBase *SysBase, ULONG frnm_orig)
{
    ULONG frnm = frnm_orig >> 3;
    static ULONG last_frame = 0;
    struct IOUsbHWReq *req = NULL, *next = NULL;
    struct USB2OTGDevice * USB2OTGBase = USBUnit->hu_USB2OTGBase;

    wr32le(USB2OTG_INTR, USB2OTG_INTRCORE_DMASTARTOFFRAME);

    if (frnm < last_frame)
    {
        D(bug("[USB2OTG] SOF, frame wrap %d %d\n", frnm, last_frame));
#if 0
        bug("Ch2\n");
        DumpChannelRegs(CHAN_INT1);
        bug("Ch3\n");
        DumpChannelRegs(CHAN_INT2);
        bug("Ch4\n");
        DumpChannelRegs(CHAN_INT3);
        #endif
    }

    if (frnm != last_frame)
    {
        #if 0
         if (USBUnit->hu_CtrlXFerQueue.lh_Head->ln_Succ)
    {
        D(bug("[USB2OTG] [0x%p:PEND] Process CtrlXFer ..\n", otg_Unit));
        FNAME_DEV(ScheduleCtrlTDs)(USBUnit);
    }

    if (USBUnit->hu_IntXFerQueue.lh_Head->ln_Succ)
    {
//        D(bug("[USB2OTG] [0x%p:PEND] Process IntXFer ..\n", otg_Unit));
        FNAME_DEV(ScheduleIntTDs)(USBUnit);
    }

    if (USBUnit->hu_BulkXFerQueue.lh_Head->ln_Succ)
    {
        D(bug("[USB2OTG] [0x%p:PEND] Process BulkXFer ..\n", otg_Unit));
//        FNAME_DEV(ScheduleBulkTDs)(otg_Unit);
    }
#endif
        FNAME_DEV(Cause)(USB2OTGBase, &USBUnit->hu_PendingInt);

        ForeachNodeSafe(&USBUnit->hu_IntXFerQueue, req, next)
        {
            ULONG last_handled = (ULONG)req->iouh_DriverPrivate1 >> 16;
            ULONG next_to_handle = (ULONG)req->iouh_DriverPrivate1 & 0xfff;

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
                (frnm < last_handled && frnm < next_to_handle) ||
                (frnm > next_to_handle && frnm < last_handled)
            )
            {
                D(bug("[USB2OTG] overdued INT request, scheduling. last=%d, next=%d, frnm=%d\n", last_handled, next_to_handle, frnm));
                REMOVE(req);
                ADDTAIL(&USBUnit->hu_IntXFerScheduled, req);
            }
        }

        /* If the Scheduled list is not empty process it now */
        if (!IsListEmpty(&USBUnit->hu_IntXFerScheduled))
            FNAME_DEV(ScheduleIntTDs)(USBUnit);

        last_frame = frnm;


    }
    {
            int chan;
            for (chan=0; chan < 8; chan++)
            {
                if (delayed_channel[chan] != 0)
                {
                    delayed_channel[chan]--;
                    if (delayed_channel[chan] == 0)
                    {
    //                    bug("R%p ", frnm_orig);
                        FNAME_DEV(StartChannel)(USBUnit, chan, 1);
                    }
                }
            }

        }
}

void FNAME_DEV(GlobalIRQHandler)(struct USB2OTGUnit *USBUnit, struct ExecBase *SysBase)
{
    //static ULONG last_frame = 0;

    //static ULONG delayed_frnm = 0;
    volatile unsigned int otg_RegVal;
    //struct USB2OTGDevice * USB2OTGBase = USBUnit->hu_USB2OTGBase;
    ULONG frnm = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff);

    otg_RegVal = rd32le(USB2OTG_INTR);

    if (otg_RegVal & USB2OTG_INTRCORE_DMASTARTOFFRAME)
    {
        handle_SOF(USBUnit, SysBase, frnm);
    }

    frnm >>= 3;

    if (otg_RegVal & USB2OTG_INTRCORE_HOSTCHANNEL)
    {
        unsigned int otg_ChanVal;
        int chan;

        otg_ChanVal = rd32le(USB2OTG_HOSTINTR);


        //D(bug("[USB2OTG] HOSTCHANNEL %x frnm %d\n", otg_ChanVal, frnm));

        for (chan = 0; chan < 8; chan++)
        {


            if (otg_ChanVal & (1 << chan))
            {
                struct IOUsbHWReq * req = USBUnit->hu_Channel[chan].hc_Request;
                uint32_t intr = rd32le(USB2OTG_CHANNEL_REG(chan, INTR));
                wr32le(USB2OTG_CHANNEL_REG(chan, INTR), intr);


            if (rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & 0x40000000)
            {

                bug("disable bit on channel %d is set!!!\n", chan);

                bug("req=%08x\n", USBUnit->hu_Channel[chan].hc_Request);

                DumpChannelRegs(chan);
                wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), 0);
                bug("HOSTINTR=%08x\nHOSTINTRMASK=%08x\n", otg_ChanVal, rd32le(USB2OTG_HOSTINTRMASK));


            }

                if (req)
                {
                    //if (req->iouh_DevAddr == 4)
                    //    bug("Interrupt %04x for request\n", intr);

                    int do_split = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL)) & 0x80010000;
                    /*
                        Channel closed with ACK but not comleted yet and split is active. Complete split now, do not continue with
                        normal processing of this channel
                    */
                    if (intr == 0x22 && do_split == 0x80000000)
                    {
                        uint32_t tmp = rd32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL));
                        tmp |= 1 << 16;   /* Set "do complete split" */
                        wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), tmp);

                        D(bug("[USB2OTG] Completing split transaction in interrupt. SPLITCTRL=%08x\n", tmp));
//bug("%08x\n", rd32le(USB2OTG_CHANNEL_REG(chan, DMAADDR)));
#if 0
                        /* Enable channel again */
                        tmp = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                        tmp |= USB2OTG_HOSTCHAR_ENABLE;
                        wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), tmp);
                        //asm volatile("wfe");
#endif
#if 0
                        delayed_channel |= 1 << chan;
                        delayed_frnm = frnm;
#else
FNAME_DEV(StartChannel)(USBUnit, chan, 1);
#endif
                    }
                    else if (intr == 0x42 && do_split == 0x80010000)
                    {
#if 1
                        ULONG last = (ULONG)req->iouh_DriverPrivate1 >> 16;
                        ULONG thresh = (last + (ULONG)req->iouh_Interval / 2) % 2047;
//bug("0x%p ", rd32le(USB2OTG_HOSTFRAMENO));
//bug("0x%p ", rd32le(USB2OTG_HOSTFRAMENO));
//bug("NYET!");
//DumpChannelRegs(chan);
//bug("NYET! frnm=%d last=%d thresh=%d\n", frnm, last, thresh);

                        if ((chan >= CHAN_INT1 && chan <= CHAN_INT3)  &&
                            ((frnm > thresh && frnm > last) ||
                            (frnm < thresh && frnm < last) ||
                            (frnm > thresh && frnm < last)))
                        {
                            D(bug("[USB2OTG] restarting transaction... %x, %x, %d\n", frnm, req->iouh_DriverPrivate1, req->iouh_DriverPrivate2));

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
                            USBUnit->hu_Channel[chan].hc_Request = NULL;
                            req = NULL;
                        }
                        else
#endif
                        {
                            //D(bug("!!! Channel %d, Restarting CSPLIT phase! %d, %d, %d\n", chan, frnm, req->iouh_DriverPrivate1, req->iouh_DriverPrivate2));
                            #if 0
                            /* Enable channel again */
                            uint32_t tmp = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
                            tmp |= USB2OTG_HOSTCHAR_ENABLE;
                            wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), tmp);
                              #endif

                            //intr = 2;
        #if 1
        if ((chan >= CHAN_INT1 && chan <= CHAN_INT3))
        {
         //   if (req->iouh_Flags & UHFF_LOWSPEED)
         //       delayed_channel[chan] = 4;
         //   else
                FNAME_DEV(StartChannel)(USBUnit, chan, 1);
        }
            //delayed_channel[chan] = 8;
        else
                            delayed_channel[chan] = 16;
                          //  delayed_frnm = frnm;
#else
                            FNAME_DEV(StartChannel)(USBUnit, chan, 1);
#endif
                        }
                    }
                    else if ((intr == 0x12 && do_split) &&
                        (chan < CHAN_INT1 || chan > CHAN_INT3))
                    {
                        /* NAK response from split transaction. Restart the request from beginning */
                      /*  D(bug("[USB2OTG] Restarting split transaction\n"));

                        DumpChannelRegs(chan);*/

                        /* Clear interrupt flags */
                        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);

                        /* Disable channel */
                        if (rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE)) & 0x80000000)
                        {
                            bug("Channel completed but still active. Disabling now...\n");
                            wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), 0x40000000);
                        }
                        else
                            wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), 0);

                        /* Put transfer back into queue */
                        if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                            ADDHEAD(&USBUnit->hu_CtrlXFerQueue, req);
                        if (req->iouh_Req.io_Command == UHCMD_BULKXFER)
                            ADDHEAD(&USBUnit->hu_BulkXFerQueue, req);
                        if (req->iouh_Req.io_Command == UHCMD_INTXFER)
                            ADDHEAD(&USBUnit->hu_IntXFerQueue, req);

                        /* Mark channel free */
                        USBUnit->hu_Channel[chan].hc_Request = NULL;
                        req = NULL;
                    }
                    else
                    {
                        int cont = 0;

                        /* Ignore NAK on INT channels when reporting closed channel */
                        //if (intr != 0x12 || (chan < CHAN_INT1 && chan > CHAN_INT3))
                        //    D(bug("[USB2OTG] Channel %d closed. INTR=%08x\n", chan, intr));

                        if ((intr & 0x21) == 0x21)
                        {
                            req->iouh_Req.io_Error = 0;

                            if (!FNAME_DEV(AdvanceChannel)(USBUnit, chan))
                            {
                                cont = 1;
                                FNAME_DEV(StartChannel)(USBUnit, chan, 0);
                            }
                            else
                            {
                                //CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_InvalidateD);
                                #if 0
                                for (int i=0; i < 1; i++)
                                {

                                    (bug("[USB2OTG] Transfer completted: %d of %d transferred at %p\n[USB2OTG]",
                                        req->iouh_Actual, req->iouh_Length, req->iouh_Data));

                                    UBYTE *ptr = req->iouh_Data;
                                    int len = 16;
                                    if (len > req->iouh_Length)
                                        len = req->iouh_Length;

                                    for (int i=0; i < len; i++)
                                        (bug(" %02x", ptr[i]));

                                    (bug("\n"));

                                    if (*((ULONG*)req->iouh_Data) == 0xffffffff) {
                                        bug("[USB2OTG] Have not received?\n");
                                        while(1);
                                    }
                                }
                                #endif
                            }
                        }
                        else if (intr & 0x80)
                        {
                            req->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                            bug("0x80: ");
                            DumpChannelRegs(chan);
                        }
                        else if (intr & 0x08)
                        {
                            bug("0x08: ");
                            req->iouh_Req.io_Error = UHIOERR_STALL;
                            DumpChannelRegs(chan);
                        }
                        else if (intr & 0x10)
                        {
                            /* In case of INT requests NAK is silently ignored. Just put the request back to the IntXferQueue */
                            if (chan >= CHAN_INT1 && chan <= CHAN_INT3)
                            {
//                                if (req->iouh_DevAddr == 4)
//                                    bug("!NAK channel %d\n", chan);

                                /* Clear interrupt flags */
                                wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);

                                /* Disable channel */
                                wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), 0);

                                ADDTAIL(&USBUnit->hu_IntXFerQueue, req);
                                USBUnit->hu_Channel[chan].hc_Request = NULL;
                                req = NULL;
                            }
                            else
                            {
                                bug("0x10: ");
                                req->iouh_Req.io_Error = UHIOERR_NAK;
                                DumpChannelRegs(chan);
                            }
                        }
                        else if (intr & 0x100)
                        {
                            bug("0x100: ");
                            req->iouh_Req.io_Error = UHIOERR_BABBLE;
                            DumpChannelRegs(chan);
                        }
                        else if (intr & 0x200)
                        {
                            /* Frame overrun. Don't report problem, reinsert request for further processing */
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
                            USBUnit->hu_Channel[chan].hc_Request = NULL;
                            req = NULL;
                        }
                        else if (intr & 0x400)
                        {
                            bug("0x400: ");
                            req->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                            DumpChannelRegs(chan);
                        }
                        else
                        {
                            bug("Unknown %04x: ", intr);
                            DumpChannelRegs(chan);
                        }


                        /* If there is no transfer continuation on that channel then free it and reply the msg */
                        if (cont == 0)
                        {
                            USBUnit->hu_Channel[chan].hc_Request = NULL;
                            if (req) ADDTAIL(&USBUnit->hu_FinishedXfers, (struct Node *)req);//FNAME_DEV(TermIO)(req, USB2OTGBase);
                        }
                    }
                }
//intr...
            }
        }
    }
#if 0
    if (delayed_frnm != frnm)
    {
        int chan;
        for (chan=0; chan < 8; chan++)
        {
            if (delayed_channel & (1 << chan))
            {
                FNAME_DEV(StartChannel)(USBUnit, chan);
            }
        }
        delayed_channel = 0;
    }
#endif
    {
        ULONG reg, msk;


        reg = rd32le(USB2OTG_OTGINTR);

        if (reg)
        {
//            bug("leaving interrupt with OTGINTR active: %08x\n", reg);
        }

        reg = rd32le(USB2OTG_INTR);
        msk = rd32le(USB2OTG_INTRMASK);

        if (reg & msk)
        {
//            bug("leaving interrupt with INTR active: %08x %08x %08\n", reg, msk, reg&msk);
        }

        for (int i=0; i < 8; i++)
        {
            if (reg & (1 << i))
            {
                //bug("HOSTCHAN: ")
            }
        }
    }
}


AROS_INTH1(FNAME_DEV(PendingInt), struct USB2OTGUnit *, otg_Unit)
{
    AROS_INTFUNC_INIT

    //struct USB2OTGDevice * USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    //D(bug("[USB2OTG] [0x%p:PEND] Pending Work Interupt\n", otg_Unit));

    /* **************** PROCESS DONE TRANSFERS **************** */

    FNAME_ROOTHUB(PendingIO)(otg_Unit);

//    FNAME_DEV(DoFinishedTDs)(otg_Unit);

    struct IOUsbHWReq * req = NULL;

    while((req = (struct IOUsbHWReq *)REMHEAD(&otg_Unit->hu_FinishedXfers)))
    {
        FNAME_DEV(TermIO)(req, otg_Unit->hu_USB2OTGBase);
    }

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
