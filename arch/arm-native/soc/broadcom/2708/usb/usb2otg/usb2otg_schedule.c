/*
    Copyright � 2013-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "usb2otg_intern.h"

#if DEBUG
static void DumpChannelRegs(int channel)
{
    D(bug("CHARBASE=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, CHARBASE))));
    D(bug("SPLITCTRL=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, SPLITCTRL))));
    D(bug("INTR=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, INTR))));
    D(bug("INTRMASK=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, INTRMASK))));
    D(bug("TRANSSIZE=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, TRANSSIZE))));
    D(bug("DMAADR=%08x\n", rd32le(USB2OTG_CHANNEL_REG(channel, DMAADDR))));
}
#endif

/* Prepare Channel for transfer */
void FNAME_DEV(SetupChannel)(struct USB2OTGUnit *otg_Unit, int chan)
{
    struct IOUsbHWReq *req = NULL;
    uint8_t direction = 0;
    ULONG xfer_size = 0;
    ULONG pkt_count = 0;
    APTR buffer = NULL;
    ULONG reg = 0;
    int pid = 0;

    if (chan < 0 || chan > 7)
        return;

    req = otg_Unit->hu_Channel[chan].hc_Request;

    D(bug("[USB2OTG] %s(%p, %d)\n", __PRETTY_FUNCTION__, otg_Unit, chan));

    if (req == NULL)
        return;

    req->iouh_Actual = 0;

    /*
        In case of control transfer setup the buffers for request, flush necessary caches and
        determine correct direction
    */
    if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
    {
        /* Direction is out, setup packet goes always to the device */
        direction = 0;

        /* Buffer points to the setup data, 8 bytes */
        buffer = &req->iouh_SetupData;
        xfer_size = 8;

        /* Flush caches for setup and actual data */
        CacheClearE(buffer, xfer_size, CACRF_ClearD);
        if (req->iouh_Data != NULL && req->iouh_Length != 0)
        {
            /* Determine data direction from request type, direction is of no use here */
            if (req->iouh_SetupData.bmRequestType & URTF_IN) {
                CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_InvalidateD);
            }
            else
                CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_ClearD);
        }

        /* Set toggle bit to SETUP */
        otg_Unit->hu_PIDBits[req->iouh_DevAddr] &= ~(3 << (2 * req->iouh_Endpoint));
        otg_Unit->hu_PIDBits[req->iouh_DevAddr] |= (USB2OTG_PID_SETUP << (2 * req->iouh_Endpoint));

        /* Get pid */
        pid = (otg_Unit->hu_PIDBits[req->iouh_DevAddr] >> (2 * req->iouh_Endpoint)) & 3;
    }
    else
    {
        /* Data direction as requested */
        if (req->iouh_Dir == UHDIR_IN) direction = 1; else direction = 0;

        /* Buffer points to the actual data, maximal length */
        buffer = req->iouh_Data;
        xfer_size = req->iouh_Length;

        /* Flush caches */
        if (req->iouh_Data != NULL && req->iouh_Length != 0)
        {
            if (direction)
                CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_InvalidateD);
            else
                CacheClearE(req->iouh_Data, req->iouh_Length, CACRF_ClearD);
        }

        /* Get pid */
        pid = (otg_Unit->hu_PIDBits[req->iouh_DevAddr] >> (2 * req->iouh_Endpoint)) & 3;
    }

    /* Prepare HOSTCHAR register */
    reg = USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
          USB2OTG_HOSTCHAR_EPDIR(direction) |
          USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
          USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize);

    if (req->iouh_Flags & UHFF_LOWSPEED)
        reg |= USB2OTG_HOSTCHAR_LOWSPEED;

    switch(req->iouh_Req.io_Command)
    {
        case UHCMD_CONTROLXFER:
            reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_CTRL);
            break;
        case UHCMD_INTXFER:
            reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_INT);
            break;
        case UHCMD_BULKXFER:
            reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_BULK);
            break;
        case UHCMD_ISOXFER:
            reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_ISO);
            break;
        default:
            return;
    }

    /* If split transaction requested limit transfer size to max packet size or 188 bytes, whichever is less */
    if (req->iouh_Flags & UHFF_SPLITTRANS)
    {
        if (req->iouh_Req.io_Command == UHCMD_INTXFER ||
            req->iouh_Req.io_Command == UHCMD_ISOXFER)
            reg |= USB2OTG_HOSTCHAR_EC(3);
        else
            reg |= USB2OTG_HOSTCHAR_EC(1);

        wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL),
                (1 << 31) |     /* Split enable */
                (3 << 14) |     /* Split position: 3 == ALL, 2 == Begin, 0 == Mid, 1 == END */
                ((req->iouh_SplitHubAddr & 0x7f) << 7) |
                ((req->iouh_SplitHubPort & 0x0f)));

        if (xfer_size > req->iouh_MaxPktSize)
            xfer_size = req->iouh_MaxPktSize;

        /* 188 bytes is maximal payload in single split transaction */
        if (xfer_size > 188)
            xfer_size = 188;
    }
    else
    {
        reg |= USB2OTG_HOSTCHAR_EC(1);

        wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), 0);
    }

    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), reg);

    /* Get packet count and adjust this and the transfer size */
    if (xfer_size > (1 << (otg_Unit->hu_XferSizeWidth - 1)))
        xfer_size = 1 << (otg_Unit->hu_XferSizeWidth - 1);

    pkt_count = (xfer_size + req->iouh_MaxPktSize - 1) / req->iouh_MaxPktSize;
    if (pkt_count > ((1 << (otg_Unit->hu_PktSizeWidth)) - 1))
    {
        pkt_count = (1 << (otg_Unit->hu_PktSizeWidth)) - 1;
        xfer_size = pkt_count * req->iouh_MaxPktSize;
    }

    /* Setup the size, PID, packet count and length */
    wr32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE),
        USB2OTG_HOSTTSIZE_PID(pid) |
        USB2OTG_HOSTTSIZE_PKTCNT(pkt_count) |
        USB2OTG_HOSTTSIZE_SIZE(xfer_size));

    otg_Unit->hu_Channel[chan].hc_XferSize = xfer_size;

    /* Set the bus address of transferred data (use L2 uncached from AHB's point of view!) */
    wr32le(USB2OTG_CHANNEL_REG(chan, DMAADDR), 0xc0000000 | (ULONG)buffer);
}

/* Advance the status of channel, adjust iouh_Actual, PID etc */
int FNAME_DEV(AdvanceChannel)(struct USB2OTGUnit *otg_Unit, int chan)
{
    uint8_t direction = 0;
    ULONG xfer_size = 0;
    ULONG pkt_count = 0;
    APTR buffer = NULL;
    ULONG reg = 0;
    int channel_ready = 1;

    struct IOUsbHWReq *req = otg_Unit->hu_Channel[chan].hc_Request;

    D(bug("[USB2OTG] %s(%p, %d)\n", __PRETTY_FUNCTION__, otg_Unit, chan));

    /*
        Determine number of packets involved in last transfer. If it is even, toggle
        the PID (the OTG was toggling it itself)
    */
    int txsize = otg_Unit->hu_Channel[chan].hc_XferSize;
    int last_pid = (otg_Unit->hu_PIDBits[req->iouh_DevAddr] >> (2 * req->iouh_Endpoint)) & 3;
    int pid = 0;
    int pktcnt = (txsize + req->iouh_MaxPktSize - 1) / req->iouh_MaxPktSize;
    if (pktcnt & 1)
    {
        /* Toggle PID */
        otg_Unit->hu_PIDBits[req->iouh_DevAddr] ^= (USB2OTG_PID_DATA1 << (2 * req->iouh_Endpoint));
    }
    /* Update the transferred size unless it was control channel in setup phase */
    req->iouh_Actual += txsize;
    req->iouh_Req.io_Error = 0;

    /* If it was CTRL channel in setup phase reset PID and iouh_Actual */
    if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER)
    {
        if (req->iouh_SetupData.bmRequestType & URTF_IN) direction = 1; else direction = 0;

        if (last_pid == USB2OTG_PID_SETUP)
        {
            req->iouh_Actual = 0;
            otg_Unit->hu_PIDBits[req->iouh_DevAddr] &= ~(3 << (2 * req->iouh_Endpoint));
            otg_Unit->hu_PIDBits[req->iouh_DevAddr] |= (USB2OTG_PID_DATA1 << (2 * req->iouh_Endpoint));
        }
    }
    else
    {
        /* Data direction as requested */
        if (req->iouh_Dir == UHDIR_IN) direction = 1; else direction = 0;
    }

    D(bug("[USB2OTG] Last transfer completed. Chan=%d, last pid=%d, last xfer size=%d, %08x\n", chan, last_pid, txsize,
        rd32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE))));

    /* Get new PID for transfer */
    pid = (otg_Unit->hu_PIDBits[req->iouh_DevAddr] >> (2 * req->iouh_Endpoint)) & 3;

    /* Get buffer und xfer length */
    buffer = (APTR)((IPTR)req->iouh_Data + req->iouh_Actual);
    xfer_size = req->iouh_Length - req->iouh_Actual;

    /* If there is still anything to transmit, do it now */
    if (xfer_size != 0 && buffer != NULL)
    {
        channel_ready = 0;

        D(bug("[USB2OTG] Channel %d not ready yet\n", chan));
    }
    else
    {
        /*
            Nothing to transmit now. If this was CTRL message, issue ACK transfer. But only if last transfer was
            not empty one (which would indicate ACK issued)
        */
        if (req->iouh_Req.io_Command == UHCMD_CONTROLXFER && txsize != 0)
        {
            channel_ready = 0;
            buffer = NULL;
            xfer_size = 0;
            pkt_count = 1;

            pid = USB2OTG_PID_DATA1;

            /* Turn direction */
            if (req->iouh_Length == 0 || direction == 0)
                direction = 1;
            else
                direction = 0;

            D(bug("[USB2OTG] Channel %d not ready yet - ACK phase\n", chan));
        }
    }

    if (!channel_ready)
    {
        D(bug("[USB2OTG] Req %p on channel %d continuing with transfer: buf=%p len=%d, act=%d, pid=%d\n",
                                        req, chan, buffer, xfer_size, req->iouh_Actual, pid));

        /* Prepare HOSTCHAR register */
        reg = USB2OTG_HOSTCHAR_ADDR(req->iouh_DevAddr) |
            USB2OTG_HOSTCHAR_EPDIR(direction) |
            USB2OTG_HOSTCHAR_EPNO(req->iouh_Endpoint) |
            USB2OTG_HOSTCHAR_MAXPACKETSIZE(req->iouh_MaxPktSize);

        if (req->iouh_Flags & UHFF_LOWSPEED)
            reg |= USB2OTG_HOSTCHAR_LOWSPEED;

        switch(req->iouh_Req.io_Command)
        {
            case UHCMD_CONTROLXFER:
                reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_CTRL);
                break;
            case UHCMD_INTXFER:
                reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_INT);
                break;
            case UHCMD_BULKXFER:
                reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_BULK);
                break;
            case UHCMD_ISOXFER:
                reg |= USB2OTG_HOSTCHAR_EPTYPE(USB2OTG_TYPE_ISO);
                break;
            default:
                return 1;
        }

        /* If split transaction requested limit transfer size to max packet size or 188 bytes, whichever is less */
        if (req->iouh_Flags & UHFF_SPLITTRANS)
        {
            if (req->iouh_Req.io_Command == UHCMD_INTXFER ||
                req->iouh_Req.io_Command == UHCMD_ISOXFER)
                reg |= USB2OTG_HOSTCHAR_EC(3);
            else
                reg |= USB2OTG_HOSTCHAR_EC(1);

            wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL),
                    (1 << 31) |     /* Split enable */
                    (3 << 14) |     /* Split position: 3 == ALL, 2 == Begin, 0 == Mid, 1 == END */
                    ((req->iouh_SplitHubAddr & 0x7f) << 7) |
                    ((req->iouh_SplitHubPort & 0x0f)));

            if (xfer_size > req->iouh_MaxPktSize)
                xfer_size = req->iouh_MaxPktSize;

            /* 188 bytes is maximal payload in single split transaction */
            if (xfer_size > 188)
                xfer_size = 188;
        }
        else
        {
            reg |= USB2OTG_HOSTCHAR_EC(1);

            wr32le(USB2OTG_CHANNEL_REG(chan, SPLITCTRL), 0);
        }

        wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), reg);

        /* Get packet count and adjust this and the transfer size */
        if (xfer_size > (1 << (otg_Unit->hu_XferSizeWidth - 1)))
            xfer_size = 1 << (otg_Unit->hu_XferSizeWidth - 1);

        pkt_count = (xfer_size + req->iouh_MaxPktSize - 1) / req->iouh_MaxPktSize;

        if (pkt_count == 0)
            pkt_count = 1;

        if (pkt_count > ((1 << (otg_Unit->hu_PktSizeWidth)) - 1))
        {
            pkt_count = (1 << (otg_Unit->hu_PktSizeWidth)) - 1;
            xfer_size = pkt_count * req->iouh_MaxPktSize;
        }

        /* Setup the size, PID, packet count and length */
        wr32le(USB2OTG_CHANNEL_REG(chan, TRANSSIZE),
            USB2OTG_HOSTTSIZE_PID(pid) |
            USB2OTG_HOSTTSIZE_PKTCNT(pkt_count) |
            USB2OTG_HOSTTSIZE_SIZE(xfer_size));

        otg_Unit->hu_Channel[chan].hc_XferSize = xfer_size;

        /* Set the bus address of transferred data (use L2 uncached from AHB's point of view!) */
        wr32le(USB2OTG_CHANNEL_REG(chan, DMAADDR), 0xc0000000 | (ULONG)buffer);
    }

    return channel_ready;
}

/* Start Channel */
void FNAME_DEV(StartChannel)(struct USB2OTGUnit *otg_Unit, int chan, int quick)
{
    ULONG tmp = 0;

    if (quick == 0)
    {
        if (chan < 0 || chan > 7)
            return;

        /* Enable interrupts for this channel */
        tmp = rd32le(USB2OTG_CHANNEL_REG(chan, INTRMASK));
        tmp |= USB2OTG_INTRCHAN_HALT;
        wr32le(USB2OTG_CHANNEL_REG(chan, INTRMASK), tmp);
        wr32le(USB2OTG_CHANNEL_REG(chan, INTR), 0x7ff);

        /* Enable interrupt for this channel */
        wr32le(USB2OTG_HOSTINTR, 1 << chan);
        tmp = rd32le(USB2OTG_HOSTINTRMASK);
        tmp |= 1 << chan;
        wr32le(USB2OTG_HOSTINTRMASK, tmp);

        tmp = rd32le(USB2OTG_INTRMASK);
        tmp |= USB2OTG_INTRCORE_HOSTCHANNEL;
        wr32le(USB2OTG_INTRMASK, tmp);
    }

    /* Finally enable the channel and thus start transaction */
    tmp = rd32le(USB2OTG_CHANNEL_REG(chan, CHARBASE));
    tmp |= USB2OTG_HOSTCHAR_ENABLE;
    wr32le(USB2OTG_CHANNEL_REG(chan, CHARBASE), tmp);
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
    struct IOUsbHWReq *req = NULL;

    D(bug("[USB2OTG] %s(%p)\n", __PRETTY_FUNCTION__, otg_Unit));

    /* First try to get head of the transfer queue */
    Disable();
    req = (struct IOUsbHWReq *)REMHEAD(&otg_Unit->hu_CtrlXFerQueue);

    /* If there was any request in the queue, try to put it into InProgress slot */
    if (req)
    {
        /* Channel slot empty? If yes store the request there. Otherwise put it back to the queue */
        if (otg_Unit->hu_Channel[CHAN_CTRL].hc_Request == NULL) {
            otg_Unit->hu_Channel[CHAN_CTRL].hc_Request = req;
        } else {
            ADDHEAD(&otg_Unit, req);
            Enable();
            return;
        }
        Enable();

        FNAME_DEV(SetupChannel)(otg_Unit, CHAN_CTRL);

        FNAME_DEV(StartChannel)(otg_Unit, CHAN_CTRL, 0);
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

    ULONG frnm = (rd32le(USB2OTG_HOSTFRAMENO) & 0x3fff) >> 3;

    /* Check if any of INT channels is free */
    for (chan = CHAN_INT1; chan <= CHAN_INT3; chan++)
    {
        struct IOUsbHWReq *req = NULL;

        /* If channel is in use unlock Enable() and continue checking, otherwise stay in Disable() state for a while */
        Disable();
        if (otg_Unit->hu_Channel[chan].hc_Request != NULL)
        {
            Enable();
            continue;
        }

        /* First try to get head of the int schedule queue */
        req = (struct IOUsbHWReq *)REMHEAD(&otg_Unit->hu_IntXFerScheduled);

        if (req)
        {
            /* Channel was free and there is request available. Assign the request to given channel now */
            otg_Unit->hu_Channel[chan].hc_Request = req;
            Enable();

            ULONG last_handled = frnm;
            ULONG next_to_handle = (last_handled + req->iouh_Interval) & 0x7ff;
            req->iouh_DriverPrivate1 = (APTR)((last_handled << 16) | next_to_handle);

            FNAME_DEV(SetupChannel)(otg_Unit, chan);

            FNAME_DEV(StartChannel)(otg_Unit, chan, 0);
        }
        else
        {
            /* No more requests in the queue, return */
            Enable();
            return;
        }
    }
}
