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

#define DEBUG 0

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>

#include <aros/debug.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/newstyle.h>
#include <devices/timer.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/battclock.h>
#include <proto/oop.h>
#include <proto/timer.h>
#include <proto/utility.h>

#include <stdlib.h>

#include "nforce.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/*
 * Report incoming events to all hyphotetical event receivers
 */
VOID ReportEvents(struct NFBase *NforceBase, struct NFUnit *unit, ULONG events)
{
    struct IOSana2Req *request, *tail, *next_request;
    struct List *list;

    list = &unit->request_ports[EVENT_QUEUE]->mp_MsgList;
    next_request = (APTR)list->lh_Head;
    tail = (APTR)&list->lh_Tail;

    /* Go through list of event listeners. If send messages to receivers if event found */
    Disable();
    while(next_request != tail)
    {
        request = next_request;
        next_request = (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

        if((request->ios2_WireError&events) != 0)
        {
            request->ios2_WireError = events;
            Remove((APTR)request);
            ReplyMsg((APTR)request);
        }
    }
    Enable();

    return;
}

struct TypeStats *FindTypeStats(struct NFBase *NforceBase, struct NFUnit *unit, 
                    struct MinList *list, ULONG packet_type)
{
    struct TypeStats *stats, *tail;
    BOOL found = FALSE;

    stats = (APTR)list->mlh_Head;
    tail = (APTR)&list->mlh_Tail;

    while(stats != tail && !found)
    {
        if(stats->packet_type == packet_type)
            found = TRUE;
        else
            stats = (APTR)stats->node.mln_Succ;
    }

    if(!found)
        stats = NULL;

    return stats;
}

void FlushUnit(LIBBASETYPEPTR LIBBASE, struct NFUnit *unit, UBYTE last_queue, BYTE error)
{
    struct IORequest *request;
    UBYTE i;
    struct Opener *opener, *tail;

    D(bug("[nforce] FlushUnit\n"));

    /* Abort queued operations */

    for (i=0; i <= last_queue; i++)
    {
        while ((request = (APTR)GetMsg(unit->request_ports[i])) != NULL)
        {
            request->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *)request);
        }
    }

    opener = (APTR)unit->nu_Openers.mlh_Head;
    tail = (APTR)unit->nu_Openers.mlh_Tail;

    /* Flush every opener's read queue */

    while(opener != tail)
    {
        while ((request = (APTR)GetMsg(&opener->read_port)) != NULL)
        {
            request->io_Error = error;
            ReplyMsg((struct Message *)request);
        }
        opener = (struct Opener *)opener->node.mln_Succ;
    }
}

static inline volatile ULONG readl(APTR base)
{
    return *((ULONG*)base);
}

static inline volatile void writel(ULONG val, APTR base)
{
    *((ULONG*)base) = val;
}

static inline void pci_push(UBYTE *base)
{
    /* force out pending posted writes */
    readl(base);
}

/*
 * Interrupt handler called whenever nForce NIC interface generates interrupt.
 * It's duty is to iterate throgh RX queue searching for new packets.
 * 
 * Please note, that allthough multicast support could be done on interface 
 * basis, it is done in this function as result of quick integration of both
 * the forcedeth driver (IFF_ALLMULTI flag) and etherling3 driver (AddressMatch
 * filter function).
 */
AROS_UFH3(void, RX_Int,
    AROS_UFHA(struct NFUnit *,  unit, A1),
    AROS_UFHA(APTR,             dummy, A5),
    AROS_UFHA(struct ExecBase *,SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct NFBase *NforceBase = unit->nu_device;
    struct fe_priv *np = unit->nu_fe_priv;
    ULONG Flags;
    struct TypeStats *tracker;
    ULONG packet_type;
    struct Opener *opener, *opener_tail;
    struct IOSana2Req *request, *request_tail;
    BOOL accepted, is_orphan;

    /* Endless loop, with break from inside */
    for(;;)
    {
        int i,len;
        struct eth_frame *frame;

        if (np->cur_rx - np->refill_rx >= RX_RING)
            break;	/* we scanned the whole ring - do not continue */

        /* Get the in-queue number */
        i = np->cur_rx % RX_RING;
        Flags = AROS_LE2LONG(np->rx_ring[i].FlagLen);
        len = unit->descr_getlength(&np->rx_ring[i], np->desc_ver);

        D(bug("%s: nv_rx_process: looking at packet %d, Flags 0x%x, len=%d\n",
                unit->name, np->cur_rx, Flags, len));

        /* Free frame? Do nothing - we've empty queue now */
        if (Flags & NV_RX_AVAIL)
            break;	/* still owned by hardware, */

        /*
         * the packet is for us - get it :)
         */

        /* look at what we actually got: */
        if (np->desc_ver == DESC_VER_1) {
            if (!(Flags & NV_RX_DESCRIPTORVALID))
                goto next_pkt;

            if (Flags & NV_RX_MISSEDFRAME) {
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->stats.BadData++;
                goto next_pkt;
            }
            if (Flags & (NV_RX_ERROR1|NV_RX_ERROR2|NV_RX_ERROR3|NV_RX_ERROR4)) {
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->stats.BadData++;
                goto next_pkt;
            }
            if (Flags & NV_RX_CRCERR) {
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->stats.BadData++;
                goto next_pkt;
            }
            if (Flags & NV_RX_OVERFLOW) {
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->stats.BadData++;
                goto next_pkt;
            }
            if (Flags & NV_RX_ERROR) {
                /* framing errors are soft errors, the rest is fatal. */
                if (Flags & NV_RX_FRAMINGERR) {
                    if (Flags & NV_RX_SUBSTRACT1) {
                        len--;
                    }
                } else {
                    ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                    unit->stats.BadData++;
                    goto next_pkt;
                }
            }
        } else {
            if (!(Flags & NV_RX2_DESCRIPTORVALID))
                goto next_pkt;

            if (Flags & (NV_RX2_ERROR1|NV_RX2_ERROR2|NV_RX2_ERROR3|NV_RX2_ERROR4)) {
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->stats.BadData++;
                goto next_pkt;
            }
            if (Flags & NV_RX2_CRCERR) {
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->stats.BadData++;
                goto next_pkt;
            }
            if (Flags & NV_RX2_OVERFLOW) {
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->stats.BadData++;
                goto next_pkt;
            }
            if (Flags & NV_RX2_ERROR) {
                /* framing errors are soft errors, the rest is fatal. */
                if (Flags & NV_RX2_FRAMINGERR) {
                    if (Flags & NV_RX2_SUBSTRACT1) {
                        len--;
                    }
                } else {
                    ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                    unit->stats.BadData++;
                    goto next_pkt;
                }
            }
            Flags &= NV_RX2_CHECKSUMMASK;
            if (Flags == NV_RX2_CHECKSUMOK1 ||
                    Flags == NV_RX2_CHECKSUMOK2 ||
                    Flags == NV_RX2_CHECKSUMOK3) {
                D(bug("%s: hw checksum hit!.\n", unit->name));
            } else {
                D(bug("%s: hwchecksum miss!.\n", unit->name));
            }
        }

        /* got a valid packet - forward it to the network core */
        frame = &np->rx_buffer[i];
        is_orphan = TRUE;

        /* Dump contents of frame if DEBUG enabled */
#ifdef DEBUG
                {
                int j;
                    for (j=0; j<64; j++) {
                        if ((j%16) == 0)
                            D(bug("\n%03x:", j));
                        D(bug(" %02x", ((unsigned char*)frame)[j]));
                    }
                    D(bug("\n"));
                }
#endif

        /* Check for address validity */
        if(AddressFilter(LIBBASE, unit, frame->eth_packet_dest))
        {
            /* Packet is addressed to this driver */
            packet_type = AROS_BE2WORD(frame->eth_packet_type);

            opener = (APTR)unit->nu_Openers.mlh_Head;
            opener_tail = (APTR)&unit->nu_Openers.mlh_Tail;

            /* Offer packet to every opener */
            while(opener != opener_tail)
            {
               request = (APTR)opener->read_port.mp_MsgList.lh_Head;
               request_tail = (APTR)&opener->read_port.mp_MsgList.lh_Tail;
               accepted = FALSE;

               /* Offer packet to each request until it's accepted */
               while((request != request_tail) && !accepted)
               {
                  if((request->ios2_PacketType == packet_type)
                     || ((request->ios2_PacketType <= ETH_MTU)
                          && (packet_type <= ETH_MTU)))
                  {
                     CopyPacket(LIBBASE, unit, request, len, packet_type, frame);
                     accepted = TRUE;
                  }
                  request =
                     (struct IOSana2Req *)request->ios2_Req.io_Message.mn_Node.ln_Succ;
               }

               if(accepted)
                  is_orphan = FALSE;

               opener = (APTR)opener->node.mln_Succ;
            }

            /* If packet was unwanted, give it to S2_READORPHAN request */
            if(is_orphan)
            {
                unit->stats.UnknownTypesReceived++;

                if(!IsMsgPortEmpty(unit->request_ports[ADOPT_QUEUE]))
                {
                    CopyPacket(LIBBASE, unit,
                        (APTR)unit->request_ports[ADOPT_QUEUE]->
                        mp_MsgList.lh_Head, len, packet_type, frame);
                }
            }

            /* Update remaining statistics */

            tracker =
                FindTypeStats(LIBBASE, unit, &unit->type_trackers, packet_type);
            if(tracker != NULL)
            {
                tracker->stats.PacketsReceived++;
                tracker->stats.BytesReceived += len;
            }
        }

        unit->stats.PacketsReceived++;

next_pkt:
        np->cur_rx++;
    }

    AROS_USERFUNC_EXIT
}

/*
 * Check status of packets which we've already sent to the NIC. Update
 * statistics, and reenable TX queue if only there is some free space.
 */
static void nv_tx_done(struct NFUnit *unit)
{
    struct fe_priv *np = unit->nu_fe_priv;
    struct NFBase *NforceBase = unit->nu_device;
    ULONG Flags;
    int i;

    /* Go through tx chain and mark all send packets as free */	
    while (np->nic_tx != np->next_tx)
    {
        i = np->nic_tx % TX_RING;

        Flags = AROS_LE2LONG(np->tx_ring[i].FlagLen);

        D(bug("%s: nv_tx_done: looking at packet %d, Flags 0x%x.\n",
                unit->name, np->nic_tx, Flags));

        if (Flags & NV_TX_VALID)
            break;

        if (np->desc_ver == DESC_VER_1) {
            if (Flags & (NV_TX_RETRYERROR|NV_TX_CARRIERLOST|NV_TX_LATECOLLISION|
                         NV_TX_UNDERFLOW|NV_TX_ERROR))
            {
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_TX);
            }
            else
            {
                unit->stats.PacketsSent++;
            }
        }
        else
        {
            if (Flags & (NV_TX2_RETRYERROR|NV_TX2_CARRIERLOST|NV_TX2_LATECOLLISION|
                         NV_TX2_UNDERFLOW|NV_TX2_ERROR))
            {
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_TX);
            }
            else
            {
                unit->stats.PacketsSent++;
            }
        }
        np->nic_tx++;
    }

    /*
     * Do we have some spare space in TX queue and the queue self is blocked?
     * Reenable it then!
     */
    if (np->next_tx - np->nic_tx < TX_LIMIT_START) {
        if (netif_queue_stopped(unit)) {
            bug("%s: output queue restart\n", unit->name);
            unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
            netif_wake_queue(unit);
        }
    }
}

/*
 * Interrupt generated by Cause() to push new packets into the NIC interface
 */
AROS_UFH3(void, TX_Int,
    AROS_UFHA(struct NFUnit *,  unit, A1),
    AROS_UFHA(APTR,             dummy, A5),
    AROS_UFHA(struct ExecBase *,SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct fe_priv *np = unit->nu_fe_priv;
    struct NFBase *NforceBase = unit->nu_device;
    int nr;
    BOOL proceed = FALSE; /* Fails by default */

    /* send packet only if there is free space on tx queue. Otherwise do nothing */
    if (!netif_queue_stopped(unit))
    {
        UWORD packet_size, data_size;
        struct NFBase *base;
        struct IOSana2Req *request;
        struct Opener *opener;
        UBYTE *buffer;
        ULONG wire_error=0;
        BYTE error;
        struct MsgPort *port;
        struct TypeStats *tracker;

        proceed = TRUE; /* Success by default */
        base = unit->nu_device;
        port = unit->request_ports[WRITE_QUEUE];

        /* Still no error and there are packets to be sent? */
        while(proceed && (!IsMsgPortEmpty(port)))
        {
            nr = np->next_tx % TX_RING;
            error = 0;

            request = (APTR)port->mp_MsgList.lh_Head;
            data_size = packet_size = request->ios2_DataLength;

            opener = (APTR)request->ios2_BufferManagement;

            if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
            {
                packet_size += ETH_PACKET_DATA;
                CopyMem(request->ios2_DstAddr, np->tx_buffer[nr].eth_packet_dest, ETH_ADDRESSSIZE);
                CopyMem(unit->dev_addr, np->tx_buffer[nr].eth_packet_source, ETH_ADDRESSSIZE);
                np->tx_buffer[nr].eth_packet_type = AROS_WORD2BE(request->ios2_PacketType);

                buffer = np->tx_buffer[nr].eth_packet_data;
            }
            else
                buffer = (UBYTE*)&np->tx_buffer[nr];

            if (!opener->tx_function(buffer, request->ios2_Data, data_size))
            {
                error = S2ERR_NO_RESOURCES;
                wire_error = S2WERR_BUFF_ERROR;
                ReportEvents(LIBBASE, unit,
                  S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF
                  | S2EVENT_TX);
            }

            /* Now the packet is already in TX buffer, update flags for NIC */
            if (error == 0)
            {
                Disable();
                np->tx_ring[nr].FlagLen = AROS_LONG2LE((packet_size-1) | np->tx_flags );
                D(bug("%s: nv_start_xmit: packet packet %d queued for transmission.",
                        unit->name, np->next_tx));

                /* DEBUG? Dump frame if so */
#ifdef DEBUG
                {
                int j;
                    for (j=0; j<64; j++) {
                        if ((j%16) == 0)
                            D(bug("\n%03x:", j));
                        D(bug(" %02x", ((unsigned char*)&np->tx_buffer[nr])[j]));
                    }
                    D(bug("\n"));
                }
#endif
                np->next_tx++;

                /* 
                 * If we've just run out of free space on the TX queue, stop
                 * it and give up pushing further frames
                 */
                if (np->next_tx - np->nic_tx >= TX_LIMIT_STOP)
                {
                    bug("%s: output queue full. Stopping\n", unit->name);
                    netif_stop_queue(unit);
                    proceed = FALSE;
                }
                Enable();
                /* 
                 * At this place linux driver used to trigger NIC to output
                 * the queued packets through wire. We will not do it as we
                 * may already see if there are new outcomming packets.
                 * 
                 * Yes, this driver might be a bit faster than linux one.
                 */
            }

            /* Reply packet */

            request->ios2_Req.io_Error = error;
            request->ios2_WireError = wire_error;
            Disable();
            Remove((APTR)request);
            Enable();
            ReplyMsg((APTR)request);

            /* Update statistics */

            if(error == 0)
            {
                tracker = FindTypeStats(LIBBASE, unit, &unit->type_trackers,
                    request->ios2_PacketType);
                if(tracker != NULL)
                {
                    tracker->stats.PacketsSent++;
                    tracker->stats.BytesSent += packet_size;
                }
            }	
        }

        /* 
         * Here either we've filled the queue with packets to be transmitted,
         * or just run out of spare space in TX queue. In both cases tell the
         * NIC to start transmitting them all through wire.
         */
        writel(NVREG_TXRXCTL_KICK|np->desc_ver, (UBYTE*)unit->nu_BaseMem + NvRegTxRxControl);
        pci_push((UBYTE*)unit->nu_BaseMem);
    }

    /* Was there success? Enable incomming of new packets */    
    if(proceed)
        unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
    else
        unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;

    AROS_USERFUNC_EXIT
}

/*
 * Interrupt used to restart the real one
 */
AROS_UFH3(void, TX_End_Int,
    AROS_UFHA(struct NFUnit *,  unit, A1),
    AROS_UFHA(APTR,             dummy, A5),
    AROS_UFHA(struct ExecBase *,SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct NFUnit *dev = unit;
    struct fe_priv *np = dev->nu_fe_priv;
    UBYTE *base = (UBYTE*) dev->nu_BaseMem;

    Disable();

    writel(np->irqmask, base + NvRegIrqMask);
    pci_push(base);
    dev->nu_irqhandler->h_Code(dev->nu_irqhandler, NULL);
    Enable();

    AROS_USERFUNC_EXIT
}

/*
 * Maximum number of loops until we assume that a bit in the irq mask
 * is stuck. Overridable with module param.
 */
static const int max_interrupt_work = 5;

/*
 * Handle timeouts and other strange cases
 */
static void NF_TimeoutHandler(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct NFUnit *dev = (struct NFUnit *) irq->h_Data;
    struct timeval time;
    struct Device *TimerBase = dev->nu_TimerSlowReq->tr_node.io_Device;

    GetSysTime(&time);

    /*
     * If timeout timer is expected, and time elapsed - regenerate the 
     * interrupt handler 
     */
    if (dev->nu_toutNEED && (CmpTime(&time, &dev->nu_toutPOLL ) < 0))
    {
        dev->nu_toutNEED = FALSE;
        Cause(&dev->tx_end_int);
    }
}

/*
 * The interrupt handler - schedules code execution to proper handlers depending
 * on the message from nForce.
 * 
 * NOTE.
 * 
 * Don't be surprised - this driver used to restart itself several times, in
 * order to handle events which occur when the driver was handling previous
 * events. It reduces the latency and amount of dropped packets. Additionally, 
 * this interrupt may put itself into deep sleep (or just quit) and restarts 
 * after certain amount of time (POLL_WAIT).
 */
static void NF_IntHandler(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct NFUnit *dev = (struct NFUnit *) irq->h_Data;
    struct fe_priv *np = dev->nu_fe_priv;
    UBYTE *base = (UBYTE*) dev->nu_BaseMem;
    ULONG events;
    int i;
    struct Device *TimerBase = dev->nu_TimerSlowReq->tr_node.io_Device;
    struct timeval time;

    GetSysTime(&time);

    /* Restart automagically :) */
    for (i=0; ; i++)
    {
        events = readl(base + NvRegIrqStatus) & NVREG_IRQSTAT_MASK;
        writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
        pci_push(base);

        if (!(events & np->irqmask))
            break;

        /* 
         * Some packets have been sent? Just update statistics and empty the
         * TX queue
         */
        if (events & (NVREG_IRQ_TX1|NVREG_IRQ_TX2|NVREG_IRQ_TX_ERR)) {
            nv_tx_done(dev);
        }

        /* Something received? Handle it! */
        if (events & (NVREG_IRQ_RX_ERROR|NVREG_IRQ_RX|NVREG_IRQ_RX_NOBUF)) {
            AROS_UFC3(void, dev->rx_int.is_Code,
                AROS_UFCA(APTR, dev->rx_int.is_Data, A1),
                AROS_UFCA(APTR, dev->rx_int.is_Code, A5),
                AROS_UFCA(struct ExecBase *, SysBase, A6));
            /* Mark received frames as free for hardware */
            dev->alloc_rx(dev);
        }

        if (events & (NVREG_IRQ_LINK)) {
            Disable();
            dev->linkirq(dev);
            Enable();
        }

        /* If linktimer interrupt required, handle it here */
        if (np->need_linktimer && (CmpTime(&time, &np->link_timeout) < 0)) {
            Disable();
            dev->linkchange(dev);
            Enable();
            np->link_timeout.tv_micro = LINK_TIMEOUT % 1000000;
            np->link_timeout.tv_secs = LINK_TIMEOUT / 1000000;
            AddTime(&np->link_timeout, &time);
        }

        /* Erm? */
        if (events & (NVREG_IRQ_TX_ERR)) {
            bug("%s: received irq with events 0x%x. Probably TX fail.\n",
                    dev->name, events);
        }

        if (events & (NVREG_IRQ_UNKNOWN)) {
            bug("%s: received irq with unknown events 0x%x. Please report\n",
                    dev->name, events);
        }

        /* 
         * Straaaaaaaange, the interrupt was restarted more than 
         * max_interrupt_work times. Normally it should not happen, even on
         * gigabit ethernet. In any case setup poll handler which restart this
         * handler after specified amount of time.
         */
        if (i > max_interrupt_work)
        {
            bug("%s: too many iterations (%d) in nv_nic_irq.\n", dev->name, i);
            writel(0, base + NvRegIrqMask);
            pci_push(base);

            /* When to wake up? */
            Disable();
            dev->nu_toutPOLL.tv_micro = POLL_WAIT % 1000000;
            dev->nu_toutPOLL.tv_secs  = POLL_WAIT / 1000000;
            AddTime(&dev->nu_toutPOLL, &time);
            dev->nu_toutNEED = TRUE;
            Enable();

            break; /* break the for() loop */
        }
    }

    /*
     * If TX queue was stopped, try to reenable it *ALWAYS*
     */
    if (netif_queue_stopped(dev)) {
        nv_tx_done(dev);
    }
}

VOID CopyPacket(struct NFBase *NforceBase, struct NFUnit *unit, 
    struct IOSana2Req *request, UWORD packet_size, UWORD packet_type,
    struct eth_frame *buffer)
{
    struct Opener *opener;
    BOOL filtered = FALSE;
    UBYTE *ptr;

    /* Set multicast and broadcast flags */

    request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
    if((*((ULONG *)(buffer->eth_packet_dest)) == 0xffffffff) &&
       (*((UWORD *)(buffer->eth_packet_dest + 4)) == 0xffff))
        request->ios2_Req.io_Flags |= SANA2IOF_BCAST;

    else if((buffer->eth_packet_dest[0] & 0x1) != 0)
        request->ios2_Req.io_Flags |= SANA2IOF_MCAST;

    /* Set source and destination addresses and packet type */
    CopyMem(buffer->eth_packet_source, request->ios2_SrcAddr, ETH_ADDRESSSIZE);
    CopyMem(buffer->eth_packet_dest, request->ios2_DstAddr, ETH_ADDRESSSIZE);
    request->ios2_PacketType = packet_type;

    /* Adjust for cooked packet request */

    if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
    {
        packet_size -= ETH_PACKET_DATA;
        ptr = buffer->eth_packet_data;
    }
    else
    {
        ptr = (UBYTE*)buffer;
    }

    request->ios2_DataLength = packet_size;

    /* Filter packet */

    opener = request->ios2_BufferManagement;
    if((request->ios2_Req.io_Command == CMD_READ) &&
        (opener->filter_hook != NULL))
        if(!CallHookPkt(opener->filter_hook, request, ptr))
            filtered = TRUE;

    if(!filtered)
    {
        /* Copy packet into opener's buffer and reply packet */

        if(!opener->rx_function(request->ios2_Data, ptr, packet_size))
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

BOOL AddressFilter(struct NFBase *NforceBase, struct NFUnit *unit, UBYTE *address)
{
    struct AddressRange *range, *tail;
    BOOL accept = TRUE;
    ULONG address_left;
    UWORD address_right;

    /* Check whether address is unicast/broadcast or multicast */

    address_left = AROS_BE2LONG(*((ULONG *)address));
    address_right = AROS_BE2WORD(*((UWORD *)(address + 4)));

    if((address_left & 0x01000000) != 0 &&
        !(address_left == 0xffffffff && address_right == 0xffff))
    {
        /* Check if this multicast address is wanted */

        range = (APTR)unit->multicast_ranges.mlh_Head;
        tail = (APTR)&unit->multicast_ranges.mlh_Tail;
        accept = FALSE;

        while((range != tail) && !accept)
        {
            if((address_left > range->lower_bound_left ||
                (address_left == range->lower_bound_left &&
                address_right >= range->lower_bound_right)) &&
                (address_left < range->upper_bound_left ||
                (address_left == range->upper_bound_left &&
                address_right <= range->upper_bound_right)))
                accept = TRUE;
            range = (APTR)range->node.mln_Succ;
        }

        if(!accept)
            unit->special_stats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    }
    return accept;
}

/*
 * Unit process
 */
AROS_UFH3(void, NF_Scheduler,
    AROS_UFHA(STRPTR,              argPtr, A0),
    AROS_UFHA(ULONG,               argSize, D0),
    AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct NFUnit *dev = FindTask(NULL)->tc_UserData;
    LIBBASETYPEPTR LIBBASE = dev->nu_device;
    APTR BattClockBase;
    struct MsgPort *reply_port, *input;

    D(bug("[NFORCE] In nforce process\n"));
    D(bug("[NFORCE] Setting device up\n"));

    reply_port = CreateMsgPort();
    input = CreateMsgPort();

    dev->nu_input_port = input; 

    /* Randomize the generator with current time */
    BattClockBase =  OpenResource("battclock.resource");
    srandom(ReadBattClock());

    dev->nu_TimerSlowPort = CreateMsgPort();

    if (dev->nu_TimerSlowPort)
    {
        dev->nu_TimerSlowReq = (struct timerequest *)
            CreateIORequest((struct MsgPort *)dev->nu_TimerSlowPort, sizeof(struct timerequest));

        if (dev->nu_TimerSlowReq)
        {
            if (!OpenDevice("timer.device", UNIT_VBLANK,
                (struct IORequest *)dev->nu_TimerSlowReq, 0))
            {
                struct Message *msg = AllocVec(sizeof(struct Message), MEMF_PUBLIC|MEMF_CLEAR);
                ULONG sigset;

                D(bug("[NFORCE] Got VBLANK unit of timer.device\n"));

                dev->initialize(dev);

                msg->mn_ReplyPort = reply_port;
                msg->mn_Length = sizeof(struct Message);

                D(bug("[NFORCE] Setup complete. Sending handshake\n"));
                PutMsg(LIBBASE->nf_syncport, msg);
                WaitPort(reply_port);
                GetMsg(reply_port);

                FreeVec(msg);

                D(bug("[NFORCE] Forever loop\n"));

                dev->nu_signal_0 = AllocSignal(-1);
                dev->nu_signal_1 = AllocSignal(-1);
                dev->nu_signal_2 = AllocSignal(-1);
                dev->nu_signal_3 = AllocSignal(-1);

                sigset = 1 << input->mp_SigBit  |
                         1 << dev->nu_signal_0  |
                         1 << dev->nu_signal_1  |
                         1 << dev->nu_signal_2  |
                         1 << dev->nu_signal_3;
                for(;;)
                {	
                    ULONG recvd = Wait(sigset);
                    if (recvd & dev->nu_signal_0)
                    {
                        /*
                         * Shutdown process. Driver should close everything 
                         * already and waits for our process to complete. Free
                         * memory allocared here and kindly return.
                         */
                        dev->deinitialize(dev);
                        CloseDevice((struct IORequest *)dev->nu_TimerSlowReq);
                        DeleteIORequest((struct IORequest *)dev->nu_TimerSlowReq);
                        DeleteMsgPort(dev->nu_TimerSlowPort);
                        DeleteMsgPort(input);
                        DeleteMsgPort(reply_port);

                        D(bug("[NFORCE] Process shutdown.\n"));
                        return;
                    }
                    else if (recvd & (1 << input->mp_SigBit))
                    {
                        struct IOSana2Req *io;

                        /* Handle incoming transactions */
                        while ((io = (struct IOSana2Req *)GetMsg(input))!= NULL);
                        {
                            ObtainSemaphore(&dev->unit_lock);
                            handle_request(LIBBASE, io);
                        }
                    }
                    else
                    {
                        /* Handle incoming signals */
                    }
                }
            }
        }
    }

    AROS_USERFUNC_EXIT
}

static const struct DriverConfig {
    ULONG   ProductID;
    ULONG   DriverFlags;
    ULONG   DescVer;
} Config[] = {
    { NFORCE_MCPNET1_ID, DEV_IRQMASK_1|DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER,                        DESC_VER_1 },
    { NFORCE_MCPNET2_ID, DEV_IRQMASK_2|DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_NEED_LASTPACKET1,   DESC_VER_1 },
    { NFORCE_MCPNET3_ID, DEV_IRQMASK_2|DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_NEED_LASTPACKET1,   DESC_VER_1 },
    { NFORCE_MCPNET4_ID, DEV_IRQMASK_2|DEV_NEED_TIMERIRQ|DEV_NEED_LASTPACKET1,                      DESC_VER_2 },
    { NFORCE_MCPNET5_ID, DEV_IRQMASK_2|DEV_NEED_TIMERIRQ|DEV_NEED_LASTPACKET1,                      DESC_VER_2 },
    { NFORCE_MCPNET6_ID, DEV_IRQMASK_2|DEV_NEED_TIMERIRQ|DEV_NEED_LASTPACKET1,                      DESC_VER_2 },
    { NFORCE_MCPNET7_ID, DEV_IRQMASK_2|DEV_NEED_TIMERIRQ|DEV_NEED_LASTPACKET1,                      DESC_VER_2 },
    { NFORCE_MCPNET8_ID, DEV_IRQMASK_2|DEV_NEED_TIMERIRQ|DEV_NEED_LASTPACKET1,                      DESC_VER_2 },
    { NFORCE_MCPNET9_ID, DEV_IRQMASK_2|DEV_NEED_TIMERIRQ|DEV_NEED_LASTPACKET1,                      DESC_VER_2 },
    { NFORCE_MCPNET10_ID, DEV_IRQMASK_2|DEV_NEED_TIMERIRQ|DEV_NEED_LASTPACKET1,                      DESC_VER_2 },
    { NFORCE_MCPNET11_ID, DEV_IRQMASK_2|DEV_NEED_TIMERIRQ|DEV_NEED_LASTPACKET1,                      DESC_VER_2 },
    { 0, 0 }
};

/*
 * Create new nForce ethernet device unit
 */
struct NFUnit *CreateUnit(struct NFBase *NforceBase, OOP_Object *pciDevice)
{
    struct NFUnit *unit = AllocMem(sizeof(struct NFUnit), MEMF_PUBLIC | MEMF_CLEAR);
    BOOL success = TRUE;
    int i;

    if (unit != NULL)
    {
        IPTR        DeviceID, base, len;
        OOP_Object  *driver;

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);

        for (i=0; Config[i].ProductID; i++)
        {
            if (Config[i].ProductID == DeviceID)
            {
                unit->nu_DriverFlags = Config[i].DriverFlags;
                unit->nu_fe_priv->desc_ver = Config[i].DescVer;
                break;
            }
        }

        unit->nu_device     = NforceBase;
        unit->nu_DeviceID   = DeviceID;
        unit->mtu           = 1500;
        unit->nu_PCIDevice  = pciDevice;
        unit->nu_PCIDriver  = driver;

        InitSemaphore(&unit->unit_lock);
        NEWLIST(&unit->nu_Openers);
        NEWLIST(&unit->multicast_ranges);
        NEWLIST(&unit->type_trackers);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &unit->nu_IRQ);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base1,   &unit->nu_BaseIO);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0,   &base);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0,   &len);

        unit->nu_BaseMem = (IPTR)HIDD_PCIDriver_MapPCI(driver, (APTR)base, len);
        unit->nu_SizeMem = len;

        if (unit->nu_BaseMem)
        {
            struct TagItem attrs[] = {
                { aHidd_PCIDevice_isIO,     TRUE },
                { aHidd_PCIDevice_isMEM,    TRUE },
                { aHidd_PCIDevice_isMaster, TRUE },
                { TAG_DONE,                 0    },
            };
            OOP_SetAttrs(pciDevice, (struct TagItem *)&attrs);

            unit->name = "[nforce0]";
            unit->nu_fe_priv = AllocMem(sizeof(struct fe_priv), MEMF_PUBLIC|MEMF_CLEAR);
            unit->nu_UnitNum = 0;

            nv_get_functions(unit);

            if (unit->nu_fe_priv)
            {
                unit->nu_fe_priv->pci_dev = unit;
                InitSemaphore(&unit->nu_fe_priv->lock);

                unit->nu_irqhandler = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC|MEMF_CLEAR);
                unit->nu_touthandler = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC|MEMF_CLEAR);

                if (unit->nu_irqhandler && unit->nu_touthandler)
                {
                    struct Message *msg;

                    unit->nu_irqhandler->h_Node.ln_Pri = 100;
                    unit->nu_irqhandler->h_Node.ln_Name = LIBBASE->nf_Device.dd_Library.lib_Node.ln_Name;
                    unit->nu_irqhandler->h_Code = NF_IntHandler;
                    unit->nu_irqhandler->h_Data = unit;

                    unit->nu_touthandler->h_Node.ln_Pri = 100;
                    unit->nu_touthandler->h_Node.ln_Name = LIBBASE->nf_Device.dd_Library.lib_Node.ln_Name;
                    unit->nu_touthandler->h_Code = NF_TimeoutHandler;
                    unit->nu_touthandler->h_Data = unit;

                    unit->rx_int.is_Node.ln_Name = unit->name;
                    unit->rx_int.is_Code = RX_Int;
                    unit->rx_int.is_Data = unit;

                    unit->tx_int.is_Node.ln_Name = unit->name;
                    unit->tx_int.is_Code = TX_Int;
                    unit->tx_int.is_Data = unit;

                    unit->tx_end_int.is_Node.ln_Name = unit->name;
                    unit->tx_end_int.is_Code = TX_End_Int;
                    unit->tx_end_int.is_Data = unit;

                    for (i=0; i < REQUEST_QUEUE_COUNT; i++)
                    {
                        struct MsgPort *port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);
                        unit->request_ports[i] = port;

                        if (port == NULL) success = FALSE;

                        if (success)
                        {
                            NEWLIST(&port->mp_MsgList);
                            port->mp_Flags = PA_IGNORE;
                            port->mp_SigTask = &unit->tx_int;
                        }
                    }

                    unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

                    if (success)
                    {
                        LIBBASE->nf_syncport = CreateMsgPort();

                        unit->nu_Process = CreateNewProcTags(
                                                NP_Entry, (IPTR)NF_Scheduler,
                                                NP_Name, NFORCE_TASK_NAME,
                                                NP_Priority, 0,
                                                NP_UserData, (IPTR)unit,
                                                NP_StackSize, 140960,
                                                TAG_DONE);

                        WaitPort(LIBBASE->nf_syncport);
                        msg = GetMsg(LIBBASE->nf_syncport);
                        ReplyMsg(msg);
                        DeleteMsgPort(LIBBASE->nf_syncport);

                        D(bug("[nforce] Unit up and running\n"));

                        return unit;
                    }
                }
            }
        }
        else
            D(bug("[nforce] PANIC! Couldn't get MMIO area. Aborting\n"));
    }

    DeleteUnit(NforceBase, unit);	
    return NULL;
}

/*
 * DeleteUnit - removes selected unit. Frees all resources and structures.
 * 
 * The caller should be sure, that given unit is really ready to be freed.
 */

void DeleteUnit(struct NFBase *NforceBase, struct NFUnit *Unit)
{
    int i;
    if (Unit)
    {
        if (Unit->nu_Process)
        {
            Signal(&Unit->nu_Process->pr_Task, Unit->nu_signal_0);
        }

        for (i=0; i < REQUEST_QUEUE_COUNT; i++)
        {
            if (Unit->request_ports[i] != NULL) 
                FreeMem(Unit->request_ports[i],	sizeof(struct MsgPort));

            Unit->request_ports[i] = NULL;
        }

        if (Unit->nu_irqhandler)
        {
            FreeMem(Unit->nu_irqhandler, sizeof(HIDDT_IRQ_Handler));
            LIBBASE->nf_irq = NULL;
        }

        if (Unit->nu_fe_priv)
        {
            FreeMem(Unit->nu_fe_priv, sizeof(struct fe_priv));
            Unit->nu_fe_priv = NULL;
        }

        if (Unit->nu_BaseMem)
        {
            HIDD_PCIDriver_UnmapPCI(Unit->nu_PCIDriver, 
                                    (APTR)Unit->nu_BaseMem,
                                    Unit->nu_SizeMem);
        }

        FreeMem(Unit, sizeof(struct NFUnit));
    }
}

static struct AddressRange *FindMulticastRange(LIBBASETYPEPTR LIBBASE, struct NFUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left, UWORD upper_bound_right)
{
    struct AddressRange *range, *tail;
    BOOL found = FALSE;

    range = (APTR)unit->multicast_ranges.mlh_Head;
    tail = (APTR)&unit->multicast_ranges.mlh_Tail;

    while((range != tail) && !found)
    {
        if((lower_bound_left == range->lower_bound_left) &&
            (lower_bound_right == range->lower_bound_right) &&
            (upper_bound_left == range->upper_bound_left) &&
            (upper_bound_right == range->upper_bound_right))
            found = TRUE;
        else
            range = (APTR)range->node.mln_Succ;
    }

    if(!found)
        range = NULL;

    return range;
}

BOOL AddMulticastRange(LIBBASETYPEPTR LIBBASE, struct NFUnit *unit, const UBYTE *lower_bound,
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

    if(range != NULL)
        range->add_count++;
    else
    {
        range = AllocMem(sizeof(struct AddressRange), MEMF_PUBLIC);
        if(range != NULL)
        {
            range->lower_bound_left = lower_bound_left;
            range->lower_bound_right = lower_bound_right;
            range->upper_bound_left = upper_bound_left;
            range->upper_bound_right = upper_bound_right;
            range->add_count = 1;

            Disable();
            AddTail((APTR)&unit->multicast_ranges, (APTR)range);
            Enable();

            if (unit->range_count++ == 0)
            {
                unit->flags |= IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }

    return range != NULL;
}

BOOL RemMulticastRange(LIBBASETYPEPTR LIBBASE, struct NFUnit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound)
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

    if(range != NULL)
    {
        if(--range->add_count == 0)
        {
            Disable();
            Remove((APTR)range);
            Enable();
            FreeMem(range, sizeof(struct AddressRange));

            if (--unit->range_count == 0)
            {
                unit->flags &= ~IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }
    return range != NULL;
}

