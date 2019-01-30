/*
    Copyright � 2013-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
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

/*
    Scheduling control transfers is splitted into at least two phases:
    1. Setup phase, where ioreq->iouh_SetupData is sent
    2. Optional data phase
    3. ACK phase where either host or device respond with an ACK.

    Scheduling the transfer has to be splitted into these phases in the code, it is not possible
    to let OTG to join the transfers. Therefore, the function first disables the interrupts for
    the selected channel and enables them only for the last stage.
*/
void FNAME_DEV(ScheduleCtrlTDs)(struct USB2OTGUnit *otg_Unit)
{
    struct USB2OTGDevice * USB2OTGBase = otg_Unit->hu_USB2OTGBase;
    struct IOUsbHWReq *req = NULL;

    D(bug("[USB2OTG] %s(%p)\n", __PRETTY_FUNCTION__, otg_Unit));

    /* First try to get head of the transfer queue */
    Disable();
    req = (struct IOUsbHWReq *)REMHEAD(&otg_Unit->hu_CtrlXFerQueue);

    /* If there was any request in the queue, try to put it into InProgress slot */
    if (req)
    {
        uint32_t oldmask = 0;
        uint32_t tmp = 0;
        uint8_t direction = 0;
        int do_split = 0;
        int low_speed = 0;

        /* InProgress slot empty? If yes store the request there. Otherwise put it back to the queue */
        if (otg_Unit->hu_InProgressXFer[CHAN_CTRL] == NULL) {
            otg_Unit->hu_InProgressXFer[CHAN_CTRL] = req;
        } else {
            ADDHEAD(&otg_Unit, req);
            Enable();
            return;
        }
        Enable();

        /* At this point we have marked our request as active and it can be sent out to the device */
        D(bug("\nControlTransfer: %02x %02x %04x %04x %04x\n",
            req->iouh_SetupData.bmRequestType, req->iouh_SetupData.bRequest, AROS_LE2WORD(req->iouh_SetupData.wValue),
            AROS_LE2WORD(req->iouh_SetupData.wLength), AROS_LE2WORD(req->iouh_SetupData.wIndex)));

        if (req->iouh_Flags & UHFF_LOWSPEED)
        {
            D(bug("\n!!!!!!!!! LOW SPEED !!!!!!!!!!!!\n"));
            low_speed = 1 << 17;
        }
        if (req->iouh_Flags & UHFF_SPLITTRANS)
        {
            D(bug("\n!!!!!!!!! SPLIT TRANSACTION hub %d port %d !!!!!!!!!!!!\n", req->iouh_SplitHubAddr, req->iouh_SplitHubPort));
            do_split = 1;
        }

        /* Watch the SET_CONFIGURATION request since this one clears the toggle bits */
        if (req->iouh_SetupData.bmRequestType == 0 && req->iouh_SetupData.bRequest == USR_SET_CONFIGURATION)
        {
            otg_Unit->hu_PIDBits[req->iouh_DevAddr] = 0;    // Reset all endpoints to DATA0
        }

        /* Determine data direction */
        if (req->iouh_SetupData.bmRequestType & URTF_IN) direction = 1; else direction = 0;

        req->iouh_Actual = 0;

        /* Store old interrupt mask for the channel and make it silent */
        oldmask = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTRMASK));
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTRMASK), 0);

        /* Make sure interrupts from host channels are active */
        tmp = rd32le(USB2OTG_INTRMASK);
        tmp |= USB2OTG_INTRCORE_HOSTCHANNEL;
        wr32le(USB2OTG_INTRMASK, tmp);

        tmp = rd32le(USB2OTG_HOSTINTRMASK);
        tmp |= 1 << CHAN_CTRL;
        wr32le(USB2OTG_HOSTINTRMASK, tmp);

        /* Clear caches, actually CachePreDMA shall be used here, dunno if it is implemented on ARM already... */
        CacheClearE(&req->iouh_SetupData, 8, CACRF_ClearD);
        if (req->iouh_Data != NULL && req->iouh_Length != 0)
        {
            if (direction)
                CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_InvalidateD);
            else
                CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_ClearD);
        }

        /* SETUP phase of the transfer, always OUT type. Send CTRL data */
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE),
            USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
            USB2OTG_HOSTCHAR_EPDIR(0) |
            (do_split ? USB2OTG_HOSTCHAR_EC(3) : USB2OTG_HOSTCHAR_EC(1)) |
            USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_CTRL) |
            USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
            USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize) |
            low_speed
        );

        /* If split transaction requested then do it */
        if (do_split)
        {
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, SPLITCTRL),
                (1 << 31) |     /* Split enable */
                (3 << 14) |     /* Split position: 3 == ALL, 2 == Begin, 0 == Mid, 1 == END */
                ((req->iouh_SplitHubAddr & 0x7f) << 7) |
                ((req->iouh_SplitHubPort & 0x0f))
            );
        }
        else
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, SPLITCTRL), 0);

        /* Setup the size, PID, packet count and length */
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, TRANSSIZE),
            USB2OTG_HOSTTSIZE_PID(USB2OTG_PID_SETUP) |
            USB2OTG_HOSTTSIZE_PKTCNT(1) |
            USB2OTG_HOSTTSIZE_SIZE(8));
        /* Set the bus address of transferred data (use L2 uncached from AHB's point of view!) */
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, DMAADDR), 0xc0000000 | (ULONG)&req->iouh_SetupData);
        /* Finally enable the channel and thus start transaction */
        tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE));
        tmp |= USB2OTG_HOSTCHAR_ENABLE;
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE), tmp);

        /* Wait for completion, interrupt is masked */
        do {
            tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR));
        } while ((tmp & USB2OTG_INTRCHAN_HALT) == 0);

        /* If Split transaction was requested complete it now */
        if (do_split && (tmp == 0x22)) /* Channel halted and ACK */
        {
            uint32_t split = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, SPLITCTRL));
            D(bug("[USB2OTG] Completing split transaction. SPLITCTRL=%08x\n", split));
            split |= 1 << 16;   /* Set "do complete split" */
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, SPLITCTRL), split);

            /* Clear interrupt flags */
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR), 0x22);

            /* Enable channel again */
            tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE));
            tmp |= USB2OTG_HOSTCHAR_ENABLE;
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE), tmp);

            /* Wait for completion, interrupt is masked */
            do {
                tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR));
            } while ((tmp & USB2OTG_INTRCHAN_HALT) == 0);
        }

        /* Check for errors */
        if ((tmp & USB2OTG_INTRCHAN_TRANSFERCOMPLETE) == 0)
        {
            D(bug("[USB2OTG] %s: CTRL Channel closed but transfer failed. SETUP phase, INTR=%08x\n", __PRETTY_FUNCTION__, tmp));

            DumpChannelRegs(CHAN_CTRL);

            req->iouh_Actual = 0;
            req->iouh_Req.io_Error = UHIOERR_STALL;
            Disable();
            otg_Unit->hu_InProgressXFer[CHAN_CTRL] = NULL;
            Enable();
            FNAME_DEV(TermIO)(req, USB2OTGBase);
            return;
        }

        /* Clear interrupts */
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR), tmp);

        /* DATA phase if there is any data to transfer */
        if (req->iouh_Length && req->iouh_Data)
        {
            int pktcnt = (req->iouh_Length + req->iouh_MaxPktSize - 1) / req->iouh_MaxPktSize;

            if (pktcnt > 1)
                D(bug("!!! request for %d packets\n", pktcnt));

            if (!do_split)
            {
                /* Set up proper device address and direction */
                wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE),
                    USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
                    USB2OTG_HOSTCHAR_EPDIR(direction) |
                    USB2OTG_HOSTCHAR_EC(1) |
                    USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_CTRL) |
                    USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
                    USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize) |
                    low_speed
                );

                /* Buffer in L2 uncached AHB, it will auto advance */
                wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, DMAADDR), 0xc0000000 | (ULONG)req->iouh_Data);

                /* Transaction size starting with DATA1 PID */
                wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, TRANSSIZE),
                    USB2OTG_HOSTTSIZE_PID(USB2OTG_PID_DATA1) |
                    USB2OTG_HOSTTSIZE_PKTCNT(pktcnt) |
                    USB2OTG_HOSTTSIZE_SIZE(req->iouh_Length));

                /* Finally enable the channel and thus start transaction */
                tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE));
                tmp |= USB2OTG_HOSTCHAR_ENABLE;
                wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE), tmp);

                /* Wait for interrupt (masked) */
                do {
                    tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR));
                } while ((tmp & USB2OTG_INTRCHAN_HALT) == 0);

                if ((tmp & USB2OTG_INTRCHAN_TRANSFERCOMPLETE) == 0)
                {
                    D(bug("[USB2OTG] %s: CTRL Channel closed but transfer failed. Data phase, INTR=%08x\n", __PRETTY_FUNCTION__, tmp));

                    DumpChannelRegs(CHAN_CTRL);

                    req->iouh_Req.io_Error = UHIOERR_STALL;
                    Disable();
                    otg_Unit->hu_InProgressXFer[CHAN_CTRL] = NULL;
                    Enable();
                    FNAME_DEV(TermIO)(req, USB2OTGBase);
                    return;
                }
                else
                {
                    req->iouh_Actual += req->iouh_Length;
                }

                /* Clear interrupts */
                wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR), 0x7ff);
            }
            else
            {
                int len = req->iouh_Length;
                int toggle = USB2OTG_PID_DATA0;
                ULONG dma = (ULONG)req->iouh_Data;

                while (len)
                {
                    int xfer_len = len;
                    if (xfer_len > req->iouh_MaxPktSize)
                        xfer_len = req->iouh_MaxPktSize;

                    toggle ^= USB2OTG_PID_DATA1;

                    /* Set up proper device address and direction */
                    wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE),
                        USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
                        USB2OTG_HOSTCHAR_EPDIR(direction) |
                        (do_split ? USB2OTG_HOSTCHAR_EC(3) : USB2OTG_HOSTCHAR_EC(1)) |
                        USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_CTRL) |
                        USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
                        USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize) |
                        low_speed
                    );
                    /* If split transaction requested then do it */
                    if (do_split)
                    {
                        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, SPLITCTRL),
                            (1 << 31) |     /* Split enable */
                            (3 << 14) |     /* Split position: 3 == ALL, 2 == Begin, 0 == Mid, 1 == END */
                            ((req->iouh_SplitHubAddr & 0x7f) << 7) |
                            ((req->iouh_SplitHubPort & 0x0f))
                        );
                    }
                    else
                        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, SPLITCTRL), 0);

                    /* Buffer in L2 uncached AHB, it will auto advance */
                    wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, DMAADDR), 0xc0000000 | dma);

                    /* Transaction size starting with DATA1 PID */
                    wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, TRANSSIZE),
                        USB2OTG_HOSTTSIZE_PID(toggle) |
                        USB2OTG_HOSTTSIZE_PKTCNT(1) |
                        USB2OTG_HOSTTSIZE_SIZE(xfer_len));

                    if (pktcnt > 1)
                    {
                        D(bug("registers for Data step"));
                        DumpChannelRegs(CHAN_CTRL);
                    }

                    /* Finally enable the channel and thus start transaction */
                    tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE));
                    tmp |= USB2OTG_HOSTCHAR_ENABLE;
                    wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE), tmp);

                    /* Wait for interrupt (masked) */
                    do {
                        tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR));
                    } while ((tmp & USB2OTG_INTRCHAN_HALT) == 0);

                    /* If Split transaction was requested complete it now */
                    if (do_split && (tmp == 0x22)) /* Channel halted and ACK */
                    {
                        uint32_t split = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, SPLITCTRL));
                        D(bug("[USB2OTG] Completing split transaction. SPLITCTRL=%08x\n", split));
                        split |= 1 << 16;   /* Set "do complete split" */
                        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, SPLITCTRL), split);

                        /* Clear interrupt flags */
                        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR), 0x22);

                        /* Enable channel again */
                        tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE));
                        tmp |= USB2OTG_HOSTCHAR_ENABLE;
                        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE), tmp);

                        /* Wait for completion, interrupt is masked */
                        do {
                            tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR));
                        } while ((tmp & USB2OTG_INTRCHAN_HALT) == 0);
                    }

                    /* Check for errors again */
                    if ((tmp & USB2OTG_INTRCHAN_TRANSFERCOMPLETE) == 0)
                    {
                        D(bug("[USB2OTG] %s: CTRL Channel closed but transfer failed. Data phase, INTR=%08x\n", __PRETTY_FUNCTION__, tmp));

                        DumpChannelRegs(CHAN_CTRL);

                        req->iouh_Req.io_Error = UHIOERR_STALL;
                        Disable();
                        otg_Unit->hu_InProgressXFer[CHAN_CTRL] = NULL;
                        Enable();
                        FNAME_DEV(TermIO)(req, USB2OTGBase);
                        return;
                    }
                    else
                    {
                        req->iouh_Actual += xfer_len;
                    }

                    len -= xfer_len;
                    dma += xfer_len;

                    /* Clear interrupts */
                    wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR), 0x7ff);
                }
            }
        }

        /*
            Restore interrupts and prepare for final stage, allow HALT interrupt, the rest of bits
            will be checked in irq handler.
        */
        oldmask |= USB2OTG_INTRCHAN_HALT;
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTRMASK), oldmask);

        /* Enable interrupt for this channel */
        wr32le(USB2OTG_HOSTINTR, 1 << CHAN_CTRL);
        tmp = rd32le(USB2OTG_HOSTINTRMASK);
        tmp |= 1 << CHAN_CTRL;
        wr32le(USB2OTG_HOSTINTRMASK, tmp);

        /*
            Determine direction of zero size ACK packet. Direction is IN when packet without data
            or packet with OUT data was sent. Otherwise direction is OUT.
        */
        if (req->iouh_Length == 0 || direction == 0)
            direction = 1;
        else
            direction = 0;

        /* Set up proper device address and direction */
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE),
            USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
            USB2OTG_HOSTCHAR_EPDIR(direction) |
            (do_split ? USB2OTG_HOSTCHAR_EC(3) : USB2OTG_HOSTCHAR_EC(1)) |
            USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_CTRL) |
            USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
            USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize) |
            low_speed
        );
        /* If split transaction requested then do it */
        if (do_split)
        {
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, SPLITCTRL),
                (1 << 31) |     /* Split enable */
                (3 << 14) |     /* Split position: 3 == ALL, 2 == Begin, 0 == Mid, 1 == END */
                ((req->iouh_SplitHubAddr & 0x7f) << 7) |
                ((req->iouh_SplitHubPort & 0x0f))
            );
        }
        else
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, SPLITCTRL), 0);
        /* Transaction size points to a zero-size packet with PID of DATA1 */
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, TRANSSIZE),
            USB2OTG_HOSTTSIZE_PID(USB2OTG_PID_DATA1) |
            USB2OTG_HOSTTSIZE_PKTCNT(1) |
            USB2OTG_HOSTTSIZE_SIZE(0));

        /* Finally enable the channel, rest of the code is in IRQ handler */
        tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE));
        tmp |= USB2OTG_HOSTCHAR_ENABLE;
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE), tmp);
    }
    else
    {
        Enable();
    }
}

/*
    Schedule INT transfers, but only those from the Schedule list. The IntXferQueue is
    maintained by SOF interrupt handler
*/
void FNAME_DEV(ScheduleIntTDs)(struct USB2OTGUnit *otg_Unit)
{
    int chan = 0;
    uint32_t tmp = 0;
    int direction = 0;
    ULONG frnm = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;

    /* Check if any of INT channels is free */
    for (chan = CHAN_INT1; chan <= CHAN_INT3; chan++)
    {
        struct IOUsbHWReq *req = NULL;

        /* If channel is in use unlock Enable() and continue checking, otherwise stay in Disable() state for a while */
        Disable();
        if (otg_Unit->hu_InProgressXFer[chan] != 0)
        {
            Enable();
            continue;
        }

        /* First try to get head of the int schedule queue */
        req = (struct IOUsbHWReq *)REMHEAD(&otg_Unit->hu_IntXFerScheduled);

        if (req)
        {
            int low_speed = 0;

            /* Channel was free and there is request available. Assign the request to given channel now */
            otg_Unit->hu_InProgressXFer[chan] = req;
            Enable();

            ULONG last_handled = frnm;
            ULONG next_to_handle = (last_handled + req->iouh_Interval) & 0x7ff;
            req->iouh_DriverPrivate1 = (APTR)(last_handled);
            req->iouh_DriverPrivate2 = (APTR)(next_to_handle);

            if (req->iouh_Flags & UHFF_LOWSPEED)
            {
                //D(bug("\n!!!!!!!!! LOW SPEED !!!!!!!!!!!!\n"));
                low_speed = 1 << 17;
            }

            /* Clear or flush caches, depending on requested direction */
            if (req->iouh_Data != NULL && req->iouh_Length != 0)
            {
                if (req->iouh_Dir == UHDIR_IN)
                {
                    CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_InvalidateD);
                    direction = 1;
                }
                else
                {
                    CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_ClearD);
                    direction = 0;
                }
            }

            /* Clear interrupts */
            wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);

            /*
                Allow HALT interrupt, the rest of bits will be checked in irq handler.
            */
            tmp = rd32le(USB2OTG_CHANNEL_REG(chan, INTRMASK));
            tmp |= USB2OTG_INTRCHAN_HALT;
            wr32le(USB2OTG_CHANNEL_REG(chan, INTRMASK), tmp);

            /* Enable interrupt for this channel */
            wr32le(USB2OTG_HOSTINTR, 1 << chan);
            tmp = rd32le(USB2OTG_HOSTINTRMASK);
            tmp |= 1 << chan;
            wr32le(USB2OTG_HOSTINTRMASK, tmp);

            /* Set up proper device address and direction */
            wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE),
                USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
                USB2OTG_HOSTCHAR_EPDIR(direction) |
                ((req->iouh_Flags & UHFF_SPLITTRANS) ? USB2OTG_HOSTCHAR_EC(3) : USB2OTG_HOSTCHAR_EC(1)) |
                USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_INT) |
                USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
                USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize) |
                low_speed
            );

            /* If split transaction requested then do it */
            if (req->iouh_Flags & UHFF_SPLITTRANS)
            {
                wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL),
                    (1 << 31) |     /* Split enable */
                    (3 << 14) |     /* Split position: 3 == ALL, 2 == Begin, 0 == Mid, 1 == END */
                    ((req->iouh_SplitHubAddr & 0x7f) << 7) |
                    ((req->iouh_SplitHubPort & 0x0f))
                );
            }
            else
                wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), 0);

            /* Get the PID of transaction */
            int pid = 3 & (otg_Unit->hu_PIDBits[req->iouh_DevAddr] >> (2 * req->iouh_Endpoint));

            /* Set up transaction size */
            wr32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE),
                USB2OTG_HOSTTSIZE_PID(pid) |
                USB2OTG_HOSTTSIZE_PKTCNT(1) |
                USB2OTG_HOSTTSIZE_SIZE(req->iouh_Length));

            /* Buffer in L2 uncached AHB */
            wr32le(USB2OTG_CHANNEL_REG(chan, DMAADDR), 0xc0000000 | (ULONG)req->iouh_Data);

            /* Finally enable the channel and thus start transaction */
            tmp = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
            tmp |= USB2OTG_HOSTCHAR_ENABLE;
            wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), tmp);
        }
        else
        {
            /* No more requests in the queue, return */
            Enable();
            return;
        }
    }
}
