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

#include <string.h>

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

#include "rtl8139.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/*
 * Report incoming events to all hyphotetical event receivers
 */
VOID ReportEvents(struct RTL8139Base *RTL8139DeviceBase, struct RTL8139Unit *unit, ULONG events)
{
	struct IOSana2Req *request, *tail, *next_request;
	struct List *list;

	list = &unit->rtl8139u_request_ports[EVENT_QUEUE]->mp_MsgList;
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

struct TypeStats *FindTypeStats(struct RTL8139Base *RTL8139DeviceBase, struct RTL8139Unit *unit, 
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

void FlushUnit(LIBBASETYPEPTR LIBBASE, struct RTL8139Unit *unit, UBYTE last_queue, BYTE error)
{
	struct IORequest *request;
	UBYTE i;
	struct Opener *opener, *tail;

RTLD(bug("[%s] unit.FlushUnit\n", unit->rtl8139u_name))

	/* Abort queued operations */

	for (i=0; i <= last_queue; i++)
	{
		while ((request = (APTR)GetMsg(unit->rtl8139u_request_ports[i])) != NULL)
		{
			request->io_Error = IOERR_ABORTED;
			ReplyMsg((struct Message *)request);
		}
	}

	opener = (APTR)unit->rtl8139u_Openers.mlh_Head;
	tail = (APTR)unit->rtl8139u_Openers.mlh_Tail;

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

/* Interrupt Rx Support Function ..
 * It's duty is to iterate throgh RX queue searching for new packets.
 */
void RTL8139_RX_Process(struct RTL8139Unit *unit)
{
	struct RTL8139Base *RTL8139DeviceBase = unit->rtl8139u_device;
	struct fe_priv *np = unit->rtl8139u_fe_priv;
	UBYTE *base = unit->rtl8139u_BaseMem;

	struct TypeStats *tracker;
	ULONG packet_type;
	struct Opener *opener, *opener_tail;
	struct IOSana2Req *request, *request_tail;
	BOOL accepted, is_orphan;

RTLD(bug("[%s] RTL8139_RX_Process() !!!!\n", unit->rtl8139u_name))

	while((BYTEIN(base + RTLr_ChipCmd) & RxBufEmpty) == 0)
	{
		UWORD len = 0, overspill = 0;
		struct eth_frame *frame;

		unsigned short cur_rx = np->cur_rx;
		unsigned int ring_offset = cur_rx % np->rx_buf_len;
		unsigned long rx_status = *(unsigned long *)(np->rx_buffer + ring_offset);
		unsigned int rx_size = rx_status >> 16;

		np->cur_rx = (unsigned short)(cur_rx + rx_size + ETH_CRCSIZE + 3) & ~3;
		WORDOUT(base + RTLr_RxBufPtr, np->cur_rx - 16);

RTLD(bug("[%s] RTL8139_RX_Process: RecieveBuffers  @ 0x%p\n",
			unit->rtl8139u_name, np->rx_buffer))
		
RTLD(bug("[%s] RTL8139_RX_Process: Rx = %d [offset=%4.4x, Status=%8.8x Size=%d]\n",
			unit->rtl8139u_name, cur_rx, ring_offset, rx_status, rx_size))

		if (rx_status & (RxBadSymbol | RxRunt | RxTooLong | RxCRCErr | RxBadAlign))
		{
RTLD(bug("[%s] RTL8139_RX_Process: Ethernet frame had errors, Status %8.8x\n",
				unit->rtl8139u_name, rx_status))

			if (rx_status == 0xffffffff)
			{
RTLD(bug("[%s] RTL8139_RX_Process: Invalid Recieve Status\n", unit->rtl8139u_name))
				rx_status = 0;
			}

			if (rx_status & RxTooLong)
			{
RTLD(bug("[%s] RTL8139_RX_Process: Oversized Ethernet Frame\n", unit->rtl8139u_name))
			}

			/* Reset the reciever */
			np->cur_rx = 0;
			BYTEOUT(base + RTLr_ChipCmd, CmdTxEnb);

			rtl8139nic_set_rxmode(unit);
			BYTEOUT(base + RTLr_ChipCmd, CmdRxEnb | CmdTxEnb);
		}
		else if (rx_status & RxStatusOK)
		{
			len = rx_size - ETH_CRCSIZE;
			frame = (APTR)(np->rx_buffer + ring_offset + ETH_CRCSIZE);
RTLD(bug("[%s] RTL8139_RX_Process: frame @ %p, len=%d\n", unit->rtl8139u_name, frame, len))

			/* got a valid packet - forward it to the network core */
			is_orphan = TRUE;

			if (ring_offset + rx_size > np->rx_buf_len)
			{
				overspill = (ring_offset + rx_size) - np->rx_buf_len;
RTLD(bug("[%s] RTL8139_RX_Process: WRAPPED Frame! (%d bytes overspill)\n", unit->rtl8139u_name, overspill))
				len = len - overspill;
/* TODO: We need to copy the wrapped buffer into a temp buff to pass to listeners! */
			}
			
			RTLD( int j;
				for (j=0; j<64; j++) {
					if ((j%16) == 0)
						bug("\n%03x:", j);

					bug(" %02x", ((unsigned char*)frame)[j]);
				}
				bug("\n");)

			/* Check for address validity */
			if(AddressFilter(LIBBASE, unit, frame->eth_packet_dest))
			{
				/* Packet is addressed to this driver */
				packet_type = AROS_BE2WORD(frame->eth_packet_type);
RTLD(bug("[%s] RTL8139_RX_Process: Packet IP accepted with type = %d\n", unit->rtl8139u_name, packet_type))

				opener = (APTR)unit->rtl8139u_Openers.mlh_Head;
				opener_tail = (APTR)&unit->rtl8139u_Openers.mlh_Tail;

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
RTLD(bug("[%s] RTL8139_RX_Process: copy packet for opener ..\n", unit->rtl8139u_name))
							CopyPacket(LIBBASE, unit, request, len, packet_type, frame);
							accepted = TRUE;
						}
						request = (struct IOSana2Req *)request->ios2_Req.io_Message.mn_Node.ln_Succ;
					}

					if(accepted)
					{
						is_orphan = FALSE;
					}

					opener = (APTR)opener->node.mln_Succ;
				}

				/* If packet was unwanted, give it to S2_READORPHAN request */
				if(is_orphan)
				{
					unit->rtl8139u_stats.UnknownTypesReceived++;

					if(!IsMsgPortEmpty(unit->rtl8139u_request_ports[ADOPT_QUEUE]))
					{
						CopyPacket(LIBBASE, unit,
						  (APTR)unit->rtl8139u_request_ports[ADOPT_QUEUE]->
						  mp_MsgList.lh_Head, len, packet_type, frame);
RTLD(bug("[%s] RTL8139_RX_Process: packet copied to orphan queue\n", unit->rtl8139u_name))
					}
				}

				/* Update remaining statistics */
				tracker =  FindTypeStats(LIBBASE, unit, &unit->rtl8139u_type_trackers, packet_type);

				if(tracker != NULL)
				{
					tracker->stats.PacketsReceived++;
					tracker->stats.BytesReceived += len;
				}
			}
			unit->rtl8139u_stats.PacketsReceived++;
		}
		else
		{
RTLD(bug("[%s] RTL8139_RX_Process: Rx Packet Processing complete\n", unit->rtl8139u_name))
		}
	}
}

/*
 * Interrupt generated by Cause() to push new packets into the NIC interface
 */
static AROS_INTH1(RTL8139_TX_IntF, struct RTL8139Unit *,  unit)
{
	AROS_INTFUNC_INIT

	struct fe_priv *np = unit->rtl8139u_fe_priv;
	struct RTL8139Base *RTL8139DeviceBase = unit->rtl8139u_device;
	int nr, try_count=1;
	BOOL proceed = FALSE; /* Fails by default */

	Disable();
	RTLD(bug("[%s] RTL8139_TX_IntF()\n", unit->rtl8139u_name))

	/* send packet only if there is free space on tx queue. Otherwise do nothing */
	if (np->tx_current - np->tx_dirty < NUM_TX_DESC)
	{
		UWORD packet_size, data_size;
		struct IOSana2Req *request;
		struct Opener *opener;
		UBYTE *buffer;
		ULONG wire_error=0;
		BYTE error;
		struct MsgPort *port;
		struct TypeStats *tracker;

		proceed = TRUE; /* Success by default */
		UBYTE *base = (UBYTE*) unit->rtl8139u_BaseMem;
		port = unit->rtl8139u_request_ports[WRITE_QUEUE];

		/* Still no error and there are packets to be sent? */
		while(proceed && (!IsMsgPortEmpty(port)))
		{
			nr = np->tx_current % NUM_TX_DESC;
			error = 0;

			request = (APTR)port->mp_MsgList.lh_Head;
			data_size = packet_size = request->ios2_DataLength;

			opener = (APTR)request->ios2_BufferManagement;

			np->tx_pbuf[nr] = np->tx_buf[nr];
			if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
			{
				packet_size += ETH_PACKET_DATA;
				CopyMem(request->ios2_DstAddr,
						&((struct eth_frame *) np->tx_buf[nr])->eth_packet_dest,
						ETH_ADDRESSSIZE);
				CopyMem(unit->rtl8139u_dev_addr,
						&((struct eth_frame *) np->tx_buf[nr])->eth_packet_source,
						ETH_ADDRESSSIZE);
				((struct eth_frame *)np->tx_buf[nr])->eth_packet_type = AROS_WORD2BE(request->ios2_PacketType);

				buffer = (UBYTE *)&((struct eth_frame *) (IPTR) np->tx_buf[nr])->eth_packet_data;
			}
			else
			{
				buffer = np->tx_buf[nr];
			}

			if (packet_size < TX_BUF_SIZE)
			{
				memset(buffer, 0, TX_BUF_SIZE - packet_size);
			}

			if (!opener->tx_function(buffer, request->ios2_Data, data_size))
			{
				error = S2ERR_NO_RESOURCES;
				wire_error = S2WERR_BUFF_ERROR;
				ReportEvents(LIBBASE, unit,
							 S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF |
							 S2EVENT_TX);
			}

			/* Now the packet is already in TX buffer, update flags for NIC */
			if (error == 0)
			{
#ifdef DEBUG
				Disable();
RTLD(bug("[%s] RTL8139_TX_IntF: packet %d  @ %p [type = %d] queued for transmission.", unit->rtl8139u_name, nr, np->tx_buf[nr], AROS_BE2WORD(((struct eth_frame *)np->tx_buf[nr])->eth_packet_type)))

				RTLD( int j;
					for (j=0; j<64; j++) {
						if ((j%16) == 0)
							bug("\n%03x:", j);
						bug(" %02x", ((unsigned char*)np->tx_buf[nr])[j]);
					}
					bug("\n");)

				Enable();
#endif

				/* Set the ring details for the packet */
				LONGOUT(base + RTLr_TxAddr0 + (nr << 2), (IPTR)np->tx_buf[nr]);
				LONGOUT(base + RTLr_TxStatus0 + (nr << 2), np->tx_flag |
														   (packet_size >= ETH_ZLEN ?
														    packet_size : ETH_ZLEN));
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
				unit->rtl8139u_stats.PacketsSent++;

				tracker = FindTypeStats(LIBBASE, unit, &unit->rtl8139u_type_trackers, request->ios2_PacketType);

				if(tracker != NULL)
				{
					tracker->stats.PacketsSent++;
					tracker->stats.BytesSent += packet_size;
				}
			}
			try_count = 0;
			np->tx_current++;

			/*
			* If we've just run out of free space on the TX queue, stop
			* it and give up pushing further frames */
			if (np->tx_current - np->tx_dirty >= NUM_TX_DESC)
			{
RTLD(bug("[%s] output queue full!. Stopping [count = %d, NUM_TX_DESC = %d\n", unit->rtl8139u_name, try_count, NUM_TX_DESC))
				proceed = FALSE;
			}
		} /* while */
	}

	/* Was there success? Enable incoming of new packets */    
	if(proceed)
	{
		unit->rtl8139u_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
	}
	else
	{
		unit->rtl8139u_request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;
	}
	Enable();

	return FALSE;

	AROS_INTFUNC_EXIT
}

/*
 * Handle timeouts and other strange cases
 */
static AROS_INTH1(RTL8139_TimeoutHandlerF,struct RTL8139Unit *, unit)
{
    AROS_INTFUNC_INIT

	struct timeval time;
	struct Device *TimerBase = unit->rtl8139u_TimerSlowReq->tr_node.io_Device;

	GetSysTime(&time);
	//RTLD(bug("[%s] RTL8139_TimeoutHandlerF()\n", unit->rtl8139u_name))

	/*
	* If timeout timer is expected, and time elapsed - regenerate the 
	* interrupt handler 
	*/
	if (unit->rtl8139u_toutNEED && (CmpTime(&time, &unit->rtl8139u_toutPOLL ) < 0))
	{
		unit->rtl8139u_toutNEED = FALSE;
		//Cause(&unit->rtl8139u_tx_end_int);
	}

	return FALSE;

	AROS_INTFUNC_EXIT
}

/*
 * Interrupt handler called whenever RTL8139 NIC interface generates interrupt.
 * 
 * Please note, that although multicast support could be done on interface 
 * basis, it is done in this function as result of quick integration of both
 * the forcedeth driver (IFF_ALLMULTI flag) and etherlink3 driver (AddressMatch
 * filter function).
 */
static AROS_INTH1(RTL8139_IntHandlerF, struct RTL8139Unit *, unit)
{
    AROS_INTFUNC_INIT

	struct fe_priv *np = unit->rtl8139u_fe_priv;
	UBYTE *base = (UBYTE*) unit->rtl8139u_BaseMem;
	unsigned int link_changed, CSCRval;
	int interrupt_work = 20;

RTLD(bug("[%s] RTL8139_IntHandlerF()!!!!!!!\n", unit->rtl8139u_name))
	
	while (1)
	{
		UWORD status = WORDIN(base + RTLr_IntrStatus);

		if (status & RxUnderrun)
		{
			CSCRval = WORDIN(base + RTLr_CSCR);
			link_changed = (CSCRval & CSCR_LinkChangeBit);

RTLD(bug("[%s] RTL8139_IntHandlerF: Link Change : %d\n", unit->rtl8139u_name, link_changed))
/* TODO: Disable/Enable interface on link change */

			if (CSCRval & CSCR_LinkOKBit) {

RTLD(bug("[%s] RTL8139_IntHandlerF: Link Change : Link UP\n", unit->rtl8139u_name))

				BYTEOUT(base + RTLr_ChipCmd, CmdTxEnb);
				rtl8139nic_set_rxmode(unit); // Reset the multicast list
				BYTEOUT(base + RTLr_ChipCmd, CmdRxEnb | CmdTxEnb);
				unit->rtl8139u_flags |= IFF_UP;
			} else {

RTLD(bug("[%s] RTL8139_IntHandlerF: Link Change : Link DOWN\n", unit->rtl8139u_name))

				unit->rtl8139u_flags &= ~IFF_UP;
			}
		}

		WORDOUT(base + RTLr_IntrStatus, status);

		if ((status & (RxOK | RxErr | RxUnderrun | RxOverflow | RxFIFOOver | TxOK | TxErr | PCIErr | PCSTimeout)) == 0)
		{
RTLD(bug("[%s] RTL8139_IntHandlerF: No work to process..\n", unit->rtl8139u_name))
			break;
		}
		
		if (status & (RxOK | RxErr | RxUnderrun | RxOverflow | RxFIFOOver)) // Chipset has Received packet(s)
		{
RTLD(bug("[%s] RTL8139_IntHandlerF: Packet Reception Attempt detected!\n", unit->rtl8139u_name))
			RTL8139_RX_Process(unit);
		}

		if (status & (TxOK | TxErr)) // Chipset has attempted to Send packet(s)
		{
RTLD(bug("[%s] RTL8139_IntHandlerF: Packet Transmition Attempt detected!\n", unit->rtl8139u_name))
			unsigned int dirty_tx = np->tx_dirty;

			while (np->tx_current - dirty_tx > 0)
			{
				int entry = dirty_tx % NUM_TX_DESC;

					ULONG txstatus = LONGIN(base + RTLr_TxStatus0 + (entry << 2));

					// Still transmitting
					if (!(txstatus & (TxStatOK | TxUnderrun | TxAborted))) break;

					// N.B: TxCarrierLost is always asserted at 100mbps.
					if (txstatus & (TxOutOfWindow | TxAborted))
					{
RTLD(bug("[%s] RTL8139_IntHandlerF: Packet %d Transmition Error! Tx status %8.8x\n", unit->rtl8139u_name, entry, txstatus))
						
						if (txstatus & TxAborted)
						{
							LONGOUT(base + RTLr_TxConfig, TX_DMA_BURST << 8);
						}
					}
					else
					{
						if (txstatus & TxUnderrun)
						{
RTLD(bug("[%s] RTL8139_IntHandlerF: Packet %d Transmition Underrun Error! Adjusting flags\n", unit->rtl8139u_name, entry))
								
							if (np->tx_flag < 0x00300000)
							{
								np->tx_flag += 0x00020000;
							}
						}
					}
					np->tx_pbuf[entry] = NULL;
					dirty_tx++;
			}
			np->tx_dirty = dirty_tx;

			// Restart transmissions if they had stopped due to ring being full
			if(unit->rtl8139u_request_ports[WRITE_QUEUE]->mp_Flags == PA_IGNORE)
				Cause(&unit->rtl8139u_tx_int);
		}

		if (status & (PCIErr | PCSTimeout | TxUnderrun | RxOverflow | RxFIFOOver | TxErr | RxErr)) // Chipset has Reported an ERROR
		{
RTLD(bug("[%s] RTL8139_IntHandlerF: ERROR Detected\n", unit->rtl8139u_name))
			if (status == 0xffff)
			{
				break; // Missing Chip!
			}
		}

		if (--interrupt_work < 0)
		{
RTLD(bug("[%s] RTL8139_IntHandlerF: MAX interrupt work reached.\n", unit->rtl8139u_name))
			WORDOUT(base + RTLr_IntrStatus, 0xffff);
			break;
		}
	}

	return FALSE;

	AROS_INTFUNC_EXIT
}

VOID CopyPacket(struct RTL8139Base *RTL8139DeviceBase, struct RTL8139Unit *unit, 
	struct IOSana2Req *request, UWORD packet_size, UWORD packet_type,
	struct eth_frame *buffer)
{
	struct Opener *opener;
	BOOL filtered = FALSE;
	UBYTE *ptr;
	const UBYTE broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

RTLD(bug("[%s] CopyPacket(packet @ %x, len = %d)\n", unit->rtl8139u_name, buffer, packet_size))

	/* Set multicast and broadcast flags */

	request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
	if (memcmp(buffer->eth_packet_dest, broadcast, 6) == 0)
	{
	   request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
RTLD(bug("[%s] CopyPacket: BROADCAST Flag set\n", unit->rtl8139u_name))
	}
	else if((buffer->eth_packet_dest[0] & 0x1) != 0)
	{
	   request->ios2_Req.io_Flags |= SANA2IOF_MCAST;
RTLD(bug("[%s] CopyPacket: MULTICAST Flag set\n", unit->rtl8139u_name))
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

RTLD(bug("[%s] CopyPacket: packet @ %x (%d bytes)\n", unit->rtl8139u_name, ptr, packet_size))

	/* Filter packet */

	opener = request->ios2_BufferManagement;
	if((request->ios2_Req.io_Command == CMD_READ) &&
		(opener->filter_hook != NULL))
		if(!CallHookPkt(opener->filter_hook, request, ptr))
		{
RTLD(bug("[%s] CopyPacket: packet filtered\n", unit->rtl8139u_name))
			filtered = TRUE;
		}

	if(!filtered)
	{
		/* Copy packet into opener's buffer and reply packet */
RTLD(bug("[%s] CopyPacket: opener recieve packet .. ", unit->rtl8139u_name))
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
RTLD(bug("[%s] CopyPacket: opener notified.\n", unit->rtl8139u_name))
	}
}

BOOL AddressFilter(struct RTL8139Base *RTL8139DeviceBase, struct RTL8139Unit *unit, UBYTE *address)
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

		range = (APTR)unit->rtl8139u_multicast_ranges.mlh_Head;
		tail = (APTR)&unit->rtl8139u_multicast_ranges.mlh_Tail;
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
			unit->rtl8139u_special_stats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
	}
	return accept;
}

/*
 * Unit process
 */
AROS_UFH3(void, RTL8139_Schedular,
	AROS_UFHA(STRPTR,              argPtr, A0),
	AROS_UFHA(ULONG,               argSize, D0),
	AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
	AROS_USERFUNC_INIT

	struct Task				*taskSelf = FindTask(NULL);
    struct RTL8139Startup	*sm_UD = taskSelf->tc_UserData;
    struct RTL8139Unit		*unit = sm_UD->rtl8139sm_Unit;

	LIBBASETYPEPTR			LIBBASE = unit->rtl8139u_device;
	struct MsgPort 			*reply_port, *input;

RTLD(bug("[%s] RTL8139_Schedular()\n", taskSelf->tc_Node.ln_Name))
RTLD(bug("[%s] RTL8139_Schedular: Setting up device '%s'\n", taskSelf->tc_Node.ln_Name, unit->rtl8139u_name))

	if ((reply_port = CreateMsgPort()) == NULL)
	{
RTLD(bug("[%s] RTL8139_Schedular: Failed to create Reply message port\n", taskSelf->tc_Node.ln_Name))
	}

	if ((input = CreateMsgPort()) == NULL)
	{
RTLD(bug("[%s] RTL8139_Schedular: Failed to create Input message port\n", taskSelf->tc_Node.ln_Name))
	}

	unit->rtl8139u_input_port = input; 

	if ((unit->rtl8139u_TimerSlowPort = CreateMsgPort()) != NULL)
	{
		unit->rtl8139u_TimerSlowReq = (struct timerequest *)
			CreateIORequest((struct MsgPort *)unit->rtl8139u_TimerSlowPort, sizeof(struct timerequest));

		if (unit->rtl8139u_TimerSlowReq)
		{
			if (!OpenDevice("timer.device", UNIT_VBLANK,
				(struct IORequest *)unit->rtl8139u_TimerSlowReq, 0))
			{
				struct Message *msg = AllocVec(sizeof(struct Message), MEMF_PUBLIC|MEMF_CLEAR);
				ULONG sigset;

RTLD(bug("[%s] RTL8139_Schedular: Got VBLANK unit of timer.device\n", taskSelf->tc_Node.ln_Name))

				unit->initialize(unit);

				msg->mn_ReplyPort = reply_port;
				msg->mn_Length = sizeof(struct Message);

RTLD(bug("[%s] RTL8139_Schedular: Setup complete. Sending handshake\n", taskSelf->tc_Node.ln_Name))
				PutMsg(sm_UD->rtl8139sm_SyncPort, msg);
				WaitPort(reply_port);
				GetMsg(reply_port);

				FreeVec(msg);

RTLD(bug("[%s] RTL8139_Schedular: entering forever loop ... \n", taskSelf->tc_Node.ln_Name))

				unit->rtl8139u_signal_0 = AllocSignal(-1);
				unit->rtl8139u_signal_1 = AllocSignal(-1);
				unit->rtl8139u_signal_2 = AllocSignal(-1);
				unit->rtl8139u_signal_3 = AllocSignal(-1);

				sigset = 1 << input->mp_SigBit  |
						 1 << unit->rtl8139u_signal_0  |
						 1 << unit->rtl8139u_signal_1  |
						 1 << unit->rtl8139u_signal_2  |
						 1 << unit->rtl8139u_signal_3;
				for(;;)
				{	
					ULONG recvd = Wait(sigset);
					if (recvd & unit->rtl8139u_signal_0)
					{
						/*
						 * Shutdown process. Driver should close everything 
						 * already and waits for our process to complete. Free
						 * memory allocared here and kindly return.
						 */
						unit->deinitialize(unit);
						CloseDevice((struct IORequest *)unit->rtl8139u_TimerSlowReq);
						DeleteIORequest((struct IORequest *)unit->rtl8139u_TimerSlowReq);
						DeleteMsgPort(unit->rtl8139u_TimerSlowPort);
						DeleteMsgPort(input);
						DeleteMsgPort(reply_port);

RTLD(bug("[%s] RTL8139_Schedular: Process shutdown.\n", taskSelf->tc_Node.ln_Name))
						return;
					}
					else if (recvd & (1 << input->mp_SigBit))
					{
						struct IOSana2Req *io;

						/* Handle incoming transactions */
						while ((io = (struct IOSana2Req *)GetMsg(input))!= NULL);
						{
RTLD(bug("[%s] RTL8139_Schedular: Handle incomming transaction.\n", taskSelf->tc_Node.ln_Name))
							ObtainSemaphore(&unit->rtl8139u_unit_lock);
							handle_request(LIBBASE, io);
						}
					}
					else
					{
RTLD(bug("[%s] RTL8139_Schedular: Handle incomming signal.\n", taskSelf->tc_Node.ln_Name))
						/* Handle incoming signals */
					}
				}
			}
		}
	}

	AROS_USERFUNC_EXIT
}

/*
 * Create new RTL8139 ethernet device unit
 */
struct RTL8139Unit *CreateUnit(struct RTL8139Base *RTL8139DeviceBase, OOP_Object *pciDevice, ULONG CardCapabilities, char * CardName, char * CardChipset)
{
	struct RTL8139Unit *unit;
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
		bug("[rtl8139] CreateUnit()\n");
	}

	if ((unit = AllocMem(sizeof(struct RTL8139Unit), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
	{
		IPTR        DeviceID, base, len;
		OOP_Object  *driver;

		if (doDebug)
			unit->rtl8139u_flags |= IFF_DEBUG;
		
		unit->rtl8139u_UnitNum = RTL8139DeviceBase->rtl8139b_UnitCount++;

		unit->rtl8139u_Sana2Info.HardwareType = S2WireType_Ethernet;
		unit->rtl8139u_Sana2Info.MTU = ETH_MTU;
		unit->rtl8139u_Sana2Info.AddrFieldSize = 8 * ETH_ADDRESSSIZE;

		if ((unit->rtl8139u_name = AllocVec(8 + (unit->rtl8139u_UnitNum/10) + 2, MEMF_CLEAR|MEMF_PUBLIC)) == NULL)
		{
            FreeMem(unit, sizeof(struct RTL8139Unit));
            return NULL;
		}
        sprintf((char *)unit->rtl8139u_name, "rtl8139.%d", (int)unit->rtl8139u_UnitNum);

RTLD(bug("[rtl8139] CreateUnit: Unit allocated @ 0x%p\n", unit))

		OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);
		OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);

		unit->rtl8139u_rtl_cardname = CardName;
		unit->rtl8139u_rtl_chipname = CardChipset;
		unit->rtl8139u_rtl_chipcapabilities = CardCapabilities;

		unit->rtl8139u_device     = RTL8139DeviceBase;
		unit->rtl8139u_DeviceID   = DeviceID;
		unit->rtl8139u_PCIDevice  = pciDevice;
		unit->rtl8139u_PCIDriver  = driver;

		unit->rtl8139u_mtu        = unit->rtl8139u_Sana2Info.MTU;

		InitSemaphore(&unit->rtl8139u_unit_lock);
		NEWLIST(&unit->rtl8139u_Openers);
		NEWLIST(&unit->rtl8139u_multicast_ranges);
		NEWLIST(&unit->rtl8139u_type_trackers);

		OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &unit->rtl8139u_IRQ);
		OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base1,   &unit->rtl8139u_BaseIO);
		OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0,   &base);
		OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0,   &len);

RTLD(bug("[%s] CreateUnit:   INT:%d, base1:0x%p, base0:0x%p, size0:%d\n", unit->rtl8139u_name,
																   unit->rtl8139u_IRQ, unit->rtl8139u_BaseIO,
																   base, len))

		unit->rtl8139u_BaseMem = HIDD_PCIDriver_MapPCI(driver, (APTR)base, len);
		unit->rtl8139u_SizeMem = len;

		if (unit->rtl8139u_BaseMem)
		{
			struct TagItem attrs[] = {
				{ aHidd_PCIDevice_isIO,     TRUE },
				{ aHidd_PCIDevice_isMEM,    TRUE },
				{ aHidd_PCIDevice_isMaster, TRUE },
				{ TAG_DONE,                 0    },
			};
			OOP_SetAttrs(pciDevice, (struct TagItem *)&attrs);

RTLD(bug("[%s] CreateUnit:   PCI_BaseMem @ 0x%p\n", unit->rtl8139u_name, unit->rtl8139u_BaseMem))

			unit->rtl8139u_fe_priv = AllocMem(sizeof(struct fe_priv), MEMF_PUBLIC|MEMF_CLEAR);

			rtl8139nic_get_functions(unit);

			if (unit->rtl8139u_fe_priv)
			{
				unit->rtl8139u_fe_priv->pci_dev = unit;
				InitSemaphore(&unit->rtl8139u_fe_priv->lock);

				{
					struct Message *msg;

					unit->rtl8139u_irqhandler.is_Node.ln_Type = NT_INTERRUPT;
					unit->rtl8139u_irqhandler.is_Node.ln_Pri = 100;
					unit->rtl8139u_irqhandler.is_Node.ln_Name = LIBBASE->rtl8139b_Device.dd_Library.lib_Node.ln_Name;
					unit->rtl8139u_irqhandler.is_Code = (VOID_FUNC)RTL8139_IntHandlerF;
					unit->rtl8139u_irqhandler.is_Data = unit;

					unit->rtl8139u_touthandler.is_Node.ln_Type = NT_INTERRUPT;
					unit->rtl8139u_touthandler.is_Node.ln_Pri = 100;
					unit->rtl8139u_touthandler.is_Node.ln_Name = LIBBASE->rtl8139b_Device.dd_Library.lib_Node.ln_Name;
					unit->rtl8139u_touthandler.is_Code = (VOID_FUNC)RTL8139_TimeoutHandlerF;
					unit->rtl8139u_touthandler.is_Data = unit;

					unit->rtl8139u_rx_int.is_Node.ln_Type = NT_INTERRUPT;
					unit->rtl8139u_rx_int.is_Node.ln_Name = unit->rtl8139u_name;
					//unit->rtl8139u_rx_int.is_Code = RTL8139_RX_IntF;
					unit->rtl8139u_rx_int.is_Data = unit;

					unit->rtl8139u_tx_int.is_Node.ln_Type = NT_INTERRUPT;
					unit->rtl8139u_tx_int.is_Node.ln_Name = unit->rtl8139u_name;
					unit->rtl8139u_tx_int.is_Code = (VOID_FUNC)RTL8139_TX_IntF;
					unit->rtl8139u_tx_int.is_Data = unit;

					for (i=0; i < REQUEST_QUEUE_COUNT; i++)
					{
						struct MsgPort *port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);
						unit->rtl8139u_request_ports[i] = port;

						if (port == NULL) success = FALSE;

						if (success)
						{
							NEWLIST(&port->mp_MsgList);
							port->mp_Flags = PA_IGNORE;
							port->mp_SigTask = &unit->rtl8139u_tx_int;
						}
					}

					unit->rtl8139u_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

					if (success)
					{
						struct RTL8139Startup *sm_UD;
						UBYTE tmpbuff[100];

						if ((sm_UD = AllocMem(sizeof(struct RTL8139Startup), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
						{
							sprintf((char *)tmpbuff, RTL8139_TASK_NAME, unit->rtl8139u_name);

							sm_UD->rtl8139sm_SyncPort = CreateMsgPort();
							sm_UD->rtl8139sm_Unit = unit;

							unit->rtl8139u_Process = CreateNewProcTags(
													NP_Entry, (IPTR)RTL8139_Schedular,
													NP_Name, tmpbuff,
													NP_Synchronous , FALSE,
													NP_Priority, 0,
													NP_UserData, (IPTR)sm_UD,
													NP_StackSize, 140960,
													TAG_DONE);

							WaitPort(sm_UD->rtl8139sm_SyncPort);
							msg = GetMsg(sm_UD->rtl8139sm_SyncPort);
							ReplyMsg(msg);
							DeleteMsgPort(sm_UD->rtl8139sm_SyncPort);
							FreeMem(sm_UD, sizeof(struct RTL8139Startup));

RTLD(bug("[%s]  CreateUnit: Device Initialised. Unit %d @ %p\n", unit->rtl8139u_name, unit->rtl8139u_UnitNum, unit))
							return unit;
						}
					}
					else
					{
RTLD(bug("[%s] ERRORS occured during Device setup - ABORTING\n", unit->rtl8139u_name))
					}
				}
			}
		}
		else
		{
RTLD(bug("[rtl8139] PANIC! Couldn't get MMIO area. Aborting\n"))
		}
	}
	else if (doDebug)
	{
		bug("[rtl8139] CreateUnit: Failed to Allocate Unit storage!\n");
		return NULL;
	}
	DeleteUnit(RTL8139DeviceBase, unit);	
	return NULL;
}

/*
 * DeleteUnit - removes selected unit. Frees all resources and structures.
 * 
 * The caller should be sure, that given unit is really ready to be freed.
 */

void DeleteUnit(struct RTL8139Base *RTL8139DeviceBase, struct RTL8139Unit *Unit)
{
	int i;
	if (Unit)
	{
		if (Unit->rtl8139u_Process)
		{
			Signal(&Unit->rtl8139u_Process->pr_Task, Unit->rtl8139u_signal_0);
		}

		for (i=0; i < REQUEST_QUEUE_COUNT; i++)
		{
			if (Unit->rtl8139u_request_ports[i] != NULL) 
				FreeMem(Unit->rtl8139u_request_ports[i],	sizeof(struct MsgPort));

			Unit->rtl8139u_request_ports[i] = NULL;
		}

		if (Unit->rtl8139u_fe_priv)
		{
			FreeMem(Unit->rtl8139u_fe_priv, sizeof(struct fe_priv));
			Unit->rtl8139u_fe_priv = NULL;
		}

		if (Unit->rtl8139u_BaseMem)
		{
			HIDD_PCIDriver_UnmapPCI(Unit->rtl8139u_PCIDriver, 
									(APTR)Unit->rtl8139u_BaseMem,
									Unit->rtl8139u_SizeMem);
		}

		FreeMem(Unit, sizeof(struct RTL8139Unit));
	}
}

static struct AddressRange *FindMulticastRange(LIBBASETYPEPTR LIBBASE, struct RTL8139Unit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left, UWORD upper_bound_right)
{
	struct AddressRange *range, *tail;
	BOOL found = FALSE;

	range = (APTR)unit->rtl8139u_multicast_ranges.mlh_Head;
	tail = (APTR)&unit->rtl8139u_multicast_ranges.mlh_Tail;

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

BOOL AddMulticastRange(LIBBASETYPEPTR LIBBASE, struct RTL8139Unit *unit, const UBYTE *lower_bound,
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
			AddTail((APTR)&unit->rtl8139u_multicast_ranges, (APTR)range);
			Enable();

			if (unit->rtl8139u_range_count++ == 0)
			{
				unit->rtl8139u_flags |= IFF_ALLMULTI;
				unit->set_multicast(unit);
			}
		}
	}

	return range != NULL;
}

BOOL RemMulticastRange(LIBBASETYPEPTR LIBBASE, struct RTL8139Unit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound)
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

			if (--unit->rtl8139u_range_count == 0)
			{
				unit->rtl8139u_flags &= ~IFF_ALLMULTI;
				unit->set_multicast(unit);
			}
		}
	}
	return range != NULL;
}
