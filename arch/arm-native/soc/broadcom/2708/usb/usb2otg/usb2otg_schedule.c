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

    D(bug("[USB2OTG] %s(%p)", __PRETTY_FUNCTION__, otg_Unit));

    /* First try to get head of the transfer queue */
    Disable();
    req = (struct IOUsbHWReq *)REMHEAD(&otg_Unit->hu_CtrlXFerQueue);

    /* If there was any request in the queue, try to put it into InProgress slot */
    if (req)
    {
        uint32_t oldmask = 0;
        uint32_t tmp = 0;
        uint8_t direction = 0;

        /* InProgress slot empty? If yes store the request there. Otherwise put it back to the queue */
        if (otg_Unit->hu_InProgressCtrlXFer == NULL) {
            otg_Unit->hu_InProgressCtrlXFer = req;
        } else {
            ADDHEAD(&otg_Unit, req);
            Enable();
            return;
        }
        Enable();

        /* At this point we have marked our request as active and it can be sent out to the device */

        /* Determine data direction */
        if (req->iouh_SetupData.bmRequestType & URTF_IN) direction = 1; else direction = 0;

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
            USB2OTG_HOSTCHAR_EC(1) |
            USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
            USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize)
        );

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

        /* Check for errors */
        if ((tmp & USB2OTG_INTRCHAN_TRANSFERCOMPLETE) == 0)
        {
            D(bug("[USB2OTG] %s: Channel closed but transfer failed. INTR=%08x\n", __PRETTY_FUNCTION__, tmp));

            DumpChannelRegs(CHAN_CTRL);

            req->iouh_Actual = 0;
            req->iouh_Req.io_Error = UHIOERR_STALL;
            Disable();
            otg_Unit->hu_InProgressCtrlXFer = NULL;
            Enable();
            FNAME_DEV(TermIO)(req, USB2OTGBase);
            return;
        }

        /* Clear interrupts */
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR), tmp);

        /* DATA phase if there is any data to transfer */
        if (req->iouh_Length && req->iouh_Data)
        {
            /* Set up proper device address and direction */
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE),
                USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
                USB2OTG_HOSTCHAR_EPDIR(direction) |
                USB2OTG_HOSTCHAR_EC(1) |
                USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
                USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize)
            );
            /* Transaction size starting with DATA1 PID */
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, TRANSSIZE),
                USB2OTG_HOSTTSIZE_PID(USB2OTG_PID_DATA1) |
                USB2OTG_HOSTTSIZE_PKTCNT(1) |
                USB2OTG_HOSTTSIZE_SIZE(req->iouh_Length));
            /* Buffer in L2 uncached AHB */
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, DMAADDR), 0xc0000000 | (ULONG)req->iouh_Data);
            /* Finally enable the channel and thus start transaction */
            tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE));
            tmp |= USB2OTG_HOSTCHAR_ENABLE;
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE), tmp);

            /* Wait for interrupt (masked) */
            do {
                tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR));
            } while ((tmp & USB2OTG_INTRCHAN_HALT) == 0);

            /* Check for errors again */
            if ((tmp & USB2OTG_INTRCHAN_TRANSFERCOMPLETE) == 0)
            {
                D(bug("[USB2OTG] %s: Channel closed but transfer failed. INTR=%08x\n", __PRETTY_FUNCTION__, tmp));

                DumpChannelRegs(CHAN_CTRL);

                req->iouh_Actual = 0;
                req->iouh_Req.io_Error = UHIOERR_STALL;
                Disable();
                otg_Unit->hu_InProgressCtrlXFer = NULL;
                Enable();
                FNAME_DEV(TermIO)(req, USB2OTGBase);
                return;
            }

            /* Clear interrupts */
            wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTR), tmp);
        }

        /* Restore interrupts and prepare for final stage */
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, INTRMASK), oldmask);

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
            USB2OTG_HOSTCHAR_EC(1) |
            USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
            USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize)
        );
        /* Transaction size points to a zero-size packet with PID of DATA1 */
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, TRANSSIZE),
            USB2OTG_HOSTTSIZE_PID(USB2OTG_PID_DATA1) |
            USB2OTG_HOSTTSIZE_PKTCNT(1) |
            USB2OTG_HOSTTSIZE_SIZE(req->iouh_Length));

        /* Finally enable the channel, rest of the code is in IRQ handler */
        tmp = rd32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE));
        tmp |= USB2OTG_HOSTCHAR_ENABLE;
        wr32le(USB2OTG_CHANNEL_REG(CHAN_CTRL, CHARBASE), tmp);
    }
}
