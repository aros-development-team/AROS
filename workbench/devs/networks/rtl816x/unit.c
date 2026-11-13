/*
 * $Id$
 */

/*
        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful, but
        WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
        General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 59 Temple Place - Suite 330, Boston,
        MA 02111-1307, USA.
*/

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>

#include <aros/io.h>
#include <aros/macros.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/newstyle.h>
#include <devices/timer.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/timer.h>
#include <proto/utility.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rtl816x.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/*
 * Report incoming events to all hypothetical event receivers
 */
VOID ReportEvents(struct RTL816XBase *RTL816XDeviceBase, struct RTL816XUnit *unit, ULONG events)
{
    struct IOSana2Req *request, *tail, *next_request;
    struct List *list;

    list = &unit->rtl816xu_request_ports[EVENT_QUEUE]->mp_MsgList;

    Disable();
    next_request = (APTR)list->lh_Head;
    tail = (APTR)&list->lh_Tail;

    while (next_request != tail)
    {
        request = next_request;
        next_request = (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

        if ((request->ios2_WireError & events) != 0)
        {
            request->ios2_WireError = events;
            Remove((APTR)request);
            ReplyMsg((APTR)request);
        }
    }
    Enable();
}

struct TypeStats *FindTypeStats(struct RTL816XBase *RTL816XDeviceBase, struct RTL816XUnit *unit,
                                struct MinList *list, ULONG packet_type)
{
    struct TypeStats *stats, *tail;
    BOOL found = FALSE;

    stats = (APTR)list->mlh_Head;
    tail = (APTR)&list->mlh_Tail;

    while (stats != tail && !found)
    {
        if (stats->packet_type == packet_type)
            found = TRUE;
        else
            stats = (APTR)stats->node.mln_Succ;
    }

    if (!found)
        stats = NULL;

    return stats;
}

void FlushUnit(LIBBASETYPEPTR LIBBASE, struct RTL816XUnit *unit, UBYTE last_queue, BYTE error)
{
    struct IORequest *request;
    UBYTE i;
    struct Opener *opener, *tail;

    RTLD(bug("[%s] unit.FlushUnit\n", unit->rtl816xu_name))

    /* Abort queued operations */
    for (i = 0; i <= last_queue; i++)
    {
        while ((request = (APTR)GetMsg(unit->rtl816xu_request_ports[i])) != NULL)
        {
            request->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *)request);
        }
    }

    opener = (APTR)unit->rtl816xu_Openers.mlh_Head;
    tail = (APTR)unit->rtl816xu_Openers.mlh_Tail;

    /* Flush every opener's read queue */
    while (opener != tail)
    {
        while ((request = (APTR)GetMsg(&opener->read_port)) != NULL)
        {
            request->io_Error = error;
            ReplyMsg((struct Message *)request);
        }
        opener = (struct Opener *)opener->node.mln_Succ;
    }
}

/* Interrupt Rx support function - drains newly arrived packets from the
 * Rx descriptor ring. Every descriptor is handed back to the NIC once
 * looked at, whether the frame was wanted or not.
 */
VOID RTL816X_Rx_Process(struct RTL816XUnit *unit)
{
    struct RTL816XBase *RTL816XDeviceBase = unit->rtl816xu_device;
    struct rtl816x_priv *np = unit->rtl816xu_priv;

    struct TypeStats *tracker;
    UWORD packet_type;
    struct Opener *opener, *opener_tail;
    struct IOSana2Req *request, *request_tail;
    BOOL accepted, is_orphan;
    ULONG processed;

    RTLD(bug("[%s] RTL816X_Rx_Process()\n", unit->rtl816xu_name))

    for (processed = 0; processed < NUM_RX_DESC; processed++)
    {
        ULONG cur_rx = np->cur_rx;
        struct RxDesc *desc = np->RxDescArray + cur_rx;
        ULONG rx_status;
        int rx_size;
        struct eth_frame *frame;

        rx_status = AROS_LE2LONG(desc->opts1);

        if (rx_status & DescOwn)
            break;

        rx_size = (int)(rx_status & 0x00003fff) - ETH_CRCSIZE;

        if (rx_status & RxRES)
        {
            RTLD(bug("[%s] RTL816X_Rx_Process: frame error, status %08x\n",
                     unit->rtl816xu_name, rx_status))
            unit->rtl816xu_stats.BadData++;
        }
        else if (((rx_status & (FirstFrag | LastFrag)) != (FirstFrag | LastFrag)) ||
                 (rx_size <= 0) || (rx_size > (int)np->rx_buf_sz))
        {
            RTLD(bug("[%s] RTL816X_Rx_Process: fragmented/oversize frame dropped, status %08x\n",
                     unit->rtl816xu_name, rx_status))
            unit->rtl816xu_stats.Overruns++;
        }
        else
        {
            frame = (APTR)(IPTR)AROS_LE2QUAD(desc->addr);

            RTLD(bug("[%s] RTL816X_Rx_Process: frame @ %p, len=%d, index=%d\n",
                     unit->rtl816xu_name, frame, rx_size, cur_rx))

            is_orphan = TRUE;

            if (AddressFilter(LIBBASE, unit, frame->eth_packet_dest))
            {
                packet_type = AROS_BE2WORD(frame->eth_packet_type);

                opener = (APTR)unit->rtl816xu_Openers.mlh_Head;
                opener_tail = (APTR)&unit->rtl816xu_Openers.mlh_Tail;

                /* Offer packet to every opener */
                while (opener != opener_tail)
                {
                    request = (APTR)opener->read_port.mp_MsgList.lh_Head;
                    request_tail = (APTR)&opener->read_port.mp_MsgList.lh_Tail;
                    accepted = FALSE;

                    /* Offer packet to each request until it's accepted */
                    while ((request != request_tail) && !accepted)
                    {
                        if ((request->ios2_PacketType == packet_type)
                            || ((request->ios2_PacketType <= ETH_MTU)
                            && (packet_type <= ETH_MTU)))
                        {
                            CopyPacket(LIBBASE, unit, request, rx_size, packet_type, frame);
                            accepted = TRUE;
                        }
                        request = (struct IOSana2Req *)request->ios2_Req.io_Message.mn_Node.ln_Succ;
                    }

                    if (accepted)
                        is_orphan = FALSE;

                    opener = (APTR)opener->node.mln_Succ;
                }

                /* If packet was unwanted, give it to S2_READORPHAN request */
                if (is_orphan)
                {
                    unit->rtl816xu_stats.UnknownTypesReceived++;

                    if (!IsMsgPortEmpty(unit->rtl816xu_request_ports[ADOPT_QUEUE]))
                    {
                        CopyPacket(LIBBASE, unit,
                                   (APTR)unit->rtl816xu_request_ports[ADOPT_QUEUE]->mp_MsgList.lh_Head,
                                   rx_size, packet_type, frame);
                    }
                }

                tracker = FindTypeStats(LIBBASE, unit, &unit->rtl816xu_type_trackers, packet_type);

                if (tracker != NULL)
                {
                    tracker->stats.PacketsReceived++;
                    tracker->stats.BytesReceived += rx_size;
                }
            }
            unit->rtl816xu_stats.PacketsReceived++;
        }

        /* Hand the descriptor back to the NIC */
        desc->opts2 = 0;
        desc->opts1 = AROS_LONG2LE(DescOwn | (ULONG)np->rx_buf_sz |
                                   ((cur_rx == (NUM_RX_DESC - 1)) ? RingEnd : 0));

        np->cur_rx = (cur_rx + 1) % NUM_RX_DESC;
    }
}

/* Reclaim transmitted descriptors and restart the send queue if it was
 * stopped on a full ring.
 */
VOID RTL816X_Tx_Cleanup(struct RTL816XUnit *unit)
{
    struct rtl816x_priv *np = unit->rtl816xu_priv;
    ULONG dirty_tx = np->dirty_tx;

    while (dirty_tx != np->cur_tx)
    {
        unsigned int entry = dirty_tx % NUM_TX_DESC;

        if (AROS_LE2LONG(np->TxDescArray[entry].opts1) & DescOwn)
            break;

        dirty_tx++;
    }

    if (np->dirty_tx != dirty_tx)
    {
        np->dirty_tx = dirty_tx;

        if (netif_queue_stopped(unit) &&
            ((np->cur_tx - dirty_tx) < (NUM_TX_DESC - 4)))
        {
            netif_wake_queue(unit);
        }
    }
}

/*
 * Softint fired by Cause() to push queued write requests into the NIC
 */
static AROS_INTH1(RTL816X_TX_IntF, struct RTL816XUnit *, unit)
{
    AROS_INTFUNC_INIT

    struct rtl816x_priv *np = unit->rtl816xu_priv;
    struct RTL816XBase *RTL816XDeviceBase = unit->rtl816xu_device;
    APTR base = unit->rtl816xu_BaseMem;
    struct MsgPort *port;
    BOOL stopped = netif_queue_stopped(unit) ? TRUE : FALSE;

    RTLD(bug("[%s] RTL816X_TX_IntF()\n", unit->rtl816xu_name))

    port = unit->rtl816xu_request_ports[WRITE_QUEUE];

    if (!stopped && (np->TxDescArray != NULL))
    {
        while (!IsMsgPortEmpty(port))
        {
            unsigned int nr = np->cur_tx % NUM_TX_DESC;
            struct IOSana2Req *request;
            struct Opener *opener;
            struct eth_frame *frame;
            UBYTE *buffer;
            UWORD packet_size, data_size;
            ULONG wire_error = 0;
            BYTE error = 0;

            if (((np->cur_tx - np->dirty_tx) >= NUM_TX_DESC) ||
                (AROS_LE2LONG(np->TxDescArray[nr].opts1) & DescOwn))
            {
                /* Ring full - wait for RTL816X_Tx_Cleanup() to wake us */
                RTLD(bug("[%s] RTL816X_TX_IntF: output ring full, stopping queue\n", unit->rtl816xu_name))
                netif_stop_queue(unit);
                stopped = TRUE;
                break;
            }

            request = (APTR)port->mp_MsgList.lh_Head;
            data_size = packet_size = request->ios2_DataLength;

            opener = (APTR)request->ios2_BufferManagement;

            frame = (APTR)(IPTR)AROS_LE2QUAD(np->TxDescArray[nr].addr);

            if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
                packet_size += ETH_HEADERSIZE;

            if (packet_size > TX_BUF_SIZE)
            {
                error = S2ERR_MTU_EXCEEDED;
                wire_error = S2WERR_GENERIC_ERROR;
            }
            else
            {
                if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
                {
                    CopyMem(request->ios2_DstAddr, frame->eth_packet_dest, ETH_ADDRESSSIZE);
                    CopyMem(unit->rtl816xu_dev_addr, frame->eth_packet_source, ETH_ADDRESSSIZE);
                    frame->eth_packet_type = AROS_WORD2BE(request->ios2_PacketType);

                    buffer = frame->eth_packet_data;
                }
                else
                {
                    buffer = (UBYTE *)frame;
                }

                if (!opener->tx_function(buffer, request->ios2_Data, data_size))
                {
                    error = S2ERR_NO_RESOURCES;
                    wire_error = S2WERR_BUFF_ERROR;
                    ReportEvents(LIBBASE, unit,
                                 S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_TX);
                }
            }

            if (error == 0)
            {
                struct TypeStats *tracker;

                /* Pad short frames - the buffer is always large enough */
                if (packet_size < ETH_ZLEN)
                {
                    memset(((UBYTE *)frame) + packet_size, 0, ETH_ZLEN - packet_size);
                    packet_size = ETH_ZLEN;
                }

                RTLD(bug("[%s] RTL816X_TX_IntF: packet %d @ %p (%d bytes) queued for transmission\n",
                         unit->rtl816xu_name, nr, frame, packet_size))

                np->TxDescArray[nr].opts2 = 0;
                np->TxDescArray[nr].opts1 =
                    AROS_LONG2LE(DescOwn | FirstFrag | LastFrag | packet_size |
                                 ((nr == (NUM_TX_DESC - 1)) ? RingEnd : 0));

                RTL_W8(base + TxPoll, NPQ);     /* set polling bit */

                np->cur_tx++;

                unit->rtl816xu_stats.PacketsSent++;

                tracker = FindTypeStats(LIBBASE, unit, &unit->rtl816xu_type_trackers,
                                        request->ios2_PacketType);
                if (tracker != NULL)
                {
                    tracker->stats.PacketsSent++;
                    tracker->stats.BytesSent += packet_size;
                }
            }

            /* Reply the request whether it was sent or failed */
            request->ios2_Req.io_Error = error;
            request->ios2_WireError = wire_error;
            Disable();
            Remove((APTR)request);
            Enable();
            ReplyMsg((APTR)request);
        }
    }

    /* While stopped, incoming writes queue silently; the queue is
       re-armed by netif_wake_queue() */
    if (stopped)
        port->mp_Flags = PA_IGNORE;
    else
        port->mp_Flags = PA_SOFTINT;

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * Interrupt handler called whenever the NIC raises its interrupt.
 */
static AROS_INTH1(RTL816X_IntHandlerF, struct RTL816XUnit *, unit)
{
    AROS_INTFUNC_INIT

    struct rtl816x_priv *np = unit->rtl816xu_priv;
    APTR base = unit->rtl816xu_BaseMem;
    UWORD status;
    int boguscnt = unit->rtl816xu_device->rtl816xb_MaxIntWork;

    RTL_W16(base + (IntrMask), 0x0000);

    do
    {
        status = RTL_R16(base + (IntrStatus));

        /* hotplug/major error/no more work/shared irq */
        if ((status == 0xFFFF) || !status)
            break;

        /* Acknowledge only what was actually read */
        RTL_W16(base + (IntrStatus), status);

        if (!(status & np->intr_event))
            break;

        RTLD(bug("[%s] RTL816X_IntHandlerF: status %04x\n", unit->rtl816xu_name, status))

        if (status & SYSErr)
        {
            RTLD(bug("[%s] RTL816X_IntHandlerF: PCI system error!\n", unit->rtl816xu_name))
            break;
        }

        if (status & LinkChg)
            rtl816x_CheckLinkStatus(unit);

        if (status & (RxOK | RxErr | RxOverflow | RxFIFOOver))
            RTL816X_Rx_Process(unit);

        if (status & (TxOK | TxErr | TxDescUnavail))
        {
            RTL816X_Tx_Cleanup(unit);

            if (status & TxDescUnavail)
                RTL_W8(base + (TxPoll), NPQ);
        }

        boguscnt--;
    } while (boguscnt > 0);

    if (boguscnt <= 0)
    {
        RTLD(bug("[%s] RTL816X_IntHandlerF: Too much work at interrupt!\n", unit->rtl816xu_name))
        RTL_W16(base + (IntrStatus), 0xffff);
    }

    RTL_W16(base + (IntrMask), np->intr_event);

    return FALSE;

    AROS_INTFUNC_EXIT
}

VOID CopyPacket(struct RTL816XBase *RTL816XDeviceBase, struct RTL816XUnit *unit,
                struct IOSana2Req *request, UWORD packet_size, UWORD packet_type,
                struct eth_frame *buffer)
{
    struct Opener *opener;
    BOOL filtered = FALSE;
    UBYTE *ptr;
    const UBYTE broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    RTLD(bug("[%s] CopyPacket(packet @ %p, len = %d)\n", unit->rtl816xu_name, buffer, packet_size))

    /* Set multicast and broadcast flags */

    request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
    if (memcmp(buffer->eth_packet_dest, broadcast, 6) == 0)
    {
        request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
    }
    else if ((buffer->eth_packet_dest[0] & 0x1) != 0)
    {
        request->ios2_Req.io_Flags |= SANA2IOF_MCAST;
    }

    /* Set source and destination addresses and packet type */
    CopyMem(buffer->eth_packet_source, request->ios2_SrcAddr, ETH_ADDRESSSIZE);
    CopyMem(buffer->eth_packet_dest, request->ios2_DstAddr, ETH_ADDRESSSIZE);
    request->ios2_PacketType = packet_type;

    /* Adjust for cooked packet request */

    if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
    {
        packet_size -= ETH_PACKET_DATA;
        ptr = (UBYTE *)&buffer->eth_packet_data[0];
    }
    else
    {
        ptr = (UBYTE *)buffer;
    }

    request->ios2_DataLength = packet_size;

    /* Filter packet */

    opener = request->ios2_BufferManagement;
    if ((request->ios2_Req.io_Command == CMD_READ) &&
        (opener->filter_hook != NULL))
    {
        if (!CallHookPkt(opener->filter_hook, request, ptr))
        {
            RTLD(bug("[%s] CopyPacket: packet filtered\n", unit->rtl816xu_name))
            filtered = TRUE;
        }
    }

    if (!filtered)
    {
        /* Copy packet into opener's buffer and reply packet */
        if (!opener->rx_function(request->ios2_Data, ptr, packet_size))
        {
            request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
            request->ios2_WireError = S2WERR_BUFF_ERROR;
            ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_RX);
        }
        Disable();
        Remove((APTR)request);
        Enable();
        ReplyMsg((APTR)request);
    }
}

BOOL AddressFilter(struct RTL816XBase *RTL816XDeviceBase, struct RTL816XUnit *unit, UBYTE *address)
{
    struct AddressRange *range, *tail;
    BOOL accept = TRUE;
    ULONG address_left;
    UWORD address_right;

    /* Check whether address is unicast/broadcast or multicast */

    address_left = AROS_BE2LONG(*((ULONG *)address));
    address_right = AROS_BE2WORD(*((UWORD *)(address + 4)));

    if ((address_left & 0x01000000) != 0 &&
        !(address_left == 0xffffffff && address_right == 0xffff))
    {
        /* Check if this multicast address is wanted */

        range = (APTR)unit->rtl816xu_multicast_ranges.mlh_Head;
        tail = (APTR)&unit->rtl816xu_multicast_ranges.mlh_Tail;
        accept = FALSE;

        while ((range != tail) && !accept)
        {
            if ((address_left > range->lower_bound_left ||
                (address_left == range->lower_bound_left &&
                address_right >= range->lower_bound_right)) &&
                (address_left < range->upper_bound_left ||
                (address_left == range->upper_bound_left &&
                address_right <= range->upper_bound_right)))
                accept = TRUE;
            range = (APTR)range->node.mln_Succ;
        }

        if (!accept)
            unit->rtl816xu_special_stats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    }
    return accept;
}

/*
 * Unit process
 */
AROS_UFH3(void, RTL816X_Schedular,
        AROS_UFHA(STRPTR,              argPtr, A0),
        AROS_UFHA(ULONG,               argSize, D0),
        AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Task *taskSelf = FindTask(NULL);
    struct RTL816XStartup *sm_UD = taskSelf->tc_UserData;
    struct RTL816XUnit *unit = sm_UD->rtl816xsm_Unit;
    struct Task *parent = sm_UD->rtl816xsm_Parent;

    LIBBASETYPEPTR LIBBASE = unit->rtl816xu_device;
    struct MsgPort *input = NULL;
    BOOL timer_open = FALSE, setupOK = FALSE;
    ULONG sigset;

    RTLD(bug("[%s] RTL816X_Schedular: setting up device '%s'\n", taskSelf->tc_Node.ln_Name, unit->rtl816xu_name))

    unit->rtl816xu_signal_0 = -1;
    unit->rtl816xu_signal_1 = -1;
    unit->rtl816xu_signal_2 = -1;
    unit->rtl816xu_signal_3 = -1;

    if ((input = CreateMsgPort()) != NULL)
    {
        if ((unit->rtl816xu_TimerSlowPort = CreateMsgPort()) != NULL)
        {
            unit->rtl816xu_TimerSlowReq = (struct timerequest *)
                CreateIORequest((struct MsgPort *)unit->rtl816xu_TimerSlowPort,
                                sizeof(struct timerequest));

            if (unit->rtl816xu_TimerSlowReq != NULL)
            {
                if (!OpenDevice("timer.device", UNIT_MICROHZ,
                    (struct IORequest *)unit->rtl816xu_TimerSlowReq, 0))
                {
                    timer_open = TRUE;

                    unit->rtl816xu_signal_0 = AllocSignal(-1);
                    unit->rtl816xu_signal_1 = AllocSignal(-1);
                    unit->rtl816xu_signal_2 = AllocSignal(-1);
                    unit->rtl816xu_signal_3 = AllocSignal(-1);

                    if (unit->rtl816xu_signal_0 != -1)
                    {
                        unit->rtl816xu_input_port = input;

                        unit->initialize(unit);

                        setupOK = TRUE;
                    }
                }
            }
        }
    }

    if (!setupOK)
    {
        RTLD(bug("[%s] RTL816X_Schedular: setup failed\n", taskSelf->tc_Node.ln_Name))

        if (unit->rtl816xu_signal_0 != -1) FreeSignal(unit->rtl816xu_signal_0);
        if (unit->rtl816xu_signal_1 != -1) FreeSignal(unit->rtl816xu_signal_1);
        if (unit->rtl816xu_signal_2 != -1) FreeSignal(unit->rtl816xu_signal_2);
        if (unit->rtl816xu_signal_3 != -1) FreeSignal(unit->rtl816xu_signal_3);
        if (timer_open)
            CloseDevice((struct IORequest *)unit->rtl816xu_TimerSlowReq);
        if (unit->rtl816xu_TimerSlowReq != NULL)
        {
            DeleteIORequest((struct IORequest *)unit->rtl816xu_TimerSlowReq);
            unit->rtl816xu_TimerSlowReq = NULL;
        }
        if (unit->rtl816xu_TimerSlowPort != NULL)
        {
            DeleteMsgPort(unit->rtl816xu_TimerSlowPort);
            unit->rtl816xu_TimerSlowPort = NULL;
        }
        if (input != NULL)
            DeleteMsgPort(input);
        unit->rtl816xu_input_port = NULL;

        /* Report failure. sm_UD and unit belong to the parent and must
           not be touched after the signal. */
        sm_UD->rtl816xsm_Unit = NULL;
        Signal(parent, SIGF_SINGLE);
        return;
    }

    /* Report success. sm_UD is freed by the parent as soon as it is
       signalled, so it must not be touched afterwards. */
    Signal(parent, SIGF_SINGLE);

    RTLD(bug("[%s] RTL816X_Schedular: entering forever loop\n", taskSelf->tc_Node.ln_Name))

    sigset = 1 << input->mp_SigBit |
             1 << unit->rtl816xu_signal_0 |
             1 << unit->rtl816xu_signal_1 |
             1 << unit->rtl816xu_signal_2 |
             1 << unit->rtl816xu_signal_3;
    for (;;)
    {
        ULONG recvd = Wait(sigset);
        if (recvd & (1 << unit->rtl816xu_signal_0))
        {
            /* Shutdown process. The driver has stopped the hardware
               already and waits for this process to exit. */
            CloseDevice((struct IORequest *)unit->rtl816xu_TimerSlowReq);
            DeleteIORequest((struct IORequest *)unit->rtl816xu_TimerSlowReq);
            DeleteMsgPort(unit->rtl816xu_TimerSlowPort);
            DeleteMsgPort(input);
            FreeSignal(unit->rtl816xu_signal_0);
            if (unit->rtl816xu_signal_1 != -1) FreeSignal(unit->rtl816xu_signal_1);
            if (unit->rtl816xu_signal_2 != -1) FreeSignal(unit->rtl816xu_signal_2);
            if (unit->rtl816xu_signal_3 != -1) FreeSignal(unit->rtl816xu_signal_3);

            RTLD(bug("[%s] RTL816X_Schedular: process shutdown.\n", taskSelf->tc_Node.ln_Name))
            return;
        }
        else if (recvd & (1 << input->mp_SigBit))
        {
            struct IOSana2Req *io;

            /* Handle incoming transactions */
            while ((io = (struct IOSana2Req *)GetMsg(input)) != NULL)
            {
                ObtainSemaphore(&unit->rtl816xu_unit_lock);
                handle_request(LIBBASE, io);
            }
        }
    }

    AROS_USERFUNC_EXIT
}

static struct AddressRange *FindMulticastRange(LIBBASETYPEPTR LIBBASE, struct RTL816XUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left, UWORD upper_bound_right)
{
    struct AddressRange *range, *tail;
    BOOL found = FALSE;

    range = (APTR)unit->rtl816xu_multicast_ranges.mlh_Head;
    tail = (APTR)&unit->rtl816xu_multicast_ranges.mlh_Tail;

    while ((range != tail) && !found)
    {
        if ((lower_bound_left == range->lower_bound_left) &&
            (lower_bound_right == range->lower_bound_right) &&
            (upper_bound_left == range->upper_bound_left) &&
            (upper_bound_right == range->upper_bound_right))
            found = TRUE;
        else
            range = (APTR)range->node.mln_Succ;
    }

    if (!found)
        range = NULL;

    return range;
}

BOOL AddMulticastRange(LIBBASETYPEPTR LIBBASE, struct RTL816XUnit *unit, const UBYTE *lower_bound,
   const UBYTE *upper_bound)
{
    struct AddressRange *range;
    ULONG lower_bound_left, upper_bound_left;
    UWORD lower_bound_right, upper_bound_right;

    lower_bound_left = AROS_BE2LONG(*((ULONG *)lower_bound));
    lower_bound_right = AROS_BE2WORD(*((UWORD *)(lower_bound + 4)));
    upper_bound_left = AROS_BE2LONG(*((ULONG *)upper_bound));
    upper_bound_right = AROS_BE2WORD(*((UWORD *)(upper_bound + 4)));

    range = FindMulticastRange(LIBBASE, unit, lower_bound_left, lower_bound_right,
                               upper_bound_left, upper_bound_right);

    if (range != NULL)
        range->add_count++;
    else
    {
        range = AllocMem(sizeof(struct AddressRange), MEMF_PUBLIC);
        if (range != NULL)
        {
            range->lower_bound_left = lower_bound_left;
            range->lower_bound_right = lower_bound_right;
            range->upper_bound_left = upper_bound_left;
            range->upper_bound_right = upper_bound_right;
            range->add_count = 1;

            Disable();
            AddTail((APTR)&unit->rtl816xu_multicast_ranges, (APTR)range);
            Enable();

            if (unit->rtl816xu_range_count++ == 0)
            {
                unit->rtl816xu_flags |= IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }

    return range != NULL;
}

BOOL RemMulticastRange(LIBBASETYPEPTR LIBBASE, struct RTL816XUnit *unit,
                       const UBYTE *lower_bound, const UBYTE *upper_bound)
{
    struct AddressRange *range;
    ULONG lower_bound_left, upper_bound_left;
    UWORD lower_bound_right, upper_bound_right;

    lower_bound_left = AROS_BE2LONG(*((ULONG *)lower_bound));
    lower_bound_right = AROS_BE2WORD(*((UWORD *)(lower_bound + 4)));
    upper_bound_left = AROS_BE2LONG(*((ULONG *)upper_bound));
    upper_bound_right = AROS_BE2WORD(*((UWORD *)(upper_bound + 4)));

    range = FindMulticastRange(LIBBASE, unit, lower_bound_left, lower_bound_right,
                               upper_bound_left, upper_bound_right);

    if (range != NULL)
    {
        if (--range->add_count == 0)
        {
            Disable();
            Remove((APTR)range);
            Enable();
            FreeMem(range, sizeof(struct AddressRange));

            if (--unit->rtl816xu_range_count == 0)
            {
                unit->rtl816xu_flags &= ~IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }
    return range != NULL;
}

/*
 * Create new RTL816X ethernet device unit
 */
struct RTL816XUnit *CreateUnit(struct RTL816XBase *RTL816XDeviceBase, OOP_Object *pciDevice)
{
    struct RTL816XUnit *unit;
    BOOL success = TRUE;
    IPTR VendorId, ProductId;
    int i;

#if defined(RTL_DEBUG)
    BOOL doDebug = TRUE;
#else
    BOOL doDebug = FALSE;
#endif

    if ((unit = AllocMem(sizeof(struct RTL816XUnit), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
    {
        IPTR mmiobase = 0, mmiolen = 0, type = 0;
        OOP_Object *driver;
        BOOL mmioerror = FALSE;

        if (doDebug)
            unit->rtl816xu_flags |= IFF_DEBUG;

        RTLD(bug("[rtl816x] CreateUnit()\n"))

        unit->rtl816xu_UnitNum = RTL816XDeviceBase->rtl816xb_UnitCount++;

        unit->rtl816xu_Sana2Info.HardwareType = S2WireType_Ethernet;
        unit->rtl816xu_Sana2Info.MTU = ETH_MTU;
        unit->rtl816xu_Sana2Info.AddrFieldSize = 8 * ETH_ADDRESSSIZE;

        /* Determine which configuration to use for this card */
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorId);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductId);

        unit->rtl816xu_config = UNKNOWN_CFG;

        for (i = 0; cards[i].vendorID != 0; i++)
        {
            if ((cards[i].vendorID == VendorId) &&
                (cards[i].productID == ProductId))
            {
                unit->rtl816xu_config = cards[i].config;
                break;
            }
        }

        if (unit->rtl816xu_config == UNKNOWN_CFG)
        {
            RTLD(bug("[rtl816x] CreateUnit: %04x:%04x is not a supported device\n",
                     (unsigned int)VendorId, (unsigned int)ProductId))
            RTL816XDeviceBase->rtl816xb_UnitCount--;
            FreeMem(unit, sizeof(struct RTL816XUnit));
            return NULL;
        }

        switch (unit->rtl816xu_config)
        {
        case RTL_CFG_0:
            unit->rtl816xu_intr_event = SYSErr | LinkChg | RxOverflow |
                                        RxFIFOOver | TxErr | TxOK | RxOK | RxErr;
            break;
        case RTL_CFG_2:
            unit->rtl816xu_intr_event = SYSErr | LinkChg | RxOverflow | PCSTimeout |
                                        RxFIFOOver | TxErr | TxOK | RxOK | RxErr;
            break;
        default:
            unit->rtl816xu_intr_event = SYSErr | LinkChg | RxOverflow |
                                        TxErr | TxOK | RxOK | RxErr;
            break;
        }

        if ((unit->rtl816xu_name = AllocVec(8 + (unit->rtl816xu_UnitNum / 10) + 2,
                                            MEMF_CLEAR | MEMF_PUBLIC)) == NULL)
        {
            FreeMem(unit, sizeof(struct RTL816XUnit));
            return NULL;
        }
        sprintf((char *)unit->rtl816xu_name, "rtl816x.%d", (int)unit->rtl816xu_UnitNum);

        RTLD(bug("[rtl816x] CreateUnit: Unit allocated @ 0x%p\n", unit))

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);

        unit->rtl816xu_device     = RTL816XDeviceBase;
        unit->rtl816xu_PCIDevice  = pciDevice;
        unit->rtl816xu_PCIDriver  = driver;

        unit->rtl816xu_mtu        = unit->rtl816xu_Sana2Info.MTU;

        InitSemaphore(&unit->rtl816xu_unit_lock);
        NEWLIST(&unit->rtl816xu_Openers);
        NEWLIST(&unit->rtl816xu_multicast_ranges);
        NEWLIST(&unit->rtl816xu_type_trackers);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &unit->rtl816xu_IRQ);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, (IPTR *)&unit->rtl816xu_BaseIO);

        /* The MMIO BAR position varies across the family - use the first
           memory BAR */
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base1, &mmiobase);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size1, &mmiolen);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Type1, &type);
        if ((mmiolen == 0) || (type & ADDRF_IO))
        {
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base2, &mmiobase);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size2, &mmiolen);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Type2, &type);
            if ((mmiolen == 0) || (type & ADDRF_IO))
            {
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base3, &mmiobase);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size3, &mmiolen);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Type3, &type);
                if ((mmiolen == 0) || (type & ADDRF_IO))
                {
                    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base4, &mmiobase);
                    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size4, &mmiolen);
                    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Type4, &type);
                    if ((mmiolen == 0) || (type & ADDRF_IO))
                    {
                        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base5, &mmiobase);
                        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size5, &mmiolen);
                        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Type5, &type);
                    }
                }
            }
        }

        RTLD(bug("[%s] CreateUnit: INT:%d, io:0x%p, mmio:0x%p, size:%d\n", unit->rtl816xu_name,
                 (int)unit->rtl816xu_IRQ, unit->rtl816xu_BaseIO, (APTR)mmiobase, (int)mmiolen))

        if (type & ADDRF_IO)
        {
            RTLD(bug("[%s] CreateUnit: no memory-space BAR found!\n", unit->rtl816xu_name))
            mmioerror = TRUE;
        }

        if (mmiolen < R816X_REGS_SIZE)
        {
            RTLD(bug("[%s] CreateUnit: invalid MMIO region size (%d, expected %d)\n", unit->rtl816xu_name,
                     (int)mmiolen, R816X_REGS_SIZE))
            mmioerror = TRUE;
        }

        if (!mmioerror)
        {
            if (HIDD_PCIDevice_Obtain(pciDevice,
                    RTL816XDeviceBase->rtl816xb_Device.dd_Library.lib_Node.ln_Name))
            {
                RTLD(bug("[%s] CreateUnit: device is already owned\n", unit->rtl816xu_name))
                mmioerror = TRUE;
            }
            else
                unit->rtl816xu_Owned = TRUE;
        }

        if (mmioerror)
        {
            FreeVec(unit->rtl816xu_name);
            FreeMem(unit, sizeof(struct RTL816XUnit));
            return NULL;
        }

        unit->rtl816xu_SizeMem = R816X_REGS_SIZE;
        unit->rtl816xu_BaseMem = HIDD_PCIDriver_MapPCI(driver, (APTR)mmiobase, unit->rtl816xu_SizeMem);

        if (unit->rtl816xu_BaseMem != NULL)
        {
            struct TagItem attrs[] =
            {
                { aHidd_PCIDevice_isIO,     TRUE },
                { aHidd_PCIDevice_isMEM,    TRUE },
                { aHidd_PCIDevice_isMaster, TRUE },
                { TAG_DONE,                 0    },
            };
            OOP_SetAttrs(pciDevice, (struct TagItem *)&attrs);

            RTLD(bug("[%s] CreateUnit: PCI_BaseMem @ 0x%p\n", unit->rtl816xu_name, unit->rtl816xu_BaseMem))

            unit->rtl816xu_DelayPort.mp_SigBit = SIGB_SINGLE;
            unit->rtl816xu_DelayPort.mp_Flags = PA_SIGNAL;
            unit->rtl816xu_DelayPort.mp_SigTask = FindTask(NULL);
            unit->rtl816xu_DelayPort.mp_Node.ln_Type = NT_MSGPORT;
            NEWLIST(&unit->rtl816xu_DelayPort.mp_MsgList);

            unit->rtl816xu_DelayReq.tr_node.io_Message.mn_ReplyPort = &unit->rtl816xu_DelayPort;
            unit->rtl816xu_DelayReq.tr_node.io_Message.mn_Length = sizeof(struct timerequest);

            if (OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ,
                           (struct IORequest *)&unit->rtl816xu_DelayReq, 0) == 0)
            {
                if ((unit->rtl816xu_priv = AllocMem(sizeof(struct rtl816x_priv),
                                                    MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
                {
                    unit->rtl816xu_priv->pci_dev = unit;
                    InitSemaphore(&unit->rtl816xu_priv->lock);

                    unit->rtl816xu_irqhandler.is_Node.ln_Type = NT_INTERRUPT;
                    unit->rtl816xu_irqhandler.is_Node.ln_Pri = 100;
                    unit->rtl816xu_irqhandler.is_Node.ln_Name = RTL816XDeviceBase->rtl816xb_Device.dd_Library.lib_Node.ln_Name;
                    unit->rtl816xu_irqhandler.is_Code = (VOID_FUNC)RTL816X_IntHandlerF;
                    unit->rtl816xu_irqhandler.is_Data = unit;

                    unit->rtl816xu_tx_int.is_Node.ln_Type = NT_INTERRUPT;
                    unit->rtl816xu_tx_int.is_Node.ln_Name = unit->rtl816xu_name;
                    unit->rtl816xu_tx_int.is_Code = (VOID_FUNC)RTL816X_TX_IntF;
                    unit->rtl816xu_tx_int.is_Data = unit;

                    for (i = 0; i < REQUEST_QUEUE_COUNT; i++)
                    {
                        struct MsgPort *port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);

                        unit->rtl816xu_request_ports[i] = port;

                        if (port == NULL)
                            success = FALSE;
                        else
                        {
                            NEWLIST(&port->mp_MsgList);
                            port->mp_Flags = PA_IGNORE;
                            port->mp_SigTask = &unit->rtl816xu_tx_int;
                        }
                    }

                    if (success)
                    {
                        unit->rtl816xu_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

                        struct RTL816XStartup *sm_UD;
                        UBYTE tmpbuff[100];

                        if ((sm_UD = AllocMem(sizeof(struct RTL816XStartup),
                                              MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
                        {
                            BOOL procOK = FALSE;

                            sprintf((char *)tmpbuff, RTL816X_TASK_NAME, unit->rtl816xu_name);

                            sm_UD->rtl816xsm_Parent = FindTask(NULL);
                            sm_UD->rtl816xsm_Unit = unit;

                            rtl816x_get_functions(unit);

                            SetSignal(0, SIGF_SINGLE);

                            unit->rtl816xu_Process = CreateNewProcTags(
                                        NP_Entry, (IPTR)RTL816X_Schedular,
                                        NP_Name, tmpbuff,
                                        NP_Synchronous, FALSE,
                                        NP_Priority, 0,
                                        NP_UserData, (IPTR)sm_UD,
                                        NP_StackSize, 140960,
                                        TAG_DONE);

                            if (unit->rtl816xu_Process != NULL)
                            {
                                Wait(SIGF_SINGLE);
                                /* The unit process clears rtl816xsm_Unit if
                                   its setup failed */
                                procOK = (sm_UD->rtl816xsm_Unit != NULL);
                            }
                            FreeMem(sm_UD, sizeof(struct RTL816XStartup));

                            if (procOK)
                            {
                                RTLD(bug("[%s] CreateUnit: %s initialised, unit %d @ %p\n",
                                         unit->rtl816xu_name, unit->rtl816xu_rtl_chipname,
                                         (int)unit->rtl816xu_UnitNum, unit))
                                return unit;
                            }
                            /* The failed process cleaned up after itself
                               before signalling */
                            unit->rtl816xu_Process = NULL;
                        }
                    }
                }
            }
        }
        else
        {
            RTLD(bug("[rtl816x] CreateUnit: couldn't map MMIO area - aborting\n"))
        }
        DeleteUnit(RTL816XDeviceBase, unit);
    }
    return NULL;
}

/*
 * DeleteUnit - removes selected unit. Frees all resources and structures.
 *
 * The caller should be sure that the given unit is really ready to be freed.
 */
void DeleteUnit(struct RTL816XBase *RTL816XDeviceBase, struct RTL816XUnit *Unit)
{
    UBYTE tmpbuff[100];
    int i;

    if (Unit)
    {
        if (Unit->rtl816xu_Process)
        {
            /* Tell the unit process to quit, and wait until it does so */
            Signal(&Unit->rtl816xu_Process->pr_Task,
                   1UL << Unit->rtl816xu_signal_0);
            sprintf((char *)tmpbuff, RTL816X_TASK_NAME, Unit->rtl816xu_name);
            while (FindTask(tmpbuff) != NULL)
                Delay(5);
            Unit->rtl816xu_Process = NULL;
        }

        for (i = 0; i < REQUEST_QUEUE_COUNT; i++)
        {
            if (Unit->rtl816xu_request_ports[i] != NULL)
                FreeMem(Unit->rtl816xu_request_ports[i], sizeof(struct MsgPort));

            Unit->rtl816xu_request_ports[i] = NULL;
        }

        if (Unit->rtl816xu_priv)
        {
            FreeMem(Unit->rtl816xu_priv, sizeof(struct rtl816x_priv));
            Unit->rtl816xu_priv = NULL;
        }

        if (Unit->rtl816xu_DelayReq.tr_node.io_Device != NULL)
            CloseDevice((struct IORequest *)&Unit->rtl816xu_DelayReq);

        if (Unit->rtl816xu_BaseMem)
        {
            HIDD_PCIDriver_UnmapPCI(Unit->rtl816xu_PCIDriver,
                                    (APTR)Unit->rtl816xu_BaseMem,
                                    Unit->rtl816xu_SizeMem);
        }

        if (Unit->rtl816xu_Owned)
            HIDD_PCIDevice_Release(Unit->rtl816xu_PCIDevice);

        FreeVec(Unit->rtl816xu_name);
        FreeMem(Unit, sizeof(struct RTL816XUnit));
    }
}
