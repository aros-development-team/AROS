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

#include "rtl8168.h"
#include "unit.h"
#include LC_LIBDEFS_FILE


extern UBYTE MMIO_R8(UBYTE *);
extern UWORD MMIO_R16(UWORD *);
extern ULONG MMIO_R32(ULONG *);
extern void MMIO_W8(UBYTE *, UBYTE);
extern void MMIO_W16(UWORD *, UWORD);
extern void MMIO_W32(ULONG *, ULONG);

extern void rtl8168_CheckLinkStatus(struct net_device *);

/*
 * Report incoming events to all hyphotetical event receivers
 */
VOID ReportEvents(struct RTL8168Base *RTL8168DeviceBase, struct RTL8168Unit *unit, ULONG events)
{
    struct IOSana2Req *request, *tail, *next_request;
    struct List *list;

    list = &unit->rtl8168u_request_ports[EVENT_QUEUE]->mp_MsgList;
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

struct TypeStats *FindTypeStats(struct RTL8168Base *RTL8168DeviceBase, struct RTL8168Unit *unit, 
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

void FlushUnit(LIBBASETYPEPTR LIBBASE, struct RTL8168Unit *unit, UBYTE last_queue, BYTE error)
{
    struct IORequest *request;
    UBYTE i;
    struct Opener *opener, *tail;

    RTLD(bug("[%s] unit.FlushUnit\n", unit->rtl8168u_name))

    /* Abort queued operations */
    for (i=0; i <= last_queue; i++)
    {
	while ((request = (APTR)GetMsg(unit->rtl8168u_request_ports[i])) != NULL)
	{
	    request->io_Error = IOERR_ABORTED;
	    ReplyMsg((struct Message *)request);
	}
    }

    opener = (APTR)unit->rtl8168u_Openers.mlh_Head;
    tail = (APTR)unit->rtl8168u_Openers.mlh_Tail;

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

static inline void rtl8168_MarkToASIC(struct RxDesc *desc, ULONG rx_buf_sz)
{
	ULONG eor = AROS_LE2LONG(desc->opts1) & RingEnd;

	desc->opts1 = AROS_LONG2LE(DescOwn | eor | rx_buf_sz);
}

/* Interrupt Rx Support Function ..
 * It's duty is to iterate throgh RX queue searching for new packets.
 */
void RTL8168_Rx_Process(struct RTL8168Unit *unit)
{
    struct RTL8168Base *RTL8168DeviceBase = unit->rtl8168u_device;
    struct rtl8168_priv *np = unit->rtl8168u_priv;
    // APTR base = unit->rtl8168u_BaseMem;

    struct TypeStats *tracker;
    ULONG packet_type;
    struct Opener *opener, *opener_tail;
    struct IOSana2Req *request, *request_tail;
    BOOL accepted, is_orphan;

    RTLD(bug("[%s] RTL8168_Rx_Process() !!!!\n", unit->rtl8168u_name))

    struct eth_frame *frame;

    unsigned short cur_rx, rx_left;
    cur_rx = np->cur_rx;
    rx_left = NUM_RX_DESC - np->dirty_rx;

    RTLD(bug("[%s] RTL8168_Rx_Process: cur_rx = %d, rx_left = %d, dirty = %d\n", unit->rtl8168u_name, cur_rx, rx_left, np->dirty_rx))

    for (; rx_left > 0; rx_left--, cur_rx++) {
	unsigned int entry = cur_rx % NUM_RX_DESC;
	struct RxDesc *desc = np->RxDescArray + entry;
	ULONG status;

	status = AROS_LE2LONG(desc->opts1);

	if (status & DescOwn)
	    break;

	RTLD(bug("[%s] RTL8168_Rx_Process: Packet %d\n", unit->rtl8168u_name, entry))

	if (status & RxRES) {
	    RTLD(bug("[%s] RTL8168_Rx_Process: Rx ERROR, status = %08x\n",
		    unit->rtl8168u_name, status))

/* TODO: record rx errors .. */
/*	    RTLDEV->stats.rx_errors++;

	    if (status & (RxRWT | RxRUNT))
		    RTLDEV->stats.rx_length_errors++;
	    if (status & RxCRC)
		    RTLDEV->stats.rx_crc_errors++;
*/
	    rtl8168_MarkToASIC(desc, np->rx_buf_sz);
	}
	else
	{
	    int pkt_size = (status & 0x00003FFF) - ETH_CRCSIZE;

	    frame = (APTR)(IPTR)desc->addr;
	    RTLD(bug("[%s] RTL8168_Rx_Process: frame @ %p, pkt_size=%d\n", unit->rtl8168u_name, frame, pkt_size))

	    /* got a valid packet - forward it to the network core */
	    is_orphan = TRUE;

	    RTLDP(
		    int j;
		    for (j = 0; j < pkt_size; j++) {
			if ((j%16) == 0)
			{
			    if (j != 0)
			    {
				bug("\n");
			    }
			    bug("[%s] RTL8168_Rx_Process: %03x:", unit->rtl8168u_name, j);
			}
			bug(" %02x", ((unsigned char*)frame)[j]);
		    }
		    bug("\n")
		)

	    /* Check for address validity */
	    if(AddressFilter(LIBBASE, unit, frame->eth_packet_dest))
	    {
		/* Packet is addressed to this driver */
		packet_type = AROS_BE2WORD(frame->eth_packet_type);
		RTLD(bug("[%s] RTL8168_Rx_Process: Packet IP accepted with type = %d\n", unit->rtl8168u_name, packet_type))

		opener = (APTR)unit->rtl8168u_Openers.mlh_Head;
		opener_tail = (APTR)&unit->rtl8168u_Openers.mlh_Tail;

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
			    RTLD(bug("[%s] RTL8168_Rx_Process: copy packet for opener ..\n", unit->rtl8168u_name))
			    CopyPacket(LIBBASE, unit, request, pkt_size, packet_type, frame);
			    accepted = TRUE;
			}
			request = (struct IOSana2Req *)request->ios2_Req.io_Message.mn_Node.ln_Succ;
		    }

		    if(accepted)
			    is_orphan = FALSE;

		    opener = (APTR)opener->node.mln_Succ;
		}

		/* If packet was unwanted, give it to S2_READORPHAN request */
		if(is_orphan)
		{
		    unit->rtl8168u_stats.UnknownTypesReceived++;

		    if(!IsMsgPortEmpty(unit->rtl8168u_request_ports[ADOPT_QUEUE]))
		    {
			CopyPacket(LIBBASE, unit,
			  (APTR)unit->rtl8168u_request_ports[ADOPT_QUEUE]->
			  mp_MsgList.lh_Head, pkt_size, packet_type, frame);
			RTLD(bug("[%s] RTL8168_Rx_Process: packet copied to orphan queue\n", unit->rtl8168u_name))
		    }
		}

		rtl8168_MarkToASIC(desc, (ULONG)np->rx_buf_sz);

		/* Update remaining statistics */
		tracker =  FindTypeStats(LIBBASE, unit, &unit->rtl8168u_type_trackers, packet_type);

		if(tracker != NULL)
		{
		    tracker->stats.PacketsReceived++;
		    tracker->stats.BytesReceived += pkt_size;
		}
	    }
	    unit->rtl8168u_stats.PacketsReceived++;
	}
    }
    np->cur_rx = cur_rx;
    RTLD(bug("[%s] RTL8168_Rx_Process: Rx Packet Processing complete\n", unit->rtl8168u_name))
}

/*
 * Interrupt generated by Cause() to push new packets into the NIC interface
 */
AROS_UFH3(void, RTL8168_TX_IntF,
	AROS_UFHA(struct RTL8168Unit *,  unit, A1),
	AROS_UFHA(APTR,             dummy, A5),
	AROS_UFHA(struct ExecBase *,SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct rtl8168_priv *np = unit->rtl8168u_priv;
    struct RTL8168Base *RTL8168DeviceBase = unit->rtl8168u_device;
    APTR  base = unit->rtl8168u_BaseMem;

    int nr, try_count=1;
    BOOL proceed = FALSE; /* Fails by default */

    RTLD(bug("[%s] RTL8168_TX_IntF()\n", unit->rtl8168u_name))

    /* send packet only if there is free space on tx queue. Otherwise do nothing */
    if (!netif_queue_stopped(unit))
    {
	UWORD packet_size, data_size;
	struct IOSana2Req *request;
	struct Opener *opener;
	UBYTE *buffer;
	ULONG wire_error=0;
	BYTE error;
	struct MsgPort *port;

	proceed = TRUE; /* Success by default */
	port = unit->rtl8168u_request_ports[WRITE_QUEUE];

	/* Still no error and there are packets to be sent? */
	while(proceed && (!IsMsgPortEmpty(port)))
	{
	    nr = np->cur_tx % NUM_TX_DESC;
	    error = 0;

	    request = (APTR)port->mp_MsgList.lh_Head;
	    data_size = packet_size = request->ios2_DataLength;

	    opener = (APTR)request->ios2_BufferManagement;

	    if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
	    {
		packet_size += ETH_PACKET_DATA;
	    }

	    if ((!(AROS_LE2LONG(np->TxDescArray[nr].opts1) & DescOwn)) &&
		((np->TxDescArray[nr].addr = (IPTR)AllocMem(packet_size, MEMF_CLEAR | MEMF_PUBLIC)) != 0))
	    {
		if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
		{
		    CopyMem(request->ios2_DstAddr, &((struct eth_frame *)(IPTR)np->TxDescArray[nr].addr)->eth_packet_dest, ETH_ADDRESSSIZE);
		    CopyMem(unit->rtl8168u_dev_addr, &((struct eth_frame *)(IPTR)np->TxDescArray[nr].addr)->eth_packet_source, ETH_ADDRESSSIZE);
		    ((struct eth_frame *)(IPTR)np->TxDescArray[nr].addr)->eth_packet_type = AROS_WORD2BE(request->ios2_PacketType);

		    buffer = (APTR)&((struct eth_frame *)(IPTR)np->TxDescArray[nr].addr)->eth_packet_data;
		}
		else
		    buffer = (APTR)(IPTR)np->TxDescArray[nr].addr;

		if (!opener->tx_function(buffer, request->ios2_Data, data_size))
		{
		    error = S2ERR_NO_RESOURCES;
		    wire_error = S2WERR_BUFF_ERROR;
		    ReportEvents(LIBBASE, unit,
		     S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF
		     | S2EVENT_TX);
		}

		// Now the packet is already in TX buffer, update flags for NIC
		if (error == 0)
		{
		    RTLD(
			    APTR packet = (APTR)(IPTR)np->TxDescArray[nr].addr;
			    bug("[%s] RTL8168_TX_IntF: packet %d  @ %p [type = %d] queued for transmission.\n", unit->rtl8168u_name, nr, (APTR)(IPTR)np->TxDescArray[nr].addr, AROS_BE2WORD(((struct eth_frame *)packet)->eth_packet_type))
			)

		    RTLDP(
			    int j;
			    for (j = 0; j < packet_size; j++) {
				if ((j%16) == 0)
				{
				    if (j != 0)
				    {
					bug("\n");
				    }
				    bug("[%s] RTL8168_TX_IntF: %03x:", unit->rtl8168u_name, j);
				}
				bug(" %02x", ((unsigned char*)np->TxDescArray[nr].addr)[j]);
			    }
			    bug("\n")
			)

		    // Set the ring details for the packet ..
		    np->TxDescArray[nr].opts1 = AROS_LONG2LE(DescOwn | FirstFrag | LastFrag | packet_size | (RingEnd * !((nr + 1) % NUM_TX_DESC)));
		    np->TxDescArray[nr].opts2 = AROS_LONG2LE(0);

/* TODO: Perhaps set the Tx Poll bit after we leave the while loop .. */
		    RTL_W8(base + (TxPoll), NPQ);	/* set polling bit */
		}

		// Reply packet
		request->ios2_Req.io_Error = error;
		request->ios2_WireError = wire_error;
		Disable();
		Remove((APTR)request);
		Enable();
		ReplyMsg((APTR)request);

		try_count = 0;
	    }
	    np->cur_tx++;
	    try_count++;

	    // If we've just run out of free space on the TX queue, stop
	    // it and give up pushing further frames
	    if ( (try_count + 1) >= NUM_TX_DESC)
	    {
RTLD(bug("[%s] output queue full!. Stopping [count = %d, NUM_TX_DESC = %d\n", unit->rtl8168u_name, try_count, NUM_TX_DESC))
		netif_stop_queue(unit);
		proceed = FALSE;
	    }
	}
    }

    /* Was there success? Enable incomming of new packets */    
    if(proceed)
	unit->rtl8168u_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
    else
	unit->rtl8168u_request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;

    AROS_USERFUNC_EXIT
}

/*
 * Handle timeouts and other strange cases
 */
static void RTL8168_TimeoutHandlerF(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct RTL8168Unit *unit = (struct RTL8168Unit *) irq->h_Data;
    struct timeval time;
    struct Device *TimerBase = unit->rtl8168u_TimerSlowReq->tr_node.io_Device;

    GetSysTime(&time);
    //RTLD(bug("[%s] RTL8168_TimeoutHandlerF()\n", unit->rtl8168u_name))

    /*
    * If timeout timer is expected, and time elapsed - regenerate the 
    * interrupt handler 
    */
    if (unit->rtl8168u_toutNEED && (CmpTime(&time, &unit->rtl8168u_toutPOLL ) < 0))
    {
	    unit->rtl8168u_toutNEED = FALSE;
	    //Cause(&unit->rtl8168u_tx_end_int);
    }
}

static void RTL8168_Tx_Cleanup(struct net_device *unit)
{
    struct rtl8168_priv *np = unit->rtl8168u_priv;
    unsigned int dirty_tx, tx_left;
    struct TypeStats *tracker;

    dirty_tx = np->dirty_tx;
    tx_left = np->cur_tx - dirty_tx;

    while (tx_left > 0) {
	unsigned int entry = dirty_tx % NUM_TX_DESC;
	ULONG packet_size, status;

	status = AROS_LE2LONG(np->TxDescArray[entry].opts1);

	if (status & DescOwn)
		break;

	packet_size = status & 0x3FFF;
	tracker = FindTypeStats(unit->rtl8168u_device, unit, &unit->rtl8168u_type_trackers, ((struct eth_frame *)(IPTR)np->TxDescArray[entry].addr)->eth_packet_type);
	if(tracker != NULL)
	{
		tracker->stats.PacketsSent++;
		tracker->stats.BytesSent += packet_size;
	}

	if (status & LastFrag) {
RTLD(bug("[%s] RTL8168_Tx_Cleanup: Released buffer %d (%d bytes)\n", unit->rtl8168u_name, entry, packet_size))
	    np->TxDescArray[entry].opts1 = AROS_LONG2LE(RingEnd);
	    np->TxDescArray[entry].addr = 0;
	}
	dirty_tx++;
	tx_left--;
    }
    if (np->dirty_tx != dirty_tx) {
	np->dirty_tx = dirty_tx;
/*
	if (netif_queue_stopped(dev) &&
	    (TX_BUFFS_AVAIL(np) >= MAX_SKB_FRAGS)) {
		netif_wake_queue(dev);
	}
*/
    }
}

/*
 * Interrupt handler called whenever RTL8168 NIC interface generates interrupt.
 */
static void RTL8168_IntHandlerF(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct RTL8168Unit *unit = (struct RTL8168Unit *) irq->h_Data;
    struct rtl8168_priv *np = unit->rtl8168u_priv;
    APTR base = unit->rtl8168u_BaseMem;
    int status;

    int boguscnt = unit->rtl8168u_device->rtl8168b_MaxIntWork;

    UWORD intr_clean_mask = SYSErr | PCSTimeout | SWInt | 
			  LinkChg | RxDescUnavail | 
			  TxErr | TxOK | RxErr | RxOK;

    RTL_W16(base + (IntrMask), 0x0000);

RTLD(bug("[%s] RTL8168_IntHandlerF()!!!!!!!\n", unit->rtl8168u_name))

    do {
	status = RTL_R16(base + (IntrStatus));

	/* hotplug/major error/no more work/shared irq */
	if ((status == 0xFFFF) || !status)
		break;

//	if (!netif_running(unit)) {
//		rtl8168_asic_down(unit);
//		goto out;
//	}

	status &= (np->intr_mask | TxDescUnavail);
	RTL_W16(base + (IntrStatus), intr_clean_mask);

	if (!(status & unit->rtl8168u_intr_mask))
		break;

	//Work around for rx fifo overflow
	if (status & RxFIFOOver)
	{
RTLD(bug("[%s] RTL8168_IntHandlerF: Rx FIFO overflow occured!\n", unit->rtl8168u_name))
	    if (np->mcfg == CFG_METHOD_1) {
		    /*np->rx_fifo_overflow = 1;
		    netif_stop_queue(unit);
		    udelay(300);
		    rtl8168_rx_clear(np);
		    rtl8168_init_ring(unit);
		    rtl8168_hw_start(unit);*/
		    RTL_W16(base + (IntrStatus), RxFIFOOver);
		    /*netif_wake_queue(unit);
		    np->rx_fifo_overflow = 0;*/
	    }
	}

	if (status & SYSErr) {
RTLD(bug("[%s] RTL8168_IntHandlerF: PCI error occured!\n", unit->rtl8168u_name))
//	    rtl8168_pcierr_interrupt(unit);
	    break;
	}

	if (status & LinkChg)
	{
RTLD(bug("[%s] RTL8168_IntHandlerF: Link Change!\n", unit->rtl8168u_name))
	    rtl8168_CheckLinkStatus(unit);
	}

	if ((status & TxOK) && (status & TxDescUnavail))
	{
	    RTL_W8(base + (TxPoll), NPQ);	/* set polling bit */
	    RTL_W16(base + (IntrStatus), TxDescUnavail);
	}
	/* Rx interrupt */
	if (status & (RxOK | RxDescUnavail | RxFIFOOver))
	{
RTLD(bug("[%s] RTL8168_IntHandlerF: Packet Reception detected!\n", unit->rtl8168u_name))
	    RTL8168_Rx_Process(unit);
//	    rtl8168_rx_interrupt(unit, np, np->mmio_addr, ~(u32)0);
	}
	/* Tx interrupt */
	if (status & (TxOK | TxErr))
	{
RTLD(bug("[%s] RTL8168_IntHandlerF: Packet Transmission detected!\n", unit->rtl8168u_name))
	    RTL8168_Tx_Cleanup(unit);
	}
	boguscnt--;
    } while (boguscnt > 0);

    if (boguscnt <= 0) {
RTLD(bug("[%s] RTL8168_IntHandlerF: Too much work at interrupt!\n", unit->rtl8168u_name))
	/* Clear all interrupt sources. */
	RTL_W16(base + (IntrStatus), 0xffff);
    }

    RTL_W16(base + (IntrMask), np->intr_mask);

    return;
}

VOID CopyPacket(struct RTL8168Base *RTL8168DeviceBase, struct RTL8168Unit *unit, 
	struct IOSana2Req *request, UWORD packet_size, UWORD packet_type,
	struct eth_frame *buffer)
{
    struct Opener *opener;
    BOOL filtered = FALSE;
    UBYTE *ptr;
    const UBYTE broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

RTLD(bug("[%s] CopyPacket(packet @ %x, len = %d)\n", unit->rtl8168u_name, buffer, packet_size))

    /* Set multicast and broadcast flags */

    request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
    if (memcmp(buffer->eth_packet_dest, broadcast, 6) == 0)
    {
       request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
RTLD(bug("[%s] CopyPacket: BROADCAST Flag set\n", unit->rtl8168u_name))
    }
    else if((buffer->eth_packet_dest[0] & 0x1) != 0)
    {
       request->ios2_Req.io_Flags |= SANA2IOF_MCAST;
RTLD(bug("[%s] CopyPacket: MULTICAST Flag set\n", unit->rtl8168u_name))
    }

    /* Set source and destination addresses and packet type */
    CopyMem(buffer->eth_packet_source, request->ios2_SrcAddr, ETH_ADDRESSSIZE);
    CopyMem(buffer->eth_packet_dest, request->ios2_DstAddr, ETH_ADDRESSSIZE);
    request->ios2_PacketType = packet_type;

    /* Adjust for cooked packet request */

    if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
    {
	packet_size -= ETH_PACKET_DATA;
	ptr = (UBYTE*)&buffer->eth_packet_data[0];
    }
    else
    {
	ptr = (UBYTE*)buffer;
    }

    request->ios2_DataLength = packet_size;

RTLD(bug("[%s] CopyPacket: packet @ %x (%d bytes)\n", unit->rtl8168u_name, ptr, packet_size))

    /* Filter packet */

    opener = request->ios2_BufferManagement;
    if((request->ios2_Req.io_Command == CMD_READ) &&
	    (opener->filter_hook != NULL))
	if(!CallHookPkt(opener->filter_hook, request, ptr))
	{
RTLD(bug("[%s] CopyPacket: packet filtered\n", unit->rtl8168u_name))
		filtered = TRUE;
	}

    if(!filtered)
    {
	    /* Copy packet into opener's buffer and reply packet */
RTLD(bug("[%s] CopyPacket: opener recieve packet .. ", unit->rtl8168u_name))
	if(!opener->rx_function(request->ios2_Data, ptr, packet_size))
	{
RTLD(bug("ERROR occured!!\n"))
		request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
		request->ios2_WireError = S2WERR_BUFF_ERROR;
		ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_RX);
	}
	else
	{
RTLD(bug("SUCCESS!!\n"))
	}
	Disable();
	Remove((APTR)request);
	Enable();
	ReplyMsg((APTR)request);
RTLD(bug("[%s] CopyPacket: opener notified.\n", unit->rtl8168u_name))
    }
}

BOOL AddressFilter(struct RTL8168Base *RTL8168DeviceBase, struct RTL8168Unit *unit, UBYTE *address)
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

	range = (APTR)unit->rtl8168u_multicast_ranges.mlh_Head;
	tail = (APTR)&unit->rtl8168u_multicast_ranges.mlh_Tail;
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
		unit->rtl8168u_special_stats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    }
    return accept;
}

/*
 * Unit process
 */
AROS_UFH3(void, RTL8168_Schedular,
	AROS_UFHA(STRPTR,              argPtr, A0),
	AROS_UFHA(ULONG,               argSize, D0),
	AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Task				*taskSelf = FindTask(NULL);
    struct RTL8168Startup		*sm_UD = taskSelf->tc_UserData;
    struct RTL8168Unit			*unit = sm_UD->rtl8168sm_Unit;

    LIBBASETYPEPTR			LIBBASE = unit->rtl8168u_device;
    struct MsgPort 			*reply_port, *input;

RTLD(bug("[%s] RTL8168_Schedular()\n", taskSelf->tc_Node.ln_Name))
RTLD(bug("[%s] RTL8168_Schedular: Setting up device '%s'\n", taskSelf->tc_Node.ln_Name, unit->rtl8168u_name))

    if ((reply_port = CreateMsgPort()) == NULL)
    {
RTLD(bug("[%s] RTL8168_Schedular: Failed to create Reply message port\n", taskSelf->tc_Node.ln_Name))
    }

    if ((input = CreateMsgPort()) == NULL)
    {
RTLD(bug("[%s] RTL8168_Schedular: Failed to create Input message port\n", taskSelf->tc_Node.ln_Name))
    }

    unit->rtl8168u_input_port = input; 

    if ((unit->rtl8168u_TimerSlowPort = CreateMsgPort()) != NULL)
    {
	unit->rtl8168u_TimerSlowReq = (struct timerequest *)
		CreateIORequest((struct MsgPort *)unit->rtl8168u_TimerSlowPort, sizeof(struct timerequest));

	if (unit->rtl8168u_TimerSlowReq)
	{
	    if (!OpenDevice("timer.device", UNIT_MICROHZ,
		(struct IORequest *)unit->rtl8168u_TimerSlowReq, 0))
	    {
		struct Message *msg = AllocVec(sizeof(struct Message), MEMF_PUBLIC|MEMF_CLEAR);
		ULONG sigset;

RTLD(bug("[%s] RTL8168_Schedular: Got MICROHZ unit of timer.device\n", taskSelf->tc_Node.ln_Name))

		unit->initialize(unit);

		msg->mn_ReplyPort = reply_port;
		msg->mn_Length = sizeof(struct Message);

RTLD(bug("[%s] RTL8168_Schedular: Setup complete. Sending handshake\n", taskSelf->tc_Node.ln_Name))
		PutMsg(sm_UD->rtl8168sm_SyncPort, msg);
		WaitPort(reply_port);
		GetMsg(reply_port);

		FreeVec(msg);

RTLD(bug("[%s] RTL8168_Schedular: entering forever loop ... \n", taskSelf->tc_Node.ln_Name))

		unit->rtl8168u_signal_0 = AllocSignal(-1);
		unit->rtl8168u_signal_1 = AllocSignal(-1);
		unit->rtl8168u_signal_2 = AllocSignal(-1);
		unit->rtl8168u_signal_3 = AllocSignal(-1);

		sigset = 1 << input->mp_SigBit  |
			 1 << unit->rtl8168u_signal_0  |
			 1 << unit->rtl8168u_signal_1  |
			 1 << unit->rtl8168u_signal_2  |
			 1 << unit->rtl8168u_signal_3;
		for(;;)
		{	
		    ULONG recvd = Wait(sigset);
		    if (recvd & unit->rtl8168u_signal_0)
		    {
			/*
			 * Shutdown process. Driver should close everything 
			 * already and waits for our process to complete. Free
			 * memory allocared here and kindly return.
			 */
			unit->deinitialize(unit);
			CloseDevice((struct IORequest *)unit->rtl8168u_TimerSlowReq);
			DeleteIORequest((struct IORequest *)unit->rtl8168u_TimerSlowReq);
			DeleteMsgPort(unit->rtl8168u_TimerSlowPort);
			DeleteMsgPort(input);
			DeleteMsgPort(reply_port);

RTLD(bug("[%s] RTL8168_Schedular: Process shutdown.\n", taskSelf->tc_Node.ln_Name))
			return;
		    }
		    else if (recvd & (1 << input->mp_SigBit))
		    {
			struct IOSana2Req *io;

			/* Handle incoming transactions */
			while ((io = (struct IOSana2Req *)GetMsg(input)) != NULL)
			{
RTLD(bug("[%s] RTL8168_Schedular: Handle incomming transaction.\n", taskSelf->tc_Node.ln_Name))
			    ObtainSemaphore(&unit->rtl8168u_unit_lock);
			    handle_request(LIBBASE, io);
			}
		    }
		    else
		    {
RTLD(bug("[%s] RTL8168_Schedular: Handle incomming signal.\n", taskSelf->tc_Node.ln_Name))
			/* Handle incoming signals */
		    }
		}
	    }
	}
    }

    AROS_USERFUNC_EXIT
}

static struct AddressRange *FindMulticastRange(LIBBASETYPEPTR LIBBASE, struct RTL8168Unit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left, UWORD upper_bound_right)
{
    struct AddressRange *range, *tail;
    BOOL found = FALSE;

    range = (APTR)unit->rtl8168u_multicast_ranges.mlh_Head;
    tail = (APTR)&unit->rtl8168u_multicast_ranges.mlh_Tail;

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

BOOL AddMulticastRange(LIBBASETYPEPTR LIBBASE, struct RTL8168Unit *unit, const UBYTE *lower_bound,
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
	    AddTail((APTR)&unit->rtl8168u_multicast_ranges, (APTR)range);
	    Enable();

	    if (unit->rtl8168u_range_count++ == 0)
	    {
		unit->rtl8168u_flags |= IFF_ALLMULTI;
		unit->set_multicast(unit);
	    }
	}
    }

    return range != NULL;
}

BOOL RemMulticastRange(LIBBASETYPEPTR LIBBASE, struct RTL8168Unit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound)
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

	    if (--unit->rtl8168u_range_count == 0)
	    {
		    unit->rtl8168u_flags &= ~IFF_ALLMULTI;
		    unit->set_multicast(unit);
	    }
	}
    }
    return range != NULL;
}

/*
 * Create new RTL8168 ethernet device unit
 */
struct RTL8168Unit *CreateUnit(struct RTL8168Base *RTL8168DeviceBase, OOP_Object *pciDevice, IPTR CardRevision)
{
    struct RTL8168Unit *unit;
    BOOL success = TRUE;
    int i;

#if defined(RTL_DEBUG)
    BOOL doDebug = TRUE;
#else
/* TODO: Get option to debug from somewhere .. */
    BOOL doDebug = FALSE;
#endif
	
    if (doDebug)
    {
	bug("[rtl8168] CreateUnit()\n");
    }

    if ((unit = AllocMem(sizeof(struct RTL8168Unit), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
    {
	IPTR		mmiobase, mmiolen, type;
	// IPTR		DeviceID;
	OOP_Object	*driver;
	BOOL		mmioerror = FALSE;

	if (doDebug)
	    unit->rtl8168u_flags |= IFF_DEBUG;
		
	unit->rtl8168u_UnitNum = RTL8168DeviceBase->rtl8168b_UnitCount++;

	unit->rtl8168u_Sana2Info.HardwareType = S2WireType_Ethernet;
	unit->rtl8168u_Sana2Info.MTU = ETH_MTU;
	unit->rtl8168u_Sana2Info.AddrFieldSize = 8 * ETH_ADDRESSSIZE;

	unit->rtl8168u_intr_mask = SYSErr | LinkChg | RxDescUnavail | TxErr | TxOK | RxErr | RxOK;

	if ((unit->rtl8168u_name = AllocVec(8 + (unit->rtl8168u_UnitNum/10) + 2, MEMF_CLEAR|MEMF_PUBLIC)) == NULL)
	{
            FreeMem(unit, sizeof(struct RTL8168Unit));
            return NULL;
	}
        sprintf((char *)unit->rtl8168u_name, "rtl8168.%d", unit->rtl8168u_UnitNum);

RTLD(bug("[rtl8168] CreateUnit: Unit allocated @ 0x%p\n", unit))

	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);

	unit->rtl8168u_device     = RTL8168DeviceBase;
	unit->rtl8168u_PCIDevice  = pciDevice;
	unit->rtl8168u_PCIDriver  = driver;

	unit->rtl8168u_mtu        = unit->rtl8168u_Sana2Info.MTU;

	InitSemaphore(&unit->rtl8168u_unit_lock);
	NEWLIST(&unit->rtl8168u_Openers);
	NEWLIST(&unit->rtl8168u_multicast_ranges);
	NEWLIST(&unit->rtl8168u_type_trackers);

	OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &unit->rtl8168u_IRQ);
	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, (IPTR *)&unit->rtl8168u_BaseIO);
	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base2, &mmiobase);
	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size2,  &mmiolen);
	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Type2,  &type);
	
RTLD(bug("[%s] CreateUnit:   INT:%d, base0:0x%p, base2:0x%p, size2:%d\n", unit->rtl8168u_name,
									    unit->rtl8168u_IRQ, unit->rtl8168u_BaseIO,
									    mmiobase, mmiolen))

	if (type & ADDRF_IO)
	{
RTLD(bug("[%s] CreateUnit: MMIO Region of wrong type!\n", unit->rtl8168u_name))
	    mmioerror = TRUE;
	}

	if (mmiolen < R8168_REGS_SIZE)
	{
RTLD(bug("[%s] CreateUnit: Invalid MMIO Reg size (%d, expected %d)\n", unit->rtl8168u_name,
									    mmiolen,
									    R8168_REGS_SIZE))
	    mmioerror = TRUE;
	}

	if (mmioerror)
	{
	    FreeMem(unit->rtl8168u_name, 8 + (unit->rtl8168u_UnitNum/10) + 2);
            FreeMem(unit, sizeof(struct RTL8168Unit));
            return NULL;
	}

/* TODO: how do we set memory write invalidate for PCI devices on AROS? */

	unit->rtl8168u_SizeMem = R8168_REGS_SIZE;
	unit->rtl8168u_BaseMem = HIDD_PCIDriver_MapPCI(driver, (APTR)mmiobase, unit->rtl8168u_SizeMem);

	if (unit->rtl8168u_BaseMem != NULL)
	{
	    struct TagItem attrs[] = {
		    { aHidd_PCIDevice_isIO,     TRUE },
		    { aHidd_PCIDevice_isMEM,    TRUE },
		    { aHidd_PCIDevice_isMaster, TRUE },
		    { TAG_DONE,                 0    },
	    };
	    OOP_SetAttrs(pciDevice, (struct TagItem *)&attrs);

RTLD(bug("[%s] CreateUnit:   PCI_BaseMem @ 0x%p\n", unit->rtl8168u_name, unit->rtl8168u_BaseMem))

            unit->rtl8168u_DelayPort.mp_SigBit = SIGB_SINGLE;
            unit->rtl8168u_DelayPort.mp_Flags = PA_SIGNAL;
            unit->rtl8168u_DelayPort.mp_SigTask = FindTask(NULL);
            unit->rtl8168u_DelayPort.mp_Node.ln_Type = NT_MSGPORT;
            NEWLIST(&unit->rtl8168u_DelayPort.mp_MsgList);

            unit->rtl8168u_DelayReq.tr_node.io_Message.mn_ReplyPort = &unit->rtl8168u_DelayPort;
            unit->rtl8168u_DelayReq.tr_node.io_Message.mn_Length = sizeof(struct timerequest);

            OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)&unit->rtl8168u_DelayReq, 0);

	    if ((unit->rtl8168u_priv = AllocMem(sizeof(struct rtl8168_priv), MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
	    {
		unit->rtl8168u_priv->pci_dev = unit;
		InitSemaphore(&unit->rtl8168u_priv->lock);

		unit->rtl8168u_irqhandler = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC|MEMF_CLEAR);
		unit->rtl8168u_touthandler = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC|MEMF_CLEAR);

		if ((unit->rtl8168u_irqhandler != NULL)
		    && (unit->rtl8168u_touthandler != NULL))
		{
		    struct Message *msg;

		    unit->rtl8168u_irqhandler->h_Node.ln_Pri = 100;
		    unit->rtl8168u_irqhandler->h_Node.ln_Name = LIBBASE->rtl8168b_Device.dd_Library.lib_Node.ln_Name;
		    unit->rtl8168u_irqhandler->h_Code = RTL8168_IntHandlerF;
		    unit->rtl8168u_irqhandler->h_Data = unit;

		    unit->rtl8168u_touthandler->h_Node.ln_Pri = 100;
		    unit->rtl8168u_touthandler->h_Node.ln_Name = LIBBASE->rtl8168b_Device.dd_Library.lib_Node.ln_Name;
		    unit->rtl8168u_touthandler->h_Code = RTL8168_TimeoutHandlerF;
		    unit->rtl8168u_touthandler->h_Data = unit;

		    unit->rtl8168u_rx_int.is_Node.ln_Name = unit->rtl8168u_name;
		    //unit->rtl8168u_rx_int.is_Code = RTL8168_RX_IntF;
		    unit->rtl8168u_rx_int.is_Data = unit;

		    unit->rtl8168u_tx_int.is_Node.ln_Name = unit->rtl8168u_name;
		    unit->rtl8168u_tx_int.is_Code = RTL8168_TX_IntF;
		    unit->rtl8168u_tx_int.is_Data = unit;

		    for (i=0; i < REQUEST_QUEUE_COUNT; i++)
		    {
			struct MsgPort *port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);
			unit->rtl8168u_request_ports[i] = port;

			if (port == NULL) success = FALSE;

			if (success)
			{
			    NEWLIST(&port->mp_MsgList);
			    port->mp_Flags = PA_IGNORE;
			    port->mp_SigTask = &unit->rtl8168u_tx_int;
			}
		    }

		    unit->rtl8168u_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

		    if (success)
		    {
			struct RTL8168Startup *sm_UD;
			UBYTE tmpbuff[100];

			if ((sm_UD = AllocMem(sizeof(struct RTL8168Startup), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
			{
			    sprintf((char *)tmpbuff, RTL8168_TASK_NAME, unit->rtl8168u_name);

			    sm_UD->rtl8168sm_SyncPort = CreateMsgPort();
			    sm_UD->rtl8168sm_Unit = unit;

			    rtl8168nic_get_functions(unit);

			    unit->rtl8168u_Process = CreateNewProcTags(
				NP_Entry, (IPTR)RTL8168_Schedular,
				NP_Name, tmpbuff,
				NP_Synchronous , FALSE,
				NP_Priority, 0,
				NP_UserData, (IPTR)sm_UD,
				NP_StackSize, 140960,
				TAG_DONE);

			    WaitPort(sm_UD->rtl8168sm_SyncPort);
			    msg = GetMsg(sm_UD->rtl8168sm_SyncPort);
			    ReplyMsg(msg);
			    DeleteMsgPort(sm_UD->rtl8168sm_SyncPort);
			    FreeMem(sm_UD, sizeof(struct RTL8168Startup));

RTLD(bug("[%s]  CreateUnit: Device Initialised. Unit %d @ %p\n", unit->rtl8168u_name, unit->rtl8168u_UnitNum, unit))
			    return unit;
			}
		    }
		    else
		    {
RTLD(bug("[%s] ERRORS occured during Device setup - ABORTING\n", unit->rtl8168u_name))
		    }
		}
	    }
	}
	else
	{
RTLD(bug("[rtl8168] PANIC! Couldn't get MMIO area. Aborting\n"))
	}
    }
    else if (doDebug)
    {
	bug("[rtl8168] CreateUnit: Failed to Allocate Unit storage!\n");
	return NULL;
    }
    DeleteUnit(RTL8168DeviceBase, unit);	
    return NULL;
}

/*
 * DeleteUnit - removes selected unit. Frees all resources and structures.
 * 
 * The caller should be sure, that given unit is really ready to be freed.
 */

void DeleteUnit(struct RTL8168Base *RTL8168DeviceBase, struct RTL8168Unit *Unit)
{
    int i;
    if (Unit)
    {
	if (Unit->rtl8168u_Process)
	{
	    Signal(&Unit->rtl8168u_Process->pr_Task, Unit->rtl8168u_signal_0);
	}

	for (i=0; i < REQUEST_QUEUE_COUNT; i++)
	{
	    if (Unit->rtl8168u_request_ports[i] != NULL) 
		FreeMem(Unit->rtl8168u_request_ports[i],	sizeof(struct MsgPort));

	    Unit->rtl8168u_request_ports[i] = NULL;
	}

	if (Unit->rtl8168u_irqhandler)
	{
	    FreeMem(Unit->rtl8168u_irqhandler, sizeof(HIDDT_IRQ_Handler));
	}

	if (Unit->rtl8168u_priv)
	{
	    FreeMem(Unit->rtl8168u_priv, sizeof(struct rtl8168_priv));
	    Unit->rtl8168u_priv = NULL;
	}

	if (Unit->rtl8168u_BaseMem)
	{
	    HIDD_PCIDriver_UnmapPCI(Unit->rtl8168u_PCIDriver, 
		(APTR)Unit->rtl8168u_BaseMem,
		Unit->rtl8168u_SizeMem);
	}

	FreeMem(Unit, sizeof(struct RTL8168Unit));
    }
}
