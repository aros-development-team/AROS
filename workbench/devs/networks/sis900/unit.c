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

#include "sis900.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/*
 * Report incoming events to all hyphotetical event receivers
 */
VOID ReportEvents(struct SiS900Base *SiS900DeviceBase, struct SiS900Unit *unit, ULONG events)
{
    struct IOSana2Req *request, *tail, *next_request;
    struct List *list;

    list = &unit->sis900u_request_ports[EVENT_QUEUE]->mp_MsgList;
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

struct TypeStats *FindTypeStats(struct SiS900Base *SiS900DeviceBase, struct SiS900Unit *unit, 
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

void FlushUnit(LIBBASETYPEPTR LIBBASE, struct SiS900Unit *unit, UBYTE last_queue, BYTE error)
{
    struct IORequest *request;
    UBYTE i;
    struct Opener *opener, *tail;

D(bug("%s unit.FlushUnit\n", unit->sis900u_name));

    /* Abort queued operations */

    for (i=0; i <= last_queue; i++)
    {
        while ((request = (APTR)GetMsg(unit->sis900u_request_ports[i])) != NULL)
        {
            request->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *)request);
        }
    }

    opener = (APTR)unit->sis900u_Openers.mlh_Head;
    tail = (APTR)unit->sis900u_Openers.mlh_Tail;

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

/*
 * Interrupt handler called whenever SiS900 NIC interface generates interrupt.
 * It's duty is to iterate throgh RX queue searching for new packets.
 * 
 * Please note, that allthough multicast support could be done on interface 
 * basis, it is done in this function as result of quick integration of both
 * the forcedeth driver (IFF_ALLMULTI flag) and etherling3 driver (AddressMatch
 * filter function).
 */
static AROS_INTH1(SiS900_RX_IntF, struct SiS900Unit *, unit)
{
    AROS_INTFUNC_INIT

    struct SiS900Base *SiS900DeviceBase = unit->sis900u_device;
    struct TypeStats *tracker;
    ULONG packet_type;
    struct Opener *opener, *opener_tail;
    struct IOSana2Req *request, *request_tail;
    BOOL accepted, is_orphan;

	unsigned int entry = unit->cur_rx % NUM_RX_DESC;
	ULONG rx_status = unit->rx_ring[entry].cmdsts;
	
D(bug("[%s]: SiS900_RX_IntF() !!!!\n", unit->sis900u_name));
D(bug("[%s]: SiS900_RX_IntF: cur_rx:%4.4d, dirty_rx:%4.4d status:0x%8.8x\n", unit->sis900u_name, unit->cur_rx, unit->dirty_rx, rx_status));

	while (rx_status & OWN)
	{
		unsigned int rx_size;
        // int i;
        struct eth_frame *frame;

		rx_size = (rx_status & DSIZE) - CRC_SIZE;

		if (rx_status & (ABORT|OVERRUN|TOOLONG|RUNT|RXISERR|CRCERR|FAERR))
		{
			/* corrupted packet received */
D(bug("[%s]: SiS900_RX_IntF: Corrupted packet received, buffer status = 0x%8.8x.\n", unit->sis900u_name, rx_status));
//			unit->stats.rx_errors++;
			if (rx_status & OVERRUN)
			{
//				unit->stats.rx_over_errors++;
			}
			if (rx_status & (TOOLONG|RUNT))
			{
//				unit->stats.rx_length_errors++;
			}
			if (rx_status & (RXISERR | FAERR))
			{
//				unit->stats.rx_frame_errors++;
			}
			if (rx_status & CRCERR) 
			{
//				unit->stats.rx_crc_errors++;
			}
			/* reset buffer descriptor state */
			unit->rx_ring[entry].cmdsts = RX_BUF_SIZE;
		}
		else
		{
			/* got a valid packet - forward it to the network core */
			frame = unit->rx_buffers[entry];
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
D(bug("[%s]: SiS900_RX_IntF: Packet IP accepted with type = %d\n", unit->sis900u_name, packet_type));

				opener = (APTR)unit->sis900u_Openers.mlh_Head;
				opener_tail = (APTR)&unit->sis900u_Openers.mlh_Tail;

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
D(bug("[%s]: SiS900_RX_IntF: copy packet for opener ..\n", unit->sis900u_name));
						 CopyPacket(LIBBASE, unit, request, rx_size, packet_type, frame);
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
					unit->sis900u_stats.UnknownTypesReceived++;

					if(!IsMsgPortEmpty(unit->sis900u_request_ports[ADOPT_QUEUE]))
					{
						CopyPacket(LIBBASE, unit,
							(APTR)unit->sis900u_request_ports[ADOPT_QUEUE]->
							mp_MsgList.lh_Head, rx_size, packet_type, frame);
D(bug("[%s]: SiS900_RX_IntF: packet copied to orphan queue\n", unit->sis900u_name));
					}
				}

				/* Update remaining statistics */

				tracker =
					FindTypeStats(LIBBASE, unit, &unit->sis900u_type_trackers, packet_type);

				if(tracker != NULL)
				{
					tracker->stats.PacketsReceived++;
					tracker->stats.BytesReceived += rx_size;
				}
			}
			unit->sis900u_stats.PacketsReceived++;

			unit->rx_ring[entry].cmdsts = RX_BUF_SIZE;
			unit->dirty_rx++;
        }
		unit->cur_rx++;
		entry = unit->cur_rx % NUM_RX_DESC;
		rx_status = unit->rx_ring[entry].cmdsts;
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * Interrupt generated by Cause() to push new packets into the NIC interface
 */
static AROS_INTH1(SiS900_TX_IntF, struct SiS900Unit *,unit)
{
    AROS_INTFUNC_INIT

    struct SiS900Base *SiS900DeviceBase = unit->sis900u_device;
	long ioaddr = unit->sis900u_BaseMem;
    BOOL proceed = FALSE; /* Fails by default */

	unsigned int  entry;
	unsigned int  index_cur_tx, index_dirty_tx;
	unsigned int  count_dirty_tx;
	
D(bug("[%s]: SiS900_TX_IntF()\n", unit->sis900u_name));

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
        struct TypeStats *tracker;

        proceed = TRUE; /* Success by default */
        port = unit->sis900u_request_ports[WRITE_QUEUE];

        /* Still no error and there are packets to be sent? */
        while(proceed && (!IsMsgPortEmpty(port)))
        {
			entry = unit->cur_tx % NUM_TX_DESC;
            error = 0;

			request = (APTR)port->mp_MsgList.lh_Head;
			data_size = packet_size = request->ios2_DataLength;

			opener = (APTR)request->ios2_BufferManagement;

			if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
			{
				packet_size += ETH_PACKET_DATA;
				CopyMem(request->ios2_DstAddr, &((struct eth_frame *)unit->tx_buffers[entry])->eth_packet_dest, ETH_ADDRESSSIZE);
				CopyMem(unit->sis900u_dev_addr, &((struct eth_frame *)unit->tx_buffers[entry])->eth_packet_source, ETH_ADDRESSSIZE);
				((struct eth_frame *)unit->tx_buffers[entry])->eth_packet_type = AROS_WORD2BE(request->ios2_PacketType);

				buffer = (UBYTE *)&((struct eth_frame *)(IPTR)unit->tx_buffers[entry])->eth_packet_data;
			}
			else
				buffer = unit->tx_buffers[entry];

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
D(bug("[%s]: SiS900_TX_IntF: packet %d  @ %x [type = %d] queued for transmission.", unit->sis900u_name, entry, unit->tx_buffers[entry], ((struct eth_frame *)unit->tx_buffers[entry])->eth_packet_type));

			  /* DEBUG? Dump frame if so */
#ifdef DEBUG
				{
					int j;
					for (j=0; j<64; j++) {
						if ((j%16) == 0)
							D(bug("\n%03x:", j));
						D(bug(" %02x", ((unsigned char*)unit->tx_buffers[entry])[j]));
					}
					D(bug("\n"));
				}
#endif

				Enable();

				/* Set the ring details for the packet .. */
				unit->tx_ring[entry].cmdsts = (OWN | packet_size);
				LONGOUT(ioaddr + cr, TxENA | LONGIN(ioaddr + cr));

				unit->cur_tx ++;
				index_cur_tx = unit->cur_tx;
				index_dirty_tx = unit->dirty_tx;

				for (count_dirty_tx = 0; index_cur_tx != index_dirty_tx; index_dirty_tx++)
					count_dirty_tx ++;

D(bug("[%s]: SiS900_TX_IntF: Packet Queued.\n", unit->sis900u_name));
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
				tracker = FindTypeStats(LIBBASE, unit, &unit->sis900u_type_trackers,
					request->ios2_PacketType);
				if(tracker != NULL)
				{
					tracker->stats.PacketsSent++;
					tracker->stats.BytesSent += packet_size;
				}
			}
        }
    }

    /* Was there success? Enable incomming of new packets */    
    if(proceed)
        unit->sis900u_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
    else
        unit->sis900u_request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * Handle timeouts and other strange cases
 */
static AROS_INTH1(SiS900_TimeoutHandlerF, struct SiS900Unit *, unit)
{
    AROS_INTFUNC_INIT

    // struct timeval time;
    // struct Device *TimerBase = unit->sis900u_TimerSlowReq->tr_node.io_Device;

//    GetSysTime(&time);
//D(bug("[%s]: SiS900_TimeoutHandlerF()\n", unit->sis900u_name));

    /*
     * If timeout timer is expected, and time elapsed - regenerate the 
     * interrupt handler 
     */
//    if (unit->sis900u_toutNEED && (CmpTime(&time, &unit->sis900u_toutPOLL ) < 0))
//    {
//        unit->sis900u_toutNEED = FALSE;
        //Cause(&unit->sis900u_tx_end_int);
//    }
    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * The interrupt handler - schedules code execution to proper handlers
 */
static AROS_INTH1(SiS900_IntHandlerF, struct SiS900Unit *, unit)
{
    AROS_INTFUNC_INIT

	long ioaddr = unit->sis900u_BaseMem;
    // ULONG events;
    int boguscnt = 20;
    // int i, link_changed;
	ULONG status;
    // struct Device *TimerBase = unit->sis900u_TimerSlowReq->tr_node.io_Device;
    // struct timeval time;

D(bug("[%s]: SiS900_IntHandlerF()!!!!!!!\n", unit->sis900u_name));

	do {
		status = LONGIN(ioaddr + isr);

		if ((status & (HIBERR|TxURN|TxERR|TxIDLE|RxORN|RxERR|RxOK)) == 0)
		{
			/* nothing intresting happened */
D(bug("[%s]: SiS900_IntHandlerF: Nothing for us ..\n", unit->sis900u_name));
			break;
		}

		/* why dow't we break after Tx/Rx case ?? keyword: full-duplex */
		if (status & (RxORN | RxERR | RxOK))
		{
D(bug("[%s]: SiS900_IntHandlerF: Rx Detected!\n", unit->sis900u_name));
			/* Rx interrupt */
			Cause(&unit->sis900u_rx_int);
		}

		if (status & (TxURN | TxERR | TxIDLE))
		{
D(bug("[%s]: SiS900_IntHandlerF: End of Tx Detected\n", unit->sis900u_name));
			/* Tx interrupt */
			for (; unit->dirty_tx != unit->cur_tx; unit->dirty_tx++) {
				unsigned int entry;
				ULONG tx_status;

				entry = unit->dirty_tx % NUM_TX_DESC;
				tx_status = unit->tx_ring[entry].cmdsts;

				if (tx_status & OWN) {
					/* The packet is not transmitted yet (owned by hardware) !
					 * Note: the interrupt is generated only when Tx Machine
					 * is idle, so this is an almost impossible case */
					break;
				}

				if (tx_status & (ABORT | UNDERRUN | OWCOLL)) {
					/* packet unsuccessfully transmitted */
D(bug("[%s]: SiS900_IntHandlerF: Transmit error, Tx status %8.8x.\n", unit->sis900u_name, tx_status));
//					unit->stats.tx_errors++;
					if (tx_status & UNDERRUN)
					{
//						unit->stats.tx_fifo_errors++;
					}
					if (tx_status & ABORT)
					{
//						unit->stats.tx_aborted_errors++;
					}
					if (tx_status & NOCARRIER)
					{
//						unit->stats.tx_carrier_errors++;
					}
					if (tx_status & OWCOLL)
					{
//						unit->stats.tx_window_errors++;
					}
				} else {
					/* packet successfully transmitted */
//					sis_priv->stats.collisions += (tx_status & COLCNT) >> 16;
//					sis_priv->stats.tx_bytes += tx_status & DSIZE;
					unit->sis900u_stats.PacketsSent++;
				}
				/* Mark the buffer as usable again ... */
				unit->tx_ring[entry].cmdsts = 0;
			}
		}

		/* something strange happened !!! */
		if (status & HIBERR) {
D(bug("[%s]: SiS900_IntHandlerF: Abnormal interrupt, status %#8.8x\n", unit->sis900u_name, status));
			break;
		}
		if (--boguscnt < 0) {
D(bug("[%s]: SiS900_IntHandlerF: Too much work at interrupt, interrupt status = %#8.8x\n", unit->sis900u_name, status));
			break;
		}
	} while (1);

D(bug("[%s]: SiS900_IntHandlerF: exiting interrupt, interrupt status = 0x%#8.8x\n", unit->sis900u_name, LONGIN(ioaddr + isr)));

   return FALSE;

   AROS_INTFUNC_EXIT
}

VOID CopyPacket(struct SiS900Base *SiS900DeviceBase, struct SiS900Unit *unit, 
    struct IOSana2Req *request, UWORD packet_size, UWORD packet_type,
    struct eth_frame *buffer)
{
    struct Opener *opener;
    BOOL filtered = FALSE;
    UBYTE *ptr;
    const UBYTE broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

D(bug("[%s]: CopyPacket(packet @ %x, len = %d)\n", unit->sis900u_name, buffer, packet_size));

    /* Set multicast and broadcast flags */

    request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
    if (memcmp(buffer->eth_packet_dest, broadcast, 6) == 0)
    {
       request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
D(bug("[%s]: CopyPacket: BROADCAST Flag set\n", unit->sis900u_name));
    }
    else if((buffer->eth_packet_dest[0] & 0x1) != 0)
    {
       request->ios2_Req.io_Flags |= SANA2IOF_MCAST;
D(bug("[%s]: CopyPacket: MULTICAST Flag set\n", unit->sis900u_name));
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

D(bug("[%s]: CopyPacket: packet @ %x (%d bytes)\n", unit->sis900u_name, ptr, packet_size));

    /* Filter packet */

    opener = request->ios2_BufferManagement;
    if((request->ios2_Req.io_Command == CMD_READ) &&
        (opener->filter_hook != NULL))
        if(!CallHookPkt(opener->filter_hook, request, ptr))
        {
D(bug("[%s]: CopyPacket: packet filtered\n", unit->sis900u_name));
            filtered = TRUE;
        }

    if(!filtered)
    {
        /* Copy packet into opener's buffer and reply packet */
D(bug("[%s]: CopyPacket: opener recieve packet .. ", unit->sis900u_name));
        if(!opener->rx_function(request->ios2_Data, ptr, packet_size))
        {
D(bug("ERROR occured!!\n"));
            request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
            request->ios2_WireError = S2WERR_BUFF_ERROR;
            ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_RX);
        }
        else
        {
D(bug("SUCCESS!!\n"));
        }
        Disable();
        Remove((APTR)request);
        Enable();
        ReplyMsg((APTR)request);
D(bug("[%s]: CopyPacket: opener notified.\n", unit->sis900u_name));
    }
}

BOOL AddressFilter(struct SiS900Base *SiS900DeviceBase, struct SiS900Unit *unit, UBYTE *address)
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

        range = (APTR)unit->sis900u_multicast_ranges.mlh_Head;
        tail = (APTR)&unit->sis900u_multicast_ranges.mlh_Tail;
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
            unit->sis900u_special_stats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    }
    return accept;
}

/*
 * Unit process
 */
AROS_UFH3(void, SiS900_Schedular,
    AROS_UFHA(STRPTR,              argPtr, A0),
    AROS_UFHA(ULONG,               argSize, D0),
    AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct SiS900Startup *sm_UD = FindTask(NULL)->tc_UserData;
    struct SiS900Unit *unit = sm_UD->sis900sm_Unit;

    LIBBASETYPEPTR LIBBASE = unit->sis900u_device;
    struct MsgPort *reply_port, *input;

D(bug("%s SiS900_Schedular()\n", unit->sis900u_name));
D(bug("%s SiS900_Schedular: Setting device up\n", unit->sis900u_name));

    reply_port = CreateMsgPort();
    input = CreateMsgPort();

    unit->sis900u_input_port = input; 

    unit->sis900u_TimerSlowPort = CreateMsgPort();

    if (unit->sis900u_TimerSlowPort)
    {
        unit->sis900u_TimerSlowReq = (struct timerequest *)
            CreateIORequest((struct MsgPort *)unit->sis900u_TimerSlowPort, sizeof(struct timerequest));

        if (unit->sis900u_TimerSlowReq)
        {
            if (!OpenDevice("timer.device", UNIT_VBLANK,
                (struct IORequest *)unit->sis900u_TimerSlowReq, 0))
            {
                struct Message *msg = AllocVec(sizeof(struct Message), MEMF_PUBLIC|MEMF_CLEAR);
                ULONG sigset;

D(bug("%s SiS900_Schedular: Got VBLANK unit of timer.device\n", unit->sis900u_name));

                sis900func_initialize(unit);

                msg->mn_ReplyPort = reply_port;
                msg->mn_Length = sizeof(struct Message);

D(bug("%s SiS900_Schedular: Setup complete. Sending handshake\n", unit->sis900u_name));
                PutMsg(sm_UD->sis900sm_SyncPort, msg);
                WaitPort(reply_port);
                GetMsg(reply_port);

                FreeVec(msg);

D(bug("%s SiS900_Schedular: entering forever loop ... \n", unit->sis900u_name));

                unit->sis900u_signal_0 = AllocSignal(-1);
                unit->sis900u_signal_1 = AllocSignal(-1);
                unit->sis900u_signal_2 = AllocSignal(-1);
                unit->sis900u_signal_3 = AllocSignal(-1);

                sigset = 1 << input->mp_SigBit  |
                         1 << unit->sis900u_signal_0  |
                         1 << unit->sis900u_signal_1  |
                         1 << unit->sis900u_signal_2  |
                         1 << unit->sis900u_signal_3;
                for(;;)
                {	
                    ULONG recvd = Wait(sigset);
                    if (recvd & unit->sis900u_signal_0)
                    {
                        /*
                         * Shutdown process. Driver should close everything 
                         * already and waits for our process to complete. Free
                         * memory allocared here and kindly return.
                         */
                        sis900func_deinitialize(unit);
                        CloseDevice((struct IORequest *)unit->sis900u_TimerSlowReq);
                        DeleteIORequest((struct IORequest *)unit->sis900u_TimerSlowReq);
                        DeleteMsgPort(unit->sis900u_TimerSlowPort);
                        DeleteMsgPort(input);
                        DeleteMsgPort(reply_port);

D(bug("%s SiS900_Schedular: Process shutdown.\n", unit->sis900u_name));
                        return;
                    }
                    else if (recvd & (1 << input->mp_SigBit))
                    {
                        struct IOSana2Req *io;

                        /* Handle incoming transactions */
                        while ((io = (struct IOSana2Req *)GetMsg(input))!= NULL);
                        {
D(bug("%s SiS900_Schedular: Handle incomming transaction.\n", unit->sis900u_name));
                            ObtainSemaphore(&unit->sis900u_unit_lock);
                            handle_request(LIBBASE, io);
                        }
                    }
                    else
                    {
D(bug("%s SiS900_Schedular: Handle incomming signal.\n", unit->sis900u_name));
                        /* Handle incoming signals */
                    }
                }
            }
        }
    }

    AROS_USERFUNC_EXIT
}

/*
 * Create new SiS900 ethernet device unit
 */
struct SiS900Unit *CreateUnit(struct SiS900Base *SiS900DeviceBase, OOP_Object *pciDevice, char * CardName, char * CardChipset)
{
    struct SiS900Unit *unit = NULL;
    BOOL success = TRUE;
    int i;

D(bug("[SiS900] CreateUnit()\n"));

    if ((unit = AllocMem(sizeof(struct SiS900Unit), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
    {
        IPTR        DeviceID = 0, RevisionID = 0, HostRevisionID = 0, base = 0, len;
        OOP_Object  *driver = NULL;

D(bug("[SiS900] CreateUnit: Unit allocated @ %x\n", unit));

		unit->sis900u_UnitNum = SiS900DeviceBase->sis900b_UnitCount++;

        unit->sis900u_Sana2Info.HardwareType = S2WireType_Ethernet;
        unit->sis900u_Sana2Info.MTU = ETH_MTU;
        unit->sis900u_Sana2Info.AddrFieldSize = 8 * ETH_ADDRESSSIZE;

        if ((unit->sis900u_name = AllocVec(7 + (unit->sis900u_UnitNum/10) + 2, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        {
            FreeMem(unit, sizeof(struct SiS900Unit));
            return NULL;
        }

        sprintf((char *)unit->sis900u_name, "sis900.%d", unit->sis900u_UnitNum);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_RevisionID, &RevisionID);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);

/* TODO: Get the host bridge revision!! */
/*        // save our host bridge revision
        dev = pci_get_device(PCI_VENDOR_ID_SI, PCI_DEVICE_ID_SI_630, NULL);
        if (dev) {
            pci_read_config_byte(dev, PCI_CLASS_REVISION, &sis_priv->host_bridge_rev);
            pci_dev_put(dev);
        }*/

        unit->sis900u_rtl_cardname   = CardName;
        unit->sis900u_rtl_chipname   = CardChipset;

        unit->sis900u_PCIDevice      = pciDevice;
        unit->sis900u_PCIDriver      = driver;

        unit->sis900u_device         = SiS900DeviceBase;
        unit->sis900u_DeviceID       = DeviceID;
        unit->sis900u_RevisionID     = RevisionID;
        unit->sis900u_HostRevisionID = HostRevisionID;

        unit->sis900u_mtu            = unit->sis900u_Sana2Info.MTU;

        InitSemaphore(&unit->sis900u_unit_lock);
        NEWLIST(&unit->sis900u_Openers);
        NEWLIST(&unit->sis900u_multicast_ranges);
        NEWLIST(&unit->sis900u_type_trackers);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &unit->sis900u_IRQ);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base1,   &unit->sis900u_BaseIO);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0,   &base);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0,   &len);

D(bug("%s CreateUnit:   INT:%d, base1:%x, base0:%x, size0:%d\n", unit->sis900u_name,
																									   unit->sis900u_IRQ, unit->sis900u_BaseIO,
																									   base, len));

        unit->sis900u_BaseMem = (IPTR)HIDD_PCIDriver_MapPCI(driver, (APTR)base, len);
        unit->sis900u_SizeMem = len;

        if (unit->sis900u_BaseMem)
        {
            struct TagItem attrs[] = {
                { aHidd_PCIDevice_isIO,     TRUE },
                { aHidd_PCIDevice_isMEM,    TRUE },
                { aHidd_PCIDevice_isMaster, TRUE },
                { TAG_DONE,                 0    },
            };
            OOP_SetAttrs(pciDevice, (struct TagItem *)&attrs);

D(bug("%s CreateUnit:   PCI_BaseMem @ %x\n", unit->sis900u_name, unit->sis900u_BaseMem));
            if ((unit->tx_ring = AllocMem(TX_TOTAL_SIZE, MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
            {

                if ((unit->tx_ring_dma = HIDD_PCIDriver_CPUtoPCI(unit->sis900u_PCIDriver, unit->tx_ring)) == NULL)
                {
D(bug("[%s]: CreateUnit: Failed to Map Tx Ring Buffer Descriptors DMA \n", unit->sis900u_name));
					return NULL;
                }
            }
            else
            {
D(bug("[%s]: CreateUnit: Failed to Allocate Tx Ring Buffer Descriptors\n", unit->sis900u_name));
				return NULL;
            }
D(bug("[%s]: CreateUnit: Tx Ring Buffer Descriptors @ %p [DMA @ %p]\n", unit->sis900u_name, unit->tx_ring, unit->tx_ring_dma));

            if ((unit->rx_ring = AllocMem(RX_TOTAL_SIZE, MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
            {

                if ((unit->rx_ring_dma = HIDD_PCIDriver_CPUtoPCI(unit->sis900u_PCIDriver, unit->rx_ring)) == NULL)
                {
D(bug("[%s]: CreateUnit: Failed to Map Rx Ring Buffer Descriptors DMA\n", unit->sis900u_name));
					return NULL;
				}
            }
            else
            {
D(bug("[%s]: CreateUnit: Failed to Allocate Rx Ring Buffer Descriptors DMA\n", unit->sis900u_name));
				return NULL;
            }
D(bug("[%s]: CreateUnit: Rx Ring Buffer Descriptors @ %p [DMA @ %p]\n", unit->sis900u_name, unit->rx_ring, unit->rx_ring_dma));

            {
                struct Message *msg;

                unit->sis900u_irqhandler.is_Node.ln_Type = NT_INTERRUPT;
                unit->sis900u_irqhandler.is_Node.ln_Pri = 100;
                unit->sis900u_irqhandler.is_Node.ln_Name = LIBBASE->sis900b_Device.dd_Library.lib_Node.ln_Name;
                unit->sis900u_irqhandler.is_Code = (VOID_FUNC)SiS900_IntHandlerF;
                unit->sis900u_irqhandler.is_Data = unit;

                unit->sis900u_touthandler.is_Node.ln_Type = NT_INTERRUPT;
                unit->sis900u_touthandler.is_Node.ln_Pri = 100;
                unit->sis900u_touthandler.is_Node.ln_Name = LIBBASE->sis900b_Device.dd_Library.lib_Node.ln_Name;
                unit->sis900u_touthandler.is_Code = (VOID_FUNC)SiS900_TimeoutHandlerF;
                unit->sis900u_touthandler.is_Data = unit;

                unit->sis900u_rx_int.is_Node.ln_Type = NT_INTERRUPT;
                unit->sis900u_rx_int.is_Node.ln_Name = unit->sis900u_name;
                unit->sis900u_rx_int.is_Code = (VOID_FUNC)SiS900_RX_IntF;
                unit->sis900u_rx_int.is_Data = unit;

                unit->sis900u_tx_int.is_Node.ln_Type = NT_INTERRUPT;
                unit->sis900u_tx_int.is_Node.ln_Name = unit->sis900u_name;
                unit->sis900u_tx_int.is_Code = (VOID_FUNC)SiS900_TX_IntF;
                unit->sis900u_tx_int.is_Data = unit;

                for (i=0; i < REQUEST_QUEUE_COUNT; i++)
                {
                    struct MsgPort *port;

                    if ((port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR)) == NULL) success = FALSE;

                    if (success)
                    {
                        unit->sis900u_request_ports[i] = port;
                        NEWLIST(&port->mp_MsgList);
                        port->mp_Flags = PA_IGNORE;
                        port->mp_SigTask = &unit->sis900u_tx_int;
                    }
                }

                unit->sis900u_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

                if (success)
                {
                    struct SiS900Startup *sm_UD;
                    UBYTE tmpbuff[100];

                    if ((sm_UD = AllocMem(sizeof(struct SiS900Startup), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
                    {
                        sprintf((char *)tmpbuff, SiS900_TASK_NAME, unit->sis900u_name);

                        sm_UD->sis900sm_SyncPort = CreateMsgPort();
                        sm_UD->sis900sm_Unit = unit;

                        unit->sis900u_Process = CreateNewProcTags(
                                                NP_Entry, (IPTR)SiS900_Schedular,
                                                NP_Name, tmpbuff,
                                                NP_Synchronous , FALSE,
                                                NP_Priority, 0,
                                                NP_UserData, (IPTR)sm_UD,
                                                NP_StackSize, 140960,
                                                TAG_DONE);

                        WaitPort(sm_UD->sis900sm_SyncPort);
                        msg = GetMsg(sm_UD->sis900sm_SyncPort);
                        ReplyMsg(msg);
                        DeleteMsgPort(sm_UD->sis900sm_SyncPort);
                        FreeMem(sm_UD, sizeof(struct SiS900Startup));

D(bug("[%s]  CreateUnit: Device Initialised. Unit %d @ %p\n", unit->sis900u_name, unit->sis900u_UnitNum, unit));
                        return unit;
                    }
                }
                else
                {
D(bug("[%s]: ERRORS occured during Device setup - ABORTING\n", unit->sis900u_name));
                }
            }
        }
        else
        {
D(bug("[SiS900] PANIC! Couldn't get MMIO area. Aborting\n"));
        }
    }
    DeleteUnit(SiS900DeviceBase, unit);	
    return NULL;
}

/*
 * DeleteUnit - removes selected unit. Frees all resources and structures.
 * 
 * The caller should be sure, that given unit is really ready to be freed.
 */

void DeleteUnit(struct SiS900Base *SiS900DeviceBase, struct SiS900Unit *Unit)
{
    int i;

D(bug("[SiS900] DeleteUnit()\n"));

    if (Unit)
    {
        if (Unit->sis900u_Process)
        {
            Signal(&Unit->sis900u_Process->pr_Task, Unit->sis900u_signal_0);
        }

        for (i=0; i < REQUEST_QUEUE_COUNT; i++)
        {
            if (Unit->sis900u_request_ports[i] != NULL) 
                FreeMem(Unit->sis900u_request_ports[i],	sizeof(struct MsgPort));

            Unit->sis900u_request_ports[i] = NULL;
        }

        if (Unit->sis900u_BaseMem)
        {
            HIDD_PCIDriver_UnmapPCI(Unit->sis900u_PCIDriver, 
                                    (APTR)Unit->sis900u_BaseMem,
                                    Unit->sis900u_SizeMem);
        }

        FreeMem(Unit, sizeof(struct SiS900Unit));
    }
}

static struct AddressRange *FindMulticastRange(LIBBASETYPEPTR LIBBASE, struct SiS900Unit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left, UWORD upper_bound_right)
{
    struct AddressRange *range, *tail;
    BOOL found = FALSE;

    range = (APTR)unit->sis900u_multicast_ranges.mlh_Head;
    tail = (APTR)&unit->sis900u_multicast_ranges.mlh_Tail;

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

BOOL AddMulticastRange(LIBBASETYPEPTR LIBBASE, struct SiS900Unit *unit, const UBYTE *lower_bound,
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
            AddTail((APTR)&unit->sis900u_multicast_ranges, (APTR)range);
            Enable();

            if (unit->sis900u_range_count++ == 0)
            {
                unit->sis900u_ifflags |= IFF_ALLMULTI;
                sis900func_set_multicast(unit);
            }
        }
    }

    return range != NULL;
}

BOOL RemMulticastRange(LIBBASETYPEPTR LIBBASE, struct SiS900Unit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound)
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

            if (--unit->sis900u_range_count == 0)
            {
                unit->sis900u_ifflags &= ~IFF_ALLMULTI;
                sis900func_set_multicast(unit);
            }
        }
    }
    return range != NULL;
}

