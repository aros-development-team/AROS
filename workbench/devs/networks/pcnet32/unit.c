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

#include "pcnet32.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/* BYTE IO */
UBYTE readb(APTR base)
{
    return BYTEIN(base);
}
void writeb(UBYTE val, APTR base)
{
    BYTEOUT(base, val);
}

/* WORD IO */
UWORD readw(APTR base)
{
    return WORDIN(base);
}
void writew(UWORD val, APTR base)
{
    WORDOUT(base, val);
}

/* LONG IO */
ULONG readl(APTR base)
{
    return *((volatile ULONG*)base);
}
void writel(ULONG val, APTR base)
{
    *((volatile ULONG*)base) = val;
}

/* 16/32bit control funcs */
static UWORD pcnet32_readcsr_16(APTR base, int index)
{
   writew( index, base + 0x12);
   return readw( base + 0x10);
}

static void pcnet32_writecsr_16(APTR base, int index, UWORD val)
{
   writew( index, base + 0x12);
   writew( val, base + 0x10);
}

static UWORD pcnet32_readbcr_16(APTR base, int index)
{
   writew( index, base + 0x12);
   return readw( base + 0x16);
}

static void pcnet32_writebcr_16(APTR base, int index, UWORD val)
{
   writew( index, base + 0x12);
   writew( val, base + 0x16);
}

static UWORD pcnet32_readrap_16(APTR base)
{
   return readw(base + 0x12);
}

static void pcnet32_writerap_16(APTR base, UWORD val)
{
   writew( val, base + 0x12);
}

static void pcnet32_reset_16(APTR base)
{
   readw(base + 0x14);
}

static int pcnet32_check_16(APTR base)
{
   writew(88, base + 0x12);
   return (readw( base + 0x12) == 88);
}

/**/

static UWORD pcnet32_readcsr_32(APTR base, int index)
{
   writel( index, base + 0x14);
   return (readl( base + 0x10) & 0xffff);
}

static void pcnet32_writecsr_32(APTR base, int index, UWORD val)
{
   writel( index, base + 0x14);
   writel( val, base + 0x10);
}

static UWORD pcnet32_readbcr_32(APTR base, int index)
{
   writel( index, base + 0x14);
   return (readl( base + 0x1c) & 0xffff);
}

static void pcnet32_writebcr_32(APTR base, int index, UWORD val)
{
   writel( index, base + 0x14);
   writel( val, base + 0x1c);
}

static UWORD pcnet32_readrap_32(APTR base)
{
   return (readl(base + 0x14) & 0xffff);
}

static void pcnet32_writerap_32(APTR base, UWORD val)
{
   writel( val, base + 0x14);
}

static void pcnet32_reset_32(APTR base)
{
   readl(base + 0x18);
}

static int pcnet32_check_32(APTR base)
{
   writel(88, base + 0x14);
   return ((readw( base + 0x14) & 0xffff) == 88);
}

/*
 * Report incoming events to all hyphotetical event receivers
 */
VOID ReportEvents(struct PCN32Base *PCNet32Base, struct PCN32Unit *unit, ULONG events)
{
    struct IOSana2Req *request, *tail, *next_request;
    struct List *list;

    list = &unit->pcnu_request_ports[EVENT_QUEUE]->mp_MsgList;
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

struct TypeStats *FindTypeStats(struct PCN32Base *PCNet32Base, struct PCN32Unit *unit, 
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

void FlushUnit(LIBBASETYPEPTR LIBBASE, struct PCN32Unit *unit, UBYTE last_queue, BYTE error)
{
    struct IORequest *request;
    UBYTE i;
    struct Opener *opener, *tail;

D(bug("[pcnet32] unit.FlushUnit\n"));

    /* Abort queued operations */

    for (i=0; i <= last_queue; i++)
    {
        while ((request = (APTR)GetMsg(unit->pcnu_request_ports[i])) != NULL)
        {
            request->io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *)request);
        }
    }

    opener = (APTR)unit->pcnu_Openers.mlh_Head;
    tail = (APTR)unit->pcnu_Openers.mlh_Tail;

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

static inline void pci_push(UBYTE *base)
{
    /* force out pending posted writes */
    readl(base);
}

/*
 * Interrupt handler called whenever pcnet32 NIC interface generates interrupt.
 * It's duty is to iterate throgh RX queue searching for new packets.
 * 
 * Please note, that allthough multicast support could be done on interface 
 * basis, it is done in this function as result of quick integration of both
 * the forcedeth driver (IFF_ALLMULTI flag) and etherling3 driver (AddressMatch
 * filter function).
 */
AROS_UFH3(void, PCN32_RX_Int,
    AROS_UFHA(struct PCN32Unit *,  unit, A1),
    AROS_UFHA(APTR,             dummy, A5),
    AROS_UFHA(struct ExecBase *,SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct PCN32Base *PCNet32Base = unit->pcnu_device;
    struct fe_priv *np = unit->pcnu_fe_priv;
    UWORD Flags;
    struct TypeStats *tracker;
    ULONG packet_type;
    struct Opener *opener, *opener_tail;
    struct IOSana2Req *request, *request_tail;
    BOOL accepted, is_orphan;

D(bug("%s: PCN32_RX_Int() !!!!\n", unit->pcnu_name));
    np->cur_rx = 0;

    /* Endless loop, with break from inside */
    for(;;)
    {
        int i;
        UWORD len=0;
        struct eth_frame *frame;

        if (np->cur_rx >= RX_RING_SIZE)
            break;	/* we scanned the whole ring - do not continue */

        /* Get the in-queue number */
        i = np->cur_rx % RX_RING_SIZE;
        Flags = AROS_LE2WORD(((struct rx_ring_desc *)np->ring_addr)[i].BufferStatus);
        len = AROS_LE2WORD(((struct rx_ring_desc *)np->ring_addr)[i].BufferMsgLength);

D(bug("%s: PCN32_RX_Int: looking at packet %d:%d, Flags 0x%x, len %d\n",
                unit->pcnu_name, np->cur_rx, i, Flags, len));

        /* Do we own the packet or the chipset? */
        if ((Flags & (1 << 15))!=0)
        {
D(bug("%s: PCN32_RX_Int: packet owned by chipset\n", unit->pcnu_name));
            goto next_pkt;	 /* still owned by hardware, */
        }

D(bug("%s: PCN32_RX_Int: packet is for us\n", unit->pcnu_name));

        /* the packet is for us - get it :) */

            if (Flags & (1 << 7)) { // Bus Parity Error
D(bug("%s: PCN32_RX_Int: packet has Bus Parity error!\n", unit->pcnu_name));
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->pcnu_stats.BadData++;
                goto next_pkt;
            }

            if (Flags & (1 << 8)) { // End of Packet
D(bug("%s: PCN32_RX_Int: END of Packet\n", unit->pcnu_name));
            }
            if (Flags & (1 << 9)) { // Start of Packet
D(bug("%s: PCN32_RX_Int: START of Packet\n", unit->pcnu_name));
            }

            if (Flags & (1 << 10)) { // Buffer Error
D(bug("%s: PCN32_RX_Int: packet has CRC error!\n", unit->pcnu_name));
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->pcnu_stats.BadData++;
                goto next_pkt;
            }
            if (Flags & (1 << 11)) { // CRC Error
D(bug("%s: PCN32_RX_Int: packet has CRC error!\n", unit->pcnu_name));
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->pcnu_stats.BadData++;
                goto next_pkt;
            }
            if (Flags & (1 << 12)) { // OVERFLOW Error
D(bug("%s: PCN32_RX_Int: packet has OVERFLOW error!\n", unit->pcnu_name));
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->pcnu_stats.BadData++;
                goto next_pkt;
            }
            if (Flags & (1 << 13)) { // Framing Error
                ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX);
                unit->pcnu_stats.BadData++;
                goto next_pkt;
            }

D(bug("%s: PCN32_RX_Int: packet doesnt report errors\n", unit->pcnu_name));

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
D(bug("%s: PCN32_RX_Int: Packet IP accepted with type = %d\n", unit->pcnu_name, packet_type));

            opener = (APTR)unit->pcnu_Openers.mlh_Head;
            opener_tail = (APTR)&unit->pcnu_Openers.mlh_Tail;

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
D(bug("%s: PCN32_RX_Int: copy packet for opener ..\n", unit->pcnu_name));
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
                unit->pcnu_stats.UnknownTypesReceived++;

                if(!IsMsgPortEmpty(unit->pcnu_request_ports[ADOPT_QUEUE]))
                {
                    CopyPacket(LIBBASE, unit,
                        (APTR)unit->pcnu_request_ports[ADOPT_QUEUE]->
                        mp_MsgList.lh_Head, len, packet_type, frame);
D(bug("%s: PCN32_RX_Int: packet copied to orphan queue\n", unit->pcnu_name));
                }
            }

            /* Update remaining statistics */

            tracker =
                FindTypeStats(LIBBASE, unit, &unit->pcnu_type_trackers, packet_type);

            if(tracker != NULL)
            {
                tracker->stats.PacketsReceived++;
                tracker->stats.BytesReceived += len;
            }
        }

        unit->pcnu_stats.PacketsReceived++;
        ((struct rx_ring_desc *)np->ring_addr)[i].BufferStatus = AROS_WORD2LE((1 << 8)|(1 << 9)|(1 << 15)); // Mark packet as available again
next_pkt:
        np->cur_rx++;
    }

    AROS_USERFUNC_EXIT
}

/*
 * Interrupt generated by Cause() to push new packets into the NIC interface
 */
AROS_UFH3(void, PCN32_TX_Int,
    AROS_UFHA(struct PCN32Unit *,  unit, A1),
    AROS_UFHA(APTR,             dummy, A5),
    AROS_UFHA(struct ExecBase *,SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct fe_priv *np = unit->pcnu_fe_priv;
    struct PCN32Base *PCNet32Base = unit->pcnu_device;
    int nr, try_count=1;
    BOOL proceed = FALSE; /* Fails by default */

D(bug("%s: PCN32_TX_Int()\n", unit->pcnu_name));

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
        port = unit->pcnu_request_ports[WRITE_QUEUE];

        /* Still no error and there are packets to be sent? */
        while(proceed && (!IsMsgPortEmpty(port)))
        {
            nr = np->next_tx % TX_RING_SIZE;
            error = 0;

            if (!((((struct tx_ring_desc *)np->ring_addr)[nr + RX_RING_SIZE].BufferStatus) & (1 << 15)))
            {

               request = (APTR)port->mp_MsgList.lh_Head;
               data_size = packet_size = request->ios2_DataLength;

               opener = (APTR)request->ios2_BufferManagement;

               if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
               {
                  packet_size += ETH_PACKET_DATA;
                  CopyMem(request->ios2_DstAddr, np->tx_buffer[nr].eth_packet_dest, ETH_ADDRESSSIZE);
                  CopyMem(unit->pcnu_dev_addr, np->tx_buffer[nr].eth_packet_source, ETH_ADDRESSSIZE);
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
D(bug("%s: PCN32_TX_Int: packet %d:%d [type = %d] queued for transmission.", unit->pcnu_name, np->next_tx, nr, np->tx_buffer[nr].eth_packet_type));

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

                  Enable();

                  /* Set the ring details for the packet .. */
                  ((struct tx_ring_desc *)np->ring_addr)[nr + RX_RING_SIZE].BufferLength = AROS_WORD2LE(-packet_size);
                  ((struct tx_ring_desc *)np->ring_addr)[nr + RX_RING_SIZE].Misc = 0x00000000;
                  ((struct tx_ring_desc *)np->ring_addr)[nr + RX_RING_SIZE].PacketBuffer = AROS_LONG2LE((IPTR)&np->tx_buffer[nr]);
                  ((struct tx_ring_desc *)np->ring_addr)[nr + RX_RING_SIZE].BufferStatus = AROS_WORD2LE(0x8300);
                
                  unit->write_csr((APTR)unit->pcnu_BaseMem,0, ((1 << 6)|(1 << 3))); /* .. And trigger an imediate Tx poll */
D(bug("%s: PCN32_TX_Int: send poll triggered.\n", unit->pcnu_name));
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
                   tracker = FindTypeStats(LIBBASE, unit, &unit->pcnu_type_trackers,
                       request->ios2_PacketType);
                   if(tracker != NULL)
                   {
                       tracker->stats.PacketsSent++;
                       tracker->stats.BytesSent += packet_size;
                   }
               }
               try_count=0;
            }
            np->next_tx++;
            try_count++;
            
            /* 
             * If we've just run out of free space on the TX queue, stop
             * it and give up pushing further frames
             */
            if ( (try_count + 1) >= TX_RING_SIZE)
            {
D(bug("%s: output queue full!. Stopping [count = %d, TX_RING_SIZE = %d\n", unit->pcnu_name, try_count, TX_RING_SIZE));
               netif_stop_queue(unit);
               proceed = FALSE;
            }
        }
    }

    /* Was there success? Enable incomming of new packets */    
    if(proceed)
        unit->pcnu_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
    else
        unit->pcnu_request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;

    AROS_USERFUNC_EXIT
}

/*
 * Interrupt used to restart the real one
 */
AROS_UFH3(void, PCN32_TX_End_Int,
    AROS_UFHA(struct PCN32Unit *,  unit, A1),
    AROS_UFHA(APTR,             dummy, A5),
    AROS_UFHA(struct ExecBase *,SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct PCN32Base *PCNet32Base = unit->pcnu_device;
    struct PCN32Unit *dev = unit;
    struct fe_priv *np = dev->pcnu_fe_priv;
    // UBYTE *base = (UBYTE*) dev->pcnu_BaseMem;

D(bug("%s: PCN32_TX_End_Int()\n", unit->pcnu_name));

   int i;
   UWORD Flags;

   for(i = 0; i < TX_RING_SIZE; i++)
   {
      Flags = AROS_LE2WORD(((struct tx_ring_desc *)np->ring_addr)[i + RX_RING_SIZE].BufferStatus);
      /* Do we own the packet or the chipset? */
D(bug("%s: PCN32_TX_End_Int: looking at TxRing packet %d:, Flags 0x%x\n",
                unit->pcnu_name, i, Flags));
      if ((Flags & (1 << 15))==0)
      {
D(bug("%s: PCN32_TX_End_Int: TxRing packet %d owned by us\n", unit->pcnu_name, i));
/* TODO: We should report send errors here .. */

         if (Flags & (1 << 14))
         {
D(bug("%s: PCN32_TX_End_Int: Errors occured transmitting packet\n", unit->pcnu_name));
            if (Flags & (1 << 11))
            {
D(bug("%s: PCN32_TX_End_Int: packet reports CRC Error\n", unit->pcnu_name));         
            }

            if (Flags & (1 << 12))
            {
D(bug("%s: PCN32_TX_End_Int: packet reports OVERFLOW error\n", unit->pcnu_name));         
            }

            if (Flags & (1 << 13))
            {
D(bug("%s: PCN32_TX_End_Int: packet reports FRAMING error\n", unit->pcnu_name));         
            }
            ReportEvents(LIBBASE, unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_TX);
         }
         else unit->pcnu_stats.PacketsSent++;

         if ((Flags & (1 << 8))||(Flags &(1 << 9))) //(ENP | STP)
         {
D(bug("%s: PCN32_TX_End_Int: freeing TxRing packet for use\n", unit->pcnu_name));
            ((struct tx_ring_desc *)np->ring_addr)[i + RX_RING_SIZE].BufferStatus = 0;
            ((struct tx_ring_desc *)np->ring_addr)[i + RX_RING_SIZE].PacketBuffer = 0;
         }
         else
         {
D(bug("%s: PCN32_TX_End_Int: TxRing packet unused ..??\n", unit->pcnu_name));         
         }
      }
   }

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
static void PCN32_TimeoutHandler(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct PCN32Unit *dev = (struct PCN32Unit *) irq->h_Data;
    struct timeval time;
    struct Device *TimerBase = dev->pcnu_TimerSlowReq->tr_node.io_Device;

    GetSysTime(&time);
//D(bug("%s: PCN32_TimeoutHandler()\n", dev->pcnu_name));

    /*
     * If timeout timer is expected, and time elapsed - regenerate the 
     * interrupt handler 
     */
    if (dev->pcnu_toutNEED && (CmpTime(&time, &dev->pcnu_toutPOLL ) < 0))
    {
        dev->pcnu_toutNEED = FALSE;
        Cause(&dev->pcnu_tx_end_int);
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
static void PCN32_IntHandler(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct PCN32Unit *dev = (struct PCN32Unit *) irq->h_Data;
    // struct fe_priv *np = dev->pcnu_fe_priv;
    // UBYTE *base = (UBYTE*) dev->pcnu_BaseMem;
    // ULONG events;
    // int i;
    struct Device *TimerBase = dev->pcnu_TimerSlowReq->tr_node.io_Device;
    struct timeval time;

    GetSysTime(&time);
    int csr_0 = 0;
    // int csr_4 = 0;
D(bug("%s: PCN32_IntHandler()!!!!!!!\n", dev->pcnu_name));
    
    while ( (csr_0 = dev->read_csr(dev->pcnu_BaseMem, 0)) & (1 << 7))
    {
       dev->write_csr(dev->pcnu_BaseMem, 0, csr_0);    
D(bug("%s: PCN32_IntHandler: csr[0] : %x\n", dev->pcnu_name, csr_0));

      if ( csr_0 & (1 << 0) ) // (INIT) is the card initialising?
      {
D(bug("%s: PCN32_IntHandler: Chipset init detected .. ", dev->pcnu_name));
         BOOL  have_Tx = FALSE, have_Rx = FALSE;

         if ( csr_0 & (1 << 1) ) // (STRT) Start/ed/ing?
         {
D(bug("[STRT]"));
         }

         if ( csr_0 & (1 << 2) ) // (STOP) Chipset is stopped
         {
D(bug("[STOP]"));
            have_Tx = TRUE;
         }

         if ( csr_0 & (1 << 4) ) // (TXON) Transmitter ON?
         {
D(bug("[TXON]"));
            have_Tx = TRUE;
         }
         else
         {
D(bug("[TXON:OFF]"));
         }

         if ( csr_0 & (1 << 5) ) // (RXON) Reciever ON?
         {
D(bug("[RXON]"));
            have_Rx = TRUE;
         }
         else
         {
D(bug("[RXON:OFF]"));
         }

         if ( csr_0 & (1 << 8) ) // (IDON) Initialisation Done?
         {
D(bug("[IDON]"));
         }
D(bug("\n"));
         if ((!(have_Tx))&&(!(have_Rx)))
         {
D(bug("%s: PCN32_IntHandler: Chipset is OFFLINE!\n", dev->pcnu_name));
         }
      }

      if ( csr_0 & (1 << 10) ) // (RINT) Chipset has Recieved packet(s)
      {
D(bug("%s: PCN32_IntHandler: Packet Reception detected!\n", dev->pcnu_name));
        Cause(&dev->pcnu_rx_int);
      }

      if ( csr_0 & (1 << 9) ) // (TINT) Chipset has Sent packet(s)
      {
D(bug("%s: PCN32_IntHandler: Packet Transmition detected!\n", dev->pcnu_name));
        Cause(&dev->pcnu_tx_end_int);
      }
      
      if ( csr_0 & (1 << 15) ) // (ERR) Chipset has Reported an ERROR
      {
D(bug("%s: PCN32_IntHandler: (ERR) ERROR Detected\n", dev->pcnu_name));
         break;
      }
    }

   return;
}

VOID CopyPacket(struct PCN32Base *PCNet32Base, struct PCN32Unit *unit, 
    struct IOSana2Req *request, UWORD packet_size, UWORD packet_type,
    struct eth_frame *buffer)
{
    struct Opener *opener;
    BOOL filtered = FALSE;
    UBYTE *ptr;
    const UBYTE broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

D(bug("%s: CopyPacket(packet @ %x, len = %d)\n", unit->pcnu_name, buffer, packet_size));

    /* Set multicast and broadcast flags */

    request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
    if(memcmp(buffer->eth_packet_dest, broadcast, 6) == 0)
    {
       request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
D(bug("%s: CopyPacket: BROADCAST Flag set\n", unit->pcnu_name));
    }
    else if((buffer->eth_packet_dest[0] & 0x1) != 0)
    {
       request->ios2_Req.io_Flags |= SANA2IOF_MCAST;
D(bug("%s: CopyPacket: MULTICAST Flag set\n", unit->pcnu_name));
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

D(bug("%s: CopyPacket: packet @ %x (%d bytes)\n", unit->pcnu_name, ptr, packet_size));

    /* Filter packet */

    opener = request->ios2_BufferManagement;
    if((request->ios2_Req.io_Command == CMD_READ) &&
        (opener->filter_hook != NULL))
        if(!CallHookPkt(opener->filter_hook, request, ptr))
        {
D(bug("%s: CopyPacket: packet filtered\n", unit->pcnu_name));
            filtered = TRUE;
        }

    if(!filtered)
    {
        /* Copy packet into opener's buffer and reply packet */
D(bug("%s: CopyPacket: opener recieve packet .. ", unit->pcnu_name));
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
D(bug("%s: CopyPacket: opener notified.\n", unit->pcnu_name));
    }
}

BOOL AddressFilter(struct PCN32Base *PCNet32Base, struct PCN32Unit *unit, UBYTE *address)
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

        range = (APTR)unit->pcnu_multicast_ranges.mlh_Head;
        tail = (APTR)&unit->pcnu_multicast_ranges.mlh_Tail;
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
            unit->pcnu_special_stats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    }
    return accept;
}

/*
 * Unit process
 */
AROS_UFH3(void, PCN32_Schedular,
    AROS_UFHA(STRPTR,              argPtr, A0),
    AROS_UFHA(ULONG,               argSize, D0),
    AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct PCN32Unit *dev = FindTask(NULL)->tc_UserData;
    LIBBASETYPEPTR LIBBASE = dev->pcnu_device;
    struct MsgPort *reply_port, *input;

D(bug("[pcnet32] PCN32_Schedular()\n"));
D(bug("[pcnet32] PCN32_Schedular: Setting device up\n"));

    reply_port = CreateMsgPort();
    input = CreateMsgPort();

    dev->pcnu_input_port = input; 

    dev->pcnu_TimerSlowPort = CreateMsgPort();

    if (dev->pcnu_TimerSlowPort)
    {
        dev->pcnu_TimerSlowReq = (struct timerequest *)
            CreateIORequest((struct MsgPort *)dev->pcnu_TimerSlowPort, sizeof(struct timerequest));

        if (dev->pcnu_TimerSlowReq)
        {
            if (!OpenDevice("timer.device", UNIT_VBLANK,
                (struct IORequest *)dev->pcnu_TimerSlowReq, 0))
            {
                struct Message *msg = AllocVec(sizeof(struct Message), MEMF_PUBLIC|MEMF_CLEAR);
                ULONG sigset;

D(bug("[pcnet32] PCN32_Schedular: Got VBLANK unit of timer.device\n"));

                dev->initialize(dev);

                msg->mn_ReplyPort = reply_port;
                msg->mn_Length = sizeof(struct Message);

D(bug("[pcnet32] PCN32_Schedular: Setup complete. Sending handshake\n"));
                PutMsg(LIBBASE->pcnb_syncport, msg);
                WaitPort(reply_port);
                GetMsg(reply_port);

                FreeVec(msg);

D(bug("[pcnet32] PCN32_Schedular: entering forever loop ... \n"));

                dev->pcnu_signal_0 = AllocSignal(-1);
                dev->pcnu_signal_1 = AllocSignal(-1);
                dev->pcnu_signal_2 = AllocSignal(-1);
                dev->pcnu_signal_3 = AllocSignal(-1);

                sigset = 1 << input->mp_SigBit  |
                         1 << dev->pcnu_signal_0  |
                         1 << dev->pcnu_signal_1  |
                         1 << dev->pcnu_signal_2  |
                         1 << dev->pcnu_signal_3;
                for(;;)
                {	
                    ULONG recvd = Wait(sigset);
                    if (recvd & dev->pcnu_signal_0)
                    {
                        /*
                         * Shutdown process. Driver should close everything 
                         * already and waits for our process to complete. Free
                         * memory allocared here and kindly return.
                         */
                        dev->deinitialize(dev);
                        CloseDevice((struct IORequest *)dev->pcnu_TimerSlowReq);
                        DeleteIORequest((struct IORequest *)dev->pcnu_TimerSlowReq);
                        DeleteMsgPort(dev->pcnu_TimerSlowPort);
                        DeleteMsgPort(input);
                        DeleteMsgPort(reply_port);

D(bug("[pcnet32] PCN32_Schedular: Process shutdown.\n"));
                        return;
                    }
                    else if (recvd & (1 << input->mp_SigBit))
                    {
                        struct IOSana2Req *io;

                        /* Handle incoming transactions */
                        while ((io = (struct IOSana2Req *)GetMsg(input))!= NULL);
                        {
D(bug("[pcnet32] PCN32_Schedular: Handle incomming transaction.\n"));
                            ObtainSemaphore(&dev->pcnu_unit_lock);
                            handle_request(LIBBASE, io);
                        }
                    }
                    else
                    {
D(bug("[pcnet32] PCN32_Schedular: Handle incomming signal.\n"));
                        /* Handle incoming signals */
                    }
                }
            }
        }
    }

    AROS_USERFUNC_EXIT
}

/*
 * Create new pcnet32 ethernet device unit
 */
struct PCN32Unit *CreateUnit(struct PCN32Base *PCNet32Base, OOP_Object *pciDevice)
{
    struct PCN32Unit *unit = AllocMem(sizeof(struct PCN32Unit), MEMF_PUBLIC | MEMF_CLEAR);
    BOOL success = TRUE;
    int i;

D(bug("[pcnet32] CreateUnit()\n"));

    if (unit != NULL)
    {
        IPTR        DeviceID, base, len;
        OOP_Object  *driver;

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);

        unit->pcnu_device     = PCNet32Base;
        unit->pcnu_DeviceID   = DeviceID;
        unit->pcnu_mtu        = ETH_MTU;
        unit->pcnu_PCIDevice  = pciDevice;
        unit->pcnu_PCIDriver  = driver;

        InitSemaphore(&unit->pcnu_unit_lock);
        NEWLIST(&unit->pcnu_Openers);
        NEWLIST(&unit->pcnu_multicast_ranges);
        NEWLIST(&unit->pcnu_type_trackers);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &unit->pcnu_IRQ);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base1,   &unit->pcnu_BaseIO);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0,   &base);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0,   &len);

        unit->pcnu_BaseMem = HIDD_PCIDriver_MapPCI(driver, (APTR)base, len);
        unit->pcnu_SizeMem = len;

        if (unit->pcnu_BaseMem)
        {
            struct TagItem attrs[] = {
                { aHidd_PCIDevice_isIO,     TRUE },
                { aHidd_PCIDevice_isMEM,    TRUE },
                { aHidd_PCIDevice_isMaster, TRUE },
                { TAG_DONE,                 0    },
            };
            OOP_SetAttrs(pciDevice, (struct TagItem *)&attrs);

            unit->pcnu_name = "[pcnet32.0]";

            unit->pcnu_fe_priv = AllocMem(sizeof(struct fe_priv), MEMF_PUBLIC|MEMF_CLEAR);

            unit->pcnu_fe_priv->fep_pcnet_init_block = HIDD_PCIDriver_AllocPCIMem(
                    driver,
                    sizeof(struct pcnet32_init_block));

            unit->pcnu_UnitNum = 0;

            pcn32_get_functions(unit);

            if (unit->pcnu_fe_priv)
            {
                unit->pcnu_fe_priv->pci_dev = unit;
                InitSemaphore(&unit->pcnu_fe_priv->lock);

                unit->pcnu_irqhandler = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC|MEMF_CLEAR);
                unit->pcnu_touthandler = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC|MEMF_CLEAR);

                if (unit->pcnu_irqhandler && unit->pcnu_touthandler)
                {
                    struct Message *msg;

                    unit->pcnu_irqhandler->h_Node.ln_Pri = 100;
                    unit->pcnu_irqhandler->h_Node.ln_Name = LIBBASE->pcnb_Device.dd_Library.lib_Node.ln_Name;
                    unit->pcnu_irqhandler->h_Code = PCN32_IntHandler;
                    unit->pcnu_irqhandler->h_Data = unit;

                    unit->pcnu_touthandler->h_Node.ln_Pri = 100;
                    unit->pcnu_touthandler->h_Node.ln_Name = LIBBASE->pcnb_Device.dd_Library.lib_Node.ln_Name;
                    unit->pcnu_touthandler->h_Code = PCN32_TimeoutHandler;
                    unit->pcnu_touthandler->h_Data = unit;

                    unit->pcnu_rx_int.is_Node.ln_Name = unit->pcnu_name;
                    unit->pcnu_rx_int.is_Code = PCN32_RX_Int;
                    unit->pcnu_rx_int.is_Data = unit;

                    unit->pcnu_tx_int.is_Node.ln_Name = unit->pcnu_name;
                    unit->pcnu_tx_int.is_Code = PCN32_TX_Int;
                    unit->pcnu_tx_int.is_Data = unit;

                    unit->pcnu_tx_end_int.is_Node.ln_Name = unit->pcnu_name;
                    unit->pcnu_tx_end_int.is_Code = PCN32_TX_End_Int;
                    unit->pcnu_tx_end_int.is_Data = unit;

                    for (i=0; i < REQUEST_QUEUE_COUNT; i++)
                    {
                        struct MsgPort *port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);
                        unit->pcnu_request_ports[i] = port;

                        if (port == NULL) success = FALSE;

                        if (success)
                        {
                            NEWLIST(&port->mp_MsgList);
                            port->mp_Flags = PA_IGNORE;
                            port->mp_SigTask = &unit->pcnu_tx_int;
                        }
                    }

                    unit->pcnu_request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

                    if (success)
                    {
D(bug("%s: Initialise Dev @ %x, IOBase @ %x\n", unit->pcnu_name, unit, unit->pcnu_BaseMem));

                       pcnet32_reset_16(unit->pcnu_BaseMem);

D(bug("%s: Chipset RESET\n", unit->pcnu_name));

                       i = pcnet32_readcsr_16(unit->pcnu_BaseMem, 0);        // Check for 16bit/32bit chip IO
                       BOOL check = pcnet32_check_16(unit->pcnu_BaseMem);
   
                       if ((i == 4)&&(check))
                       {
D(bug("%s: Using 16bit I/O Funcs\n", unit->pcnu_name));
                          unit->read_csr = pcnet32_readcsr_16;
                          unit->write_csr = pcnet32_writecsr_16;
                          unit->read_bcr = pcnet32_readbcr_16;
                          unit->write_bcr = pcnet32_writebcr_16;
                          unit->read_rap = pcnet32_readrap_16;
                          unit->write_rap = pcnet32_writerap_16;
                          unit->reset = pcnet32_reset_16;
                       }
                       else
                       {
                          pcnet32_reset_32(unit->pcnu_BaseMem);             // 32bit reset..
      
                          i = pcnet32_readcsr_32(unit->pcnu_BaseMem, 0);
                          check = pcnet32_check_32(unit->pcnu_BaseMem);

                          if ((i == 4)&&(check))
                          {
D(bug("%s: Using 32bit I/O Funcs\n", unit->pcnu_name));
                             unit->read_csr = pcnet32_readcsr_32;
                             unit->write_csr = pcnet32_writecsr_32;
                             unit->read_bcr = pcnet32_readbcr_32;
                             unit->write_bcr = pcnet32_writebcr_32;
                             unit->read_rap = pcnet32_readrap_32;
                             unit->write_rap = pcnet32_writerap_32;
                             unit->reset = pcnet32_reset_32;
                          }
                          else
                          {
D(bug("%s: Error - Unsupported chipset .. (unknown data size)\n", unit->pcnu_name));
                             success = FALSE;
                          }
                       }

                       if (success)
                       {
                          i = (unit->read_csr(unit->pcnu_BaseMem, 88) | ( unit->read_csr(unit->pcnu_BaseMem, 89) << 16));

                          unit->pcnu_pcnet_chiprevision = (i >> 12) & 0xffff;
   
D(bug("%s: PCnet chip version %x [%x]\n", unit->pcnu_name, i, unit->pcnu_pcnet_chiprevision));

                          unit->pcnu_pcnet_supported = 0;

                          switch (unit->pcnu_pcnet_chiprevision)
                          {
                          case    0x2420:
                          case    0x2430:
                             unit->pcnu_pcnet_chipname = "PCnet/PCI 79c970";
                             break;

                          case    0x2621:
                             unit->pcnu_pcnet_chipname = "PCnet/PCI II 79c970A";
                             unit->pcnu_pcnet_supported |= support_fdx;
                             break;

                          case    0x2623:
                             unit->pcnu_pcnet_chipname = "PCnet/FAST 79c971";
                             unit->pcnu_pcnet_supported |= (support_fdx | support_mii | support_fset | support_ltint );
                             break;

                          case    0x2624:
                             unit->pcnu_pcnet_chipname = "PCnet/FAST+ 79c972";
                             unit->pcnu_pcnet_supported |= (support_fdx | support_mii | support_fset );
                             break;
      
                          case    0x2625:
                             unit->pcnu_pcnet_chipname = "PCnet/FAST III 79c973";
                             unit->pcnu_pcnet_supported |= (support_fdx | support_mii );
                             break;

                          case    0x2627:
                             unit->pcnu_pcnet_chipname = "PCnet/FAST III 79c975";
                             unit->pcnu_pcnet_supported |= (support_fdx | support_mii );
                             break;

                          case    0x2626:
                             unit->pcnu_pcnet_chipname = "PCnet/Home 79c978";
                             unit->pcnu_pcnet_supported |= support_fdx;

/* TODO: PCnet/Home needs extra set up .. */
                             break;
                          default:
D(bug("%s: ERROR - Unsupported Chipset (unknown revision)\n", unit->pcnu_name));
                             success = FALSE;
                          }
#if defined(DEBUG)
D(bug("%s: Found %s chipset based NIC\n", unit->pcnu_name, unit->pcnu_pcnet_chipname));
if (unit->pcnu_pcnet_supported & support_fdx)
   D(bug("%s: Chip Supports Full Duplex\n", unit->pcnu_name));
if (unit->pcnu_pcnet_supported & support_mii)
   D(bug("%s: Chip Supports MII\n", unit->pcnu_name));
if (unit->pcnu_pcnet_supported & support_fset)
   D(bug("%s: Chip Supports FSET\n", unit->pcnu_name));
if (unit->pcnu_pcnet_supported & support_ltint)
   D(bug("%s: Chip Supports LTINT\n", unit->pcnu_name));
#endif

                          if (((unit->pcnu_pcnet_chiprevision +1) & 0xfffe) == 0x2624)
                          {
                             i = unit->read_csr(unit->pcnu_BaseMem, 80) & 0x0c00;      /* Check tx_start_pt */
#if defined(DEBUG)
D(bug("%s:    tx_start_pt(0x%hX):", unit->pcnu_name, i));
switch(i >> 10)
{
case 0:
D(bug("  20 bytes,"));
break;
case 1:
D(bug("  64 bytes,"));
break;
case 2:
D(bug(" 128 bytes,"));
break;
case 3:
D(bug("~220 bytes,"));
break;
}
#endif
                             i = unit->read_bcr(unit->pcnu_BaseMem, 18);      /* Check burst/bus control */
#if defined(DEBUG)
D(bug(" BCR18(%hX):", i & 0xffff));
if (i & (1 << 5))
   D(bug("BurstWrEn "));
if (i & (1 << 6))
   D(bug("BurstRdEn "));
if (i & (1 << 7))
   D(bug("32bitIO "));
if (i & (1 << 11))
   D(bug("NoUFlow "));
#endif
                             i = unit->read_bcr(unit->pcnu_BaseMem, 25);
D(bug("    SRAMSIZE=0x%hX,", i << 8));
                             i = unit->read_bcr(unit->pcnu_BaseMem, 26);
D(bug(" SRAM_BND=0x%hX,", i << 8));
                             i = unit->read_bcr(unit->pcnu_BaseMem, 27);
#if defined(DEBUG)
if (i & (1 << 14))
   D(bug("LowLatRx"));
#endif
D(bug("\n"));
                          }
                       }
                    }

                    if (success)
                    {
                        LIBBASE->pcnb_syncport = CreateMsgPort();

                        unit->pcnu_Process = CreateNewProcTags(
                                                NP_Entry, (IPTR)PCN32_Schedular,
                                                NP_Name, PCNET32_TASK_NAME,
                                                NP_Priority, 0,
                                                NP_UserData, (IPTR)unit,
                                                NP_StackSize, 140960,
                                                TAG_DONE);

                        WaitPort(LIBBASE->pcnb_syncport);
                        msg = GetMsg(LIBBASE->pcnb_syncport);
                        ReplyMsg(msg);
                        DeleteMsgPort(LIBBASE->pcnb_syncport);

D(bug("[pcnet32] Unit up and running\n"));

                        return unit;
                    }
                    else
                    {
D(bug("%s: ERRORS occured during Device setup - ABORTING\n", unit->pcnu_name));
                    }
                }
            }
        }
        else
D(bug("[pcnet32] PANIC! Couldn't get MMIO area. Aborting\n"));
    }
    DeleteUnit(PCNet32Base, unit);	
    return NULL;
}

/*
 * DeleteUnit - removes selected unit. Frees all resources and structures.
 * 
 * The caller should be sure, that given unit is really ready to be freed.
 */

void DeleteUnit(struct PCN32Base *PCNet32Base, struct PCN32Unit *Unit)
{
    int i;
    if (Unit)
    {
        if (Unit->pcnu_Process)
        {
            Signal(&Unit->pcnu_Process->pr_Task, Unit->pcnu_signal_0);
        }

        for (i=0; i < REQUEST_QUEUE_COUNT; i++)
        {
            if (Unit->pcnu_request_ports[i] != NULL) 
                FreeMem(Unit->pcnu_request_ports[i],	sizeof(struct MsgPort));

            Unit->pcnu_request_ports[i] = NULL;
        }

        if (Unit->pcnu_irqhandler)
        {
            FreeMem(Unit->pcnu_irqhandler, sizeof(HIDDT_IRQ_Handler));
            LIBBASE->pcnb_irq = NULL;
        }

        if (Unit->pcnu_fe_priv)
        {
            FreeMem(Unit->pcnu_fe_priv, sizeof(struct fe_priv));
            Unit->pcnu_fe_priv = NULL;
        }

        if (Unit->pcnu_BaseMem)
        {
            HIDD_PCIDriver_UnmapPCI(Unit->pcnu_PCIDriver, 
                                    (APTR)Unit->pcnu_BaseMem,
                                    Unit->pcnu_SizeMem);
        }

        FreeMem(Unit, sizeof(struct PCN32Unit));
    }
}

static struct AddressRange *FindMulticastRange(LIBBASETYPEPTR LIBBASE, struct PCN32Unit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left, UWORD upper_bound_right)
{
    struct AddressRange *range, *tail;
    BOOL found = FALSE;

    range = (APTR)unit->pcnu_multicast_ranges.mlh_Head;
    tail = (APTR)&unit->pcnu_multicast_ranges.mlh_Tail;

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

BOOL AddMulticastRange(LIBBASETYPEPTR LIBBASE, struct PCN32Unit *unit, const UBYTE *lower_bound,
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
            AddTail((APTR)&unit->pcnu_multicast_ranges, (APTR)range);
            Enable();

            if (unit->pcnu_range_count++ == 0)
            {
                unit->pcnu_flags |= IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }

    return range != NULL;
}

BOOL RemMulticastRange(LIBBASETYPEPTR LIBBASE, struct PCN32Unit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound)
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

            if (--unit->pcnu_range_count == 0)
            {
                unit->pcnu_flags &= ~IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }
    return range != NULL;
}

