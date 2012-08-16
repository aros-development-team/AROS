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

#include "via-rhine.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/*
 * Report incoming events to all hyphotetical event receivers
 */
VOID ReportEvents(struct VIARHINEBase *VIARHINEDeviceBase, struct VIARHINEUnit *unit, ULONG events)
{
    struct IOSana2Req *request, *tail, *next_request;
    struct List *list;

    list = &unit->rhineu_request_ports[EVENT_QUEUE]->mp_MsgList;
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

struct TypeStats *FindTypeStats(struct VIARHINEBase *VIARHINEDeviceBase, struct VIARHINEUnit *unit, 
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

void FlushUnit(LIBBASETYPEPTR LIBBASE, struct VIARHINEUnit *unit, UBYTE last_queue, BYTE error)
{
    struct IORequest *request;
    UBYTE i;
    struct Opener *opener, *tail;

D(bug("%s unit.FlushUnit\n", unit->rhineu_name));

    /* Abort queued operations */

    for (i=0; i <= last_queue; i++)
    {
        while ((request = (APTR)GetMsg(unit->rhineu_request_ports[i])) != NULL)
        {
            request->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *)request);
        }
    }

    opener = (APTR)unit->rhineu_Openers.mlh_Head;
    tail = (APTR)&unit->rhineu_Openers.mlh_Tail;

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
 * Interrupt handler called whenever RTL8139 NIC interface generates interrupt.
 * It's duty is to iterate throgh RX queue searching for new packets.
 * 
 * Please note, that allthough multicast support could be done on interface 
 * basis, it is done in this function as result of quick integration of both
 * the forcedeth driver (IFF_ALLMULTI flag) and etherling3 driver (AddressMatch
 * filter function).
 */
AROS_INTH1(VIARHINE_RX_IntF, struct VIARHINEUnit *, unit)
{
    AROS_INTFUNC_INIT

    struct VIARHINEBase *VIARHINEDeviceBase = unit->rhineu_device;
    struct fe_priv *np = unit->rhineu_fe_priv;
    // UWORD Flags;
    struct TypeStats *tracker;
    ULONG packet_type;
    struct Opener *opener, *opener_tail;
    struct IOSana2Req *request, *request_tail;
    BOOL accepted, is_orphan;

    UBYTE *base = (UBYTE*) unit->rhineu_BaseMem;
	
	int count = 0;
	
D(bug("%s: VIARHINE_RX_IntF() !!!!\n", unit->rhineu_name));

    /* Endless loop, with break from inside */
    for(;;)
    {
        int i, len=0;
        struct eth_frame *frame;

        if (count >= RX_RING_SIZE)
            break;	/* we scanned the whole ring - do not continue */

        /* Get the in-queue number */
        i = np->cur_rx % RX_RING_SIZE;

        /* Do we own the packet or the chipset? */
        if (np->rx_desc[i].rx_status & DescOwn)
        {
//D(bug("%s: VIARHINE_RX_IntF: buffer %d owned by chipset\n", unit->rhineu_name, i));
            //goto next_pkt;	 // still owned by hardware,
			break;
        }

		if (np->rx_desc[i].rx_status & RxErr)
        {
D(bug("%s: VIARHINE_RX_IntF: buffer %d has recieve error! skipping..\n", unit->rhineu_name, i));
            goto next_pkt;	 // still owned by hardware,
        }

		if (!(np->rx_desc[i].rx_status & RxWholePkt))
        {
D(bug("%s: VIARHINE_RX_IntF: buffer %d has recieved oversized packet! skipping..\n", unit->rhineu_name, i));
            goto next_pkt;	 // still owned by hardware,
        }

		if (np->rx_desc[i].rx_status & RxOK)
        {
D(bug("%s: VIARHINE_RX_IntF: packet in buffer %d was successfully recieved.\n", unit->rhineu_name, i));
        }
		
D(bug("%s: VIARHINE_RX_IntF: buffer %d @ %x is for us\n", unit->rhineu_name, i, np->rx_desc[i].addr));
        /* the packet is for us - get it */

        /* got a valid packet - forward it to the network core */
        frame = (struct eth_frame *)(IPTR)np->rx_desc[i].addr;
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
			len = (np->rx_desc[i].rx_status >> 16)-4;

            packet_type = AROS_BE2WORD(frame->eth_packet_type);
D(bug("%s: VIARHINE_RX_IntF: Packet IP accepted with type = %d, len = %d, desc_length = %d\n", unit->rhineu_name, packet_type, len, np->rx_desc[i].desc_length));

            opener = (APTR)unit->rhineu_Openers.mlh_Head;
            opener_tail = (APTR)&unit->rhineu_Openers.mlh_Tail;

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
D(bug("%s: VIARHINE_RX_IntF: copy packet for opener ..\n", unit->rhineu_name));
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
                unit->rhineu_stats.UnknownTypesReceived++;

                if(!IsMsgPortEmpty(unit->rhineu_request_ports[ADOPT_QUEUE]))
                {
                    CopyPacket(LIBBASE, unit,
                        (APTR)unit->rhineu_request_ports[ADOPT_QUEUE]->
                        mp_MsgList.lh_Head, len, packet_type, frame);
D(bug("%s: VIARHINE_RX_IntF: packet copied to orphan queue\n", unit->rhineu_name));
                }
            }
			np->rx_desc[i].desc_length =  MAX_FRAME_SIZE; //Set the buffer size back to max (before enabling it)
			np->rx_desc[i].rx_status =  DescOwn;

			WORDOUT(base, CmdRxDemand | np->cmd);

            /* Update remaining statistics */

            tracker =
                FindTypeStats(LIBBASE, unit, &unit->rhineu_type_trackers, packet_type);

            if(tracker != NULL)
            {
                tracker->stats.PacketsReceived++;
                tracker->stats.BytesReceived += len;
            }
        }

        unit->rhineu_stats.PacketsReceived++;
//        ((struct rx_ring_desc *)np->ring_addr)[i].BufferStatus = AROS_WORD2LE((1 << 8)|(1 << 9)|(1 << 15)); // Mark packet as available again

next_pkt:
        np->cur_rx++;
		count++;
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * Interrupt generated by Cause() to push new packets into the NIC interface
 */
AROS_INTH1(VIARHINE_TX_IntF, struct VIARHINEUnit *,  unit)
{
    AROS_INTFUNC_INIT

    struct fe_priv *np = unit->rhineu_fe_priv;
    struct VIARHINEBase *VIARHINEDeviceBase = unit->rhineu_device;
    int nr, try_count=1;
    BOOL proceed = FALSE; /* Fails by default */

D(bug("%s: VIARHINE_TX_IntF()\n", unit->rhineu_name));

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
		UBYTE *base = (UBYTE*) unit->rhineu_BaseMem;
        port = unit->rhineu_request_ports[WRITE_QUEUE];

        /* Still no error and there are packets to be sent? */
        while(proceed && (!IsMsgPortEmpty(port)))
        {
rhine_nexttx:
			if (try_count >= (TX_BUFFERS * 3))
			{
/* TODO: We should probably report that we couldnt send the packet here.. */
D(bug("%s: VIARHINE_TX_IntF: Send FAILED! no free Tx buffer(s)\n", unit->rhineu_name));
				break;
			}

            nr = np->tx_current % TX_BUFFERS;
			np->tx_current++;

			if (np->tx_desc[nr].tx_status & DescOwn)
			{
D(bug("%s: VIARHINE_TX_IntF: Buffer %d in use!\n", unit->rhineu_name, nr));
				try_count++;
				goto rhine_nexttx;
			}

		   request = (APTR)port->mp_MsgList.lh_Head;
		   data_size = packet_size = request->ios2_DataLength;

		   opener = (APTR)request->ios2_BufferManagement;

		   if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
		   {
			  packet_size += ETH_PACKET_DATA;
			  CopyMem(request->ios2_DstAddr, &((struct eth_frame *)(IPTR)np->tx_desc[nr].addr)->eth_packet_dest, ETH_ADDRESSSIZE);
			  CopyMem(unit->rhineu_dev_addr, &((struct eth_frame *)(IPTR)np->tx_desc[nr].addr)->eth_packet_source, ETH_ADDRESSSIZE);
			  ((struct eth_frame *)(IPTR)np->tx_desc[nr].addr)->eth_packet_type = AROS_WORD2BE(request->ios2_PacketType);

			  buffer = (UBYTE *)&((struct eth_frame *)(IPTR)np->tx_desc[nr].addr)->eth_packet_data;
		   }
		   else
			  buffer = (UBYTE *)(IPTR)np->tx_desc[nr].addr;

		   if (!opener->tx_function(buffer, request->ios2_Data, data_size))
		   {
			  error = S2ERR_NO_RESOURCES;
			  wire_error = S2WERR_BUFF_ERROR;
			  ReportEvents(LIBBASE, unit,
				 S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF
				 | S2EVENT_TX);
		   }

		   /* Now the packet is already in TX buffer, update flags for NIC */
D(bug("%s: VIARHINE_TX_IntF: packet %d  @ %x [type = %d] queued for transmission.", unit->rhineu_name, nr, np->tx_desc[nr].addr, ((struct eth_frame *)(IPTR)np->tx_desc[nr].addr)->eth_packet_type));
			  Disable();
			  /* DEBUG? Dump frame if so */
#ifdef DEBUG
			  {
			  int j;
				   for (j=0; j<64; j++) {
					  if ((j%16) == 0)
						  D(bug("\n%03x:", j));
					  D(bug(" %02x", ((unsigned char*)(IPTR)np->tx_desc[nr].addr)[j]));
				 }
				  D(bug("\n"));
			  }
#endif
			  Enable();

			  /* Set the ring details for the packet .. */
			  np->tx_desc[nr].tx_status = DescOwn;
			  np->tx_desc[nr].desc_length = 0x00e08000 | packet_size;
			  WORDOUT(base + VIAR_ChipCmd, CmdTxDemand | np->cmd);
D(bug("%s: VIARHINE_TX_IntF: Packet Queued.\n", unit->rhineu_name));

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
			   tracker = FindTypeStats(LIBBASE, unit, &unit->rhineu_type_trackers,
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
        unit->rhineu_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
    else
        unit->rhineu_request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * Handle timeouts and other strange cases
 */
static AROS_INTH1(VIARHINE_TimeoutHandlerF, struct VIARHINEUnit *, dev)
{
    AROS_INTFUNC_INIT

    struct timeval time;
    struct Device *TimerBase = dev->rhineu_TimerSlowReq->tr_node.io_Device;

    GetSysTime(&time);
//D(bug("%s: VIARHINE_TimeoutHandlerF()\n", dev->rhineu_name));

    /*
     * If timeout timer is expected, and time elapsed - regenerate the 
     * interrupt handler 
     */
    if (dev->rhineu_toutNEED && (CmpTime(&time, &dev->rhineu_toutPOLL ) < 0))
    {
        dev->rhineu_toutNEED = FALSE;
        //Cause(&dev->rhineu_tx_end_int);
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * The interrupt handler - schedules code execution to proper handlers
 */
static AROS_INTH1(VIARHINE_IntHandlerF, struct VIARHINEUnit *, dev)
{
    AROS_INTFUNC_INIT

    struct VIARHINEBase *VIARHINEDeviceBase = dev->rhineu_device;
    struct fe_priv *np = dev->rhineu_fe_priv;
    UBYTE *base = (UBYTE*) dev->rhineu_BaseMem;
    // ULONG events;
    // int i, link_changed;
    int interrupt_work = 20;
    // struct Device *TimerBase = dev->rhineu_TimerSlowReq->tr_node.io_Device;
    // struct timeval time;

D(bug("%s: VIARHINE_IntHandlerF()!!!!!!!\n", dev->rhineu_name));
    
    while (1)
    {
		int status = WORDIN(base + VIAR_IntrStatus);
		WORDOUT(base + VIAR_IntrStatus, status & 0xffff); // Aknowledge All Interrupt SOurces ASAP.

		if (!(status & (IntrRxDone | IntrRxErr | IntrRxEmpty | IntrRxOverflow| IntrRxDropped | IntrTxDone | IntrTxAbort | IntrTxUnderrun | IntrPCIErr | IntrStatsMax | IntrLinkChange | IntrMIIChange)))
		{
D(bug("%s: VIARHINE_IntHandlerF: No Signals for us!\n", dev->rhineu_name));
			break;
		}
		
		if ( status & (IntrRxErr | IntrRxDropped | IntrRxWakeUp | IntrRxEmpty | IntrRxNoBuf )) // Chipset has Reported a Recieve problem
		{
D(bug("%s: VIARHINE_IntHandlerF: Packet Reception Problem detected!\n", dev->rhineu_name));
			WORDOUT(base, CmdRxDemand | np->cmd);
			ReportEvents(LIBBASE, dev, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
			dev->rhineu_stats.BadData++;
		}
		else if ( status & IntrRxDone ) // Chipset has Recieved packet(s)
		{
D(bug("%s: VIARHINE_IntHandlerF: Packet Reception Attempt detected!\n", dev->rhineu_name));
			Cause(&dev->rhineu_rx_int);
		}

		if ( status & IntrTxAbort ) // Chipset has Aborted Packet Transmition
		{
D(bug("%s: VIARHINE_IntHandlerF: Chipset has Aborted Packet Transmition!\n", dev->rhineu_name));
			WORDOUT(base, CmdTxDemand | np->cmd);
			ReportEvents(LIBBASE, dev, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_TX);
		}
		
		if ( status & IntrTxUnderrun ) // Chipset Reports Tx Underrun
		{
D(bug("%s: VIARHINE_IntHandlerF: Chipset Reports Tx Underrun!\n", dev->rhineu_name));
			if (np->tx_thresh < 0xe0) BYTEOUT(base + VIAR_TxConfig, np->tx_thresh += 0x20);
			WORDOUT(base, CmdTxDemand | np->cmd);
			ReportEvents(LIBBASE, dev, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_TX);
		}

		if ( status & IntrTxDone ) // Chipset has Sent packet(s)
		{
D(bug("%s: VIARHINE_IntHandlerF: Packet Transmision detected!\n", dev->rhineu_name));
			dev->rhineu_stats.PacketsSent++;
		}
		
		if (--interrupt_work < 0)
		{
D(bug("%s: VIARHINE_IntHandlerF: MAX interrupt work reached.\n", dev->rhineu_name));
			break;
		}
    }

   return FALSE;

   AROS_INTFUNC_EXIT
}

VOID CopyPacket(struct VIARHINEBase *VIARHINEDeviceBase, struct VIARHINEUnit *unit, 
    struct IOSana2Req *request, UWORD packet_size, UWORD packet_type,
    struct eth_frame *buffer)
{
    struct Opener *opener;
    BOOL filtered = FALSE;
    UBYTE *ptr;

D(bug("%s: CopyPacket(packet @ %x, len = %d)\n", unit->rhineu_name, buffer, packet_size));

    /* Set multicast and broadcast flags */

    request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
    if((*((ULONG *)(buffer->eth_packet_dest)) == 0xffffffff) &&
       (*((UWORD *)(buffer->eth_packet_dest + 4)) == 0xffff))
    {
       request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
D(bug("%s: CopyPacket: BROADCAST Flag set\n", unit->rhineu_name));
    }
    else if((buffer->eth_packet_dest[0] & 0x1) != 0)
    {
       request->ios2_Req.io_Flags |= SANA2IOF_MCAST;
D(bug("%s: CopyPacket: MULTICAST Flag set\n", unit->rhineu_name));
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

D(bug("%s: CopyPacket: packet @ %x (%d bytes)\n", unit->rhineu_name, ptr, packet_size));

    /* Filter packet */

    opener = request->ios2_BufferManagement;
    if((request->ios2_Req.io_Command == CMD_READ) &&
        (opener->filter_hook != NULL))
        if(!CallHookPkt(opener->filter_hook, request, ptr))
        {
D(bug("%s: CopyPacket: packet filtered\n", unit->rhineu_name));
            filtered = TRUE;
        }

    if(!filtered)
    {
        /* Copy packet into opener's buffer and reply packet */
D(bug("%s: CopyPacket: opener recieve packet .. ", unit->rhineu_name));
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
D(bug("%s: CopyPacket: opener notified.\n", unit->rhineu_name));
    }
}

BOOL AddressFilter(struct VIARHINEBase *VIARHINEDeviceBase, struct VIARHINEUnit *unit, UBYTE *address)
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

        range = (APTR)unit->rhineu_multicast_ranges.mlh_Head;
        tail = (APTR)&unit->rhineu_multicast_ranges.mlh_Tail;
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
            unit->rhineu_special_stats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    }
    return accept;
}

/*
 * Unit process
 */
AROS_UFH3(void, VIARHINE_Schedular,
    AROS_UFHA(STRPTR,              argPtr, A0),
    AROS_UFHA(ULONG,               argSize, D0),
    AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct VIARHINEUnit *dev = FindTask(NULL)->tc_UserData;
    LIBBASETYPEPTR LIBBASE = dev->rhineu_device;
    struct MsgPort *reply_port, *input;

D(bug("%s VIARHINE_Schedular()\n", dev->rhineu_name));
D(bug("%s VIARHINE_Schedular: Setting device up\n", dev->rhineu_name));

    reply_port = CreateMsgPort();
    input = CreateMsgPort();

    dev->rhineu_input_port = input; 

    dev->rhineu_TimerSlowPort = CreateMsgPort();

    if (dev->rhineu_TimerSlowPort)
    {
        dev->rhineu_TimerSlowReq = (struct timerequest *)
            CreateIORequest((struct MsgPort *)dev->rhineu_TimerSlowPort, sizeof(struct timerequest));

        if (dev->rhineu_TimerSlowReq)
        {
            if (!OpenDevice("timer.device", UNIT_VBLANK,
                (struct IORequest *)dev->rhineu_TimerSlowReq, 0))
            {
                struct Message *msg = AllocVec(sizeof(struct Message), MEMF_PUBLIC|MEMF_CLEAR);
                ULONG sigset;

D(bug("%s VIARHINE_Schedular: Got VBLANK unit of timer.device\n", dev->rhineu_name));

                dev->initialize(dev);

                msg->mn_ReplyPort = reply_port;
                msg->mn_Length = sizeof(struct Message);

D(bug("%s VIARHINE_Schedular: Setup complete. Sending handshake\n", dev->rhineu_name));
                PutMsg(LIBBASE->rhineb_syncport, msg);
                WaitPort(reply_port);
                GetMsg(reply_port);

                FreeVec(msg);

D(bug("%s VIARHINE_Schedular: entering forever loop ... \n", dev->rhineu_name));

                dev->rhineu_signal_0 = AllocSignal(-1);
                dev->rhineu_signal_1 = AllocSignal(-1);
                dev->rhineu_signal_2 = AllocSignal(-1);
                dev->rhineu_signal_3 = AllocSignal(-1);

                sigset = 1 << input->mp_SigBit  |
                         1 << dev->rhineu_signal_0  |
                         1 << dev->rhineu_signal_1  |
                         1 << dev->rhineu_signal_2  |
                         1 << dev->rhineu_signal_3;
                for(;;)
                {	
                    ULONG recvd = Wait(sigset);
                    if (recvd & dev->rhineu_signal_0)
                    {
                        /*
                         * Shutdown process. Driver should close everything 
                         * already and waits for our process to complete. Free
                         * memory allocared here and kindly return.
                         */
                        dev->deinitialize(dev);
                        CloseDevice((struct IORequest *)dev->rhineu_TimerSlowReq);
                        DeleteIORequest((struct IORequest *)dev->rhineu_TimerSlowReq);
                        DeleteMsgPort(dev->rhineu_TimerSlowPort);
                        DeleteMsgPort(input);
                        DeleteMsgPort(reply_port);

D(bug("%s VIARHINE_Schedular: Process shutdown.\n", dev->rhineu_name));
                        return;
                    }
                    else if (recvd & (1 << input->mp_SigBit))
                    {
                        struct IOSana2Req *io;

                        /* Handle incoming transactions */
                        while ((io = (struct IOSana2Req *)GetMsg(input))!= NULL);
                        {
D(bug("%s VIARHINE_Schedular: Handle incomming transaction.\n", dev->rhineu_name));
                            ObtainSemaphore(&dev->rhineu_unit_lock);
                            handle_request(LIBBASE, io);
                        }
                    }
                    else
                    {
D(bug("%s VIARHINE_Schedular: Handle incomming signal.\n", dev->rhineu_name));
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
struct VIARHINEUnit *CreateUnit(struct VIARHINEBase *VIARHINEDeviceBase, OOP_Object *pciDevice, ULONG CardCapabilities, char * CardName)
{
    BOOL success = TRUE;
    int i;

D(bug("[VIA-RHINE] CreateUnit()\n"));

    struct VIARHINEUnit *unit = AllocMem(sizeof(struct VIARHINEUnit), MEMF_PUBLIC | MEMF_CLEAR);

    if (unit != NULL)
    {
        IPTR        DeviceID, base, len;
        OOP_Object  *driver;

D(bug("[VIA-RHINE] CreateUnit: Unit allocated @ %x\n", unit));
		
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);

        unit->rhineu_cardname = CardName;
        unit->rhineu_chipcapabilities = CardCapabilities;
        unit->rhineu_device     = VIARHINEDeviceBase;
        unit->rhineu_DeviceID   = DeviceID;
        unit->rhineu_mtu        = ETH_MTU;
        unit->rhineu_PCIDevice  = pciDevice;
        unit->rhineu_PCIDriver  = driver;
		unit->rhineu_UnitNum = VIARHINEDeviceBase->rhineb_UnitCount++;
        
		int unitname_len = 12 + ((unit->rhineu_UnitNum/10)+1);

		unit->rhineu_name = AllocVec(unitname_len, MEMF_CLEAR|MEMF_PUBLIC);
D(bug("[VIA-RHINE] CreateUnit: Allocated %d bytes for Unit %d's Name @ %x\n", unitname_len, unit->rhineu_UnitNum, unit->rhineu_name));
		sprintf(unit->rhineu_name, "[VIARHINE.%d]", unit->rhineu_UnitNum);

        InitSemaphore(&unit->rhineu_unit_lock);
        NEWLIST(&unit->rhineu_Openers);
        NEWLIST(&unit->rhineu_multicast_ranges);
        NEWLIST(&unit->rhineu_type_trackers);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &unit->rhineu_IRQ);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base1,   &unit->rhineu_BaseIO);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0,   &base);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0,   &len);

D(bug("%s CreateUnit:   INT:%d, base1:%x, base0:%x, size0:%d\n", unit->rhineu_name,
																									   unit->rhineu_IRQ, unit->rhineu_BaseIO,
																									   base, len));

        unit->rhineu_BaseMem = (IPTR)HIDD_PCIDriver_MapPCI(driver, (APTR)base, len);
        unit->rhineu_SizeMem = len;

        if (unit->rhineu_BaseMem)
        {
            struct TagItem attrs[] = {
                { aHidd_PCIDevice_isIO,     TRUE },
                { aHidd_PCIDevice_isMEM,    TRUE },
                { aHidd_PCIDevice_isMaster, TRUE },
                { TAG_DONE,                 0    },
            };
            OOP_SetAttrs(pciDevice, (struct TagItem *)&attrs);

D(bug("%s CreateUnit:   PCI_BaseMem @ %x\n", unit->rhineu_name, unit->rhineu_BaseMem));
			
            unit->rhineu_fe_priv = 	HIDD_PCIDriver_AllocPCIMem(
                        unit->rhineu_PCIDriver,
                        sizeof(struct fe_priv));

            viarhinenic_get_functions(unit);

            if (unit->rhineu_fe_priv)
            {
D(bug("%s CreateUnit:   NIC Private Data Area @ %x, start @ %x\n", unit->rhineu_name, unit->rhineu_fe_priv));
				
                unit->rhineu_fe_priv->pci_dev = unit;
                InitSemaphore(&unit->rhineu_fe_priv->lock);

                {
                    struct Message *msg;

                    unit->rhineu_irqhandler.is_Node.ln_Type = NT_INTERRUPT;
                    unit->rhineu_irqhandler.is_Node.ln_Pri = 100;
                    unit->rhineu_irqhandler.is_Node.ln_Name = LIBBASE->rhineb_Device.dd_Library.lib_Node.ln_Name;
                    unit->rhineu_irqhandler.is_Code = VIARHINE_IntHandlerF;
                    unit->rhineu_irqhandler.is_Data = unit;

                    unit->rhineu_touthandler.is_Node.ln_Type = NT_INTERRUPT;
                    unit->rhineu_touthandler.is_Node.ln_Pri = 100;
                    unit->rhineu_touthandler.is_Node.ln_Name = LIBBASE->rhineb_Device.dd_Library.lib_Node.ln_Name;
                    unit->rhineu_touthandler.is_Code = VIARHINE_TimeoutHandlerF;
                    unit->rhineu_touthandler.is_Data = unit;

                    unit->rhineu_rx_int.is_Node.ln_Name = unit->rhineu_name;
                    unit->rhineu_rx_int.is_Code = VIARHINE_RX_IntF;
                    unit->rhineu_rx_int.is_Data = unit;

                    unit->rhineu_tx_int.is_Node.ln_Name = unit->rhineu_name;
                    unit->rhineu_tx_int.is_Code = VIARHINE_TX_IntF;
                    unit->rhineu_tx_int.is_Data = unit;

                    for (i=0; i < REQUEST_QUEUE_COUNT; i++)
                    {
                        struct MsgPort *port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);
                        unit->rhineu_request_ports[i] = port;

                        if (port == NULL) success = FALSE;

                        if (success)
                        {
                            NEWLIST(&port->mp_MsgList);
                            port->mp_Flags = PA_IGNORE;
                            port->mp_SigTask = &unit->rhineu_tx_int;
                        }
                    }

                    unit->rhineu_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

                    if (success)
                    {
                        LIBBASE->rhineb_syncport = CreateMsgPort();

                        unit->rhineu_Process = CreateNewProcTags(
                                                NP_Entry, (IPTR)VIARHINE_Schedular,
                                                NP_Name, VIARHINE_TASK_NAME,
                                                NP_Priority, 0,
                                                NP_UserData, (IPTR)unit,
                                                NP_StackSize, 140960,
                                                TAG_DONE);

                        WaitPort(LIBBASE->rhineb_syncport);
                        msg = GetMsg(LIBBASE->rhineb_syncport);
                        ReplyMsg(msg);
                        DeleteMsgPort(LIBBASE->rhineb_syncport);

D(bug("[VIA-RHINE] Unit up and running\n"));

                        return unit;
                    }
                    else
                    {
D(bug("%s: ERRORS occured during Device setup - ABORTING\n", unit->rhineu_name));
                    }
                }
            }
        }
        else
D(bug("[VIA-RHINE] PANIC! Couldn't get MMIO area. Aborting\n"));
    }
    DeleteUnit(VIARHINEDeviceBase, unit);	
    return NULL;
}

/*
 * DeleteUnit - removes selected unit. Frees all resources and structures.
 * 
 * The caller should be sure, that given unit is really ready to be freed.
 */

void DeleteUnit(struct VIARHINEBase *VIARHINEDeviceBase, struct VIARHINEUnit *Unit)
{
    int i;
    if (Unit)
    {
        if (Unit->rhineu_Process)
        {
            Signal(&Unit->rhineu_Process->pr_Task, Unit->rhineu_signal_0);
        }

        for (i=0; i < REQUEST_QUEUE_COUNT; i++)
        {
            if (Unit->rhineu_request_ports[i] != NULL) 
                FreeMem(Unit->rhineu_request_ports[i],	sizeof(struct MsgPort));

            Unit->rhineu_request_ports[i] = NULL;
        }

        if (Unit->rhineu_fe_priv)
        {
            FreeMem(Unit->rhineu_fe_priv, sizeof(struct fe_priv));
            Unit->rhineu_fe_priv = NULL;
        }

        if (Unit->rhineu_BaseMem)
        {
            HIDD_PCIDriver_UnmapPCI(Unit->rhineu_PCIDriver, 
                                    (APTR)Unit->rhineu_BaseMem,
                                    Unit->rhineu_SizeMem);
        }

        FreeMem(Unit, sizeof(struct VIARHINEUnit));
    }
}

static struct AddressRange *FindMulticastRange(LIBBASETYPEPTR LIBBASE, struct VIARHINEUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left, UWORD upper_bound_right)
{
    struct AddressRange *range, *tail;
    BOOL found = FALSE;

    range = (APTR)unit->rhineu_multicast_ranges.mlh_Head;
    tail = (APTR)&unit->rhineu_multicast_ranges.mlh_Tail;

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

BOOL AddMulticastRange(LIBBASETYPEPTR LIBBASE, struct VIARHINEUnit *unit, const UBYTE *lower_bound,
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
            AddTail((APTR)&unit->rhineu_multicast_ranges, (APTR)range);
            Enable();

            if (unit->rhineu_range_count++ == 0)
            {
                unit->rhineu_flags |= IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }

    return range != NULL;
}

BOOL RemMulticastRange(LIBBASETYPEPTR LIBBASE, struct VIARHINEUnit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound)
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

            if (--unit->rhineu_range_count == 0)
            {
                unit->rhineu_flags &= ~IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }
    return range != NULL;
}

