/*

Copyright (C) 2001-2005 Neil Cafferkey

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


#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/errors.h>

#include <proto/exec.h>
#ifndef __amigaos4__
#include <proto/alib.h>
#else
#include <clib/alib_protos.h>
#endif
#include <proto/utility.h>
#include <proto/timer.h>

#include "device.h"
#include "etherlink3.h"
#include "mii.h"

#include "unit_protos.h"
#include "request_protos.h"


#define TASK_PRIORITY 0
#define STACK_SIZE 4096
#define INT_MASK \
   (EL3INTF_UPDATESTATS | EL3INTF_RXCOMPLETE | EL3INTF_TXAVAIL \
   | EL3INTF_TXCOMPLETE)
#define DMA_INT_MASK \
   (EL3INTF_UPDONE | EL3INTF_DOWNDONE | EL3INTF_UPDATESTATS \
   | EL3INTF_TXCOMPLETE)
#define PORT_COUNT 6


#ifndef AbsExecBase
#define AbsExecBase (*(struct ExecBase **)4)
#endif
#ifdef __amigaos4__
#undef AddTask
#define AddTask(task, initial_pc, final_pc) \
   IExec->AddTask(task, initial_pc, final_pc, NULL)
#endif

VOID SelectMedium(struct DevUnit *unit, UWORD transceiver,
   struct DevBase *base);
static struct AddressRange *FindMulticastRange(struct DevUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
   UWORD upper_bound_right, struct DevBase *base);
static VOID RXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID CopyPacket(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD packet_size, UWORD packet_type, UBYTE *buffer, BOOL all_read,
   struct DevBase *base);
static BOOL AddressFilter(struct DevUnit *unit, UBYTE *address,
   struct DevBase *base);
static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID TxError(struct DevUnit *unit, struct DevBase *base);
static VOID DMARXInt(REG(a1, struct DevUnit *unit),
   REG(a5, APTR int_code));
static VOID DMATXInt(REG(a1, struct DevUnit *unit),
   REG(a5, APTR int_code));
static VOID DMATXEndInt(REG(a1, struct DevUnit *unit),
   REG(a5, APTR int_code));
static VOID ReportEvents(struct DevUnit *unit, ULONG events,
   struct DevBase *base);
static VOID UnitTask();
static UWORD ReadEEPROM(struct DevUnit *unit, UWORD index,
   struct DevBase *base);
static BOOL ReadMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   UWORD *value, struct DevBase *base);
#if 0
static VOID WriteMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   UWORD value, struct DevBase *base);
#endif
static ULONG ReadMIIBits(struct DevUnit *unit, UBYTE count,
   struct DevBase *base);
static VOID WriteMIIBits(struct DevUnit *unit, ULONG value, UBYTE count,
   struct DevBase *base);
static BOOL ReadMIIBit(struct DevUnit *unit, struct DevBase *base);
static VOID WriteMIIBit(struct DevUnit *unit, BOOL is_one,
   struct DevBase *base);
#if 0
static VOID DoMIIZCycle(struct DevUnit *unit, struct DevBase *base);
#endif
static VOID BusyMicroDelay(ULONG micros, struct DevBase *base);


static const UBYTE port_choices[] =
{
/*   EL3XCVR_AUTONEG,*/
   EL3XCVR_100BASETX,
   EL3XCVR_10BASET,
   EL3XCVR_MII,
   EL3XCVR_100BASEFX,
   EL3XCVR_10BASE2,
   EL3XCVR_AUI
};

static const UBYTE port_masks[] =
{
   EL3REG_MEDIAOPTIONSF_10BASET,
   EL3REG_MEDIAOPTIONSF_AUI,
   0,
   EL3REG_MEDIAOPTIONSF_10BASE2,
   EL3REG_MEDIAOPTIONSF_100BASETX,
   EL3REG_MEDIAOPTIONSF_100BASEFX,
   EL3REG_MEDIAOPTIONSF_MII,
   0,
   EL3REG_MEDIAOPTIONSF_100BASETX | EL3REG_MEDIAOPTIONSF_10BASET
};


/****i* etherlink3.device/CreateUnit ***************************************
*
*   NAME
*	CreateUnit -- Create a unit.
*
*   SYNOPSIS
*	unit = CreateUnit(index, card, io_tags, generation,
*	    bus)
*
*	struct DevUnit *CreateUnit(ULONG, APTR, struct TagItem *, UWORD,
*	    UWORD);
*
*   FUNCTION
*	Creates a new unit.
*
****************************************************************************
*
*/

struct DevUnit *CreateUnit(ULONG index, APTR card,
   const struct TagItem *io_tags, UWORD generation, UWORD bus,
   struct DevBase *base)
{
   BOOL success = TRUE;
   struct DevUnit *unit;
   struct Task *task;
   struct MsgPort *port;
   UBYTE i, *buffer;
   APTR stack;
   ULONG *upd, *next_upd, *fragment, dma_size;
   APTR rx_int_function, tx_int_function;

   unit = AllocMem(sizeof(struct DevUnit), MEMF_CLEAR | MEMF_PUBLIC);
   if(unit == NULL)
      success = FALSE;

   if(success)
   {
      InitSemaphore(&unit->access_lock);
      NewList((APTR)&unit->openers);
      NewList((APTR)&unit->type_trackers);
      NewList((APTR)&unit->multicast_ranges);

      unit->index = index;
      unit->device = base;
      unit->card = card;
      unit->generation = generation;
      unit->bus = bus;
      unit->ByteIn =
         (APTR)GetTagData(IOTAG_ByteIn, (UPINT)NULL, io_tags);
      unit->LongIn =
         (APTR)GetTagData(IOTAG_LongIn, (UPINT)NULL, io_tags);
      unit->ByteOut =
         (APTR)GetTagData(IOTAG_ByteOut, (UPINT)NULL, io_tags);
      unit->WordOut =
         (APTR)GetTagData(IOTAG_WordOut, (UPINT)NULL, io_tags);
      unit->LongOut =
         (APTR)GetTagData(IOTAG_LongOut, (UPINT)NULL, io_tags);
      unit->LongsIn =
         (APTR)GetTagData(IOTAG_LongsIn, (UPINT)NULL, io_tags);
      unit->LongsOut =
         (APTR)GetTagData(IOTAG_LongsOut, (UPINT)NULL, io_tags);
      unit->BEWordOut =
         (APTR)GetTagData(IOTAG_BEWordOut, (UPINT)NULL, io_tags);
      unit->LEWordIn =
         (APTR)GetTagData(IOTAG_LEWordIn, (UPINT)NULL, io_tags);
      unit->LELongIn =
         (APTR)GetTagData(IOTAG_LELongIn, (UPINT)NULL, io_tags);
      unit->LEWordOut =
         (APTR)GetTagData(IOTAG_LEWordOut, (UPINT)NULL, io_tags);
      unit->LELongOut =
         (APTR)GetTagData(IOTAG_LELongOut, (UPINT)NULL, io_tags);
      unit->AllocDMAMem =
         (APTR)GetTagData(IOTAG_AllocDMAMem, (UPINT)NULL, io_tags);
      unit->FreeDMAMem =
         (APTR)GetTagData(IOTAG_FreeDMAMem, (UPINT)NULL, io_tags);
      if(unit->ByteIn == NULL || unit->LongIn == NULL
         || unit->ByteOut == NULL
         || unit->WordOut == NULL || unit->LongOut == NULL
         || unit->LongsIn == NULL || unit->LongsOut == NULL
         || unit->BEWordOut == NULL || unit->LEWordIn == NULL
         || unit->LELongIn == NULL || unit->LEWordOut == NULL
         || unit->LELongOut == NULL
         || generation >= BOOMERANG_GEN
         && (unit->AllocDMAMem == NULL || unit->FreeDMAMem == NULL))
         success = FALSE;
   }

   if(success)
   {
      if(unit->generation >= VORTEX_GEN)
         unit->size_shift = 2;

      InitialiseAdapter(unit, FALSE, base);
      unit->flags |= UNITF_HAVEADAPTER;

      /* Set up packet filter command */

      unit->rx_filter_cmd = EL3CMD_SETRXFILTER | EL3CMD_SETRXFILTERF_BCAST
         | EL3CMD_SETRXFILTERF_UCAST;

      /* Set up interrupt mask */

      if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
         unit->int_mask = DMA_INT_MASK;
      else
         unit->int_mask = INT_MASK;

      /* Disable statistics interrupts for PCMCIA because they can't be
         cleared on A1200 */

      if(bus == PCCARD_BUS)
         unit->int_mask &= ~EL3INTF_UPDATESTATS;

      /* Store location of registers that were originally in window 1 */

      if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
         unit->window1_offset += EL3_WINDOWSIZE;

      /* Create the message ports for queuing requests */

      for(i = 0; i < REQUEST_QUEUE_COUNT; i++)
      {
         unit->request_ports[i] = port = AllocMem(sizeof(struct MsgPort),
            MEMF_PUBLIC | MEMF_CLEAR);
         if(port == NULL)
            success = FALSE;

         if(success)
         {
            NewList(&port->mp_MsgList);
            port->mp_Flags = PA_IGNORE;
            port->mp_SigTask = &unit->tx_int;
         }
      }

      if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
      {
         unit->rx_buffer = unit->AllocDMAMem(unit->card,
            ETH_MAXPACKETSIZE * RX_SLOT_COUNT, 1);
         unit->tx_buffer =
            unit->AllocDMAMem(unit->card, ETH_MAXPACKETSIZE, 1);
      }
      else
      {
         unit->rx_buffer =
            AllocVec((ETH_MAXPACKETSIZE + 3) & ~3, MEMF_PUBLIC);
         unit->tx_buffer = AllocVec(ETH_MAXPACKETSIZE, MEMF_PUBLIC);
      }

      if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
      {
         unit->tx_requests = AllocVec(sizeof(APTR) * TX_SLOT_COUNT,
            MEMF_PUBLIC);
         unit->headers = unit->AllocDMAMem(unit->card,
            ETH_HEADERSIZE * TX_SLOT_COUNT, 1);
         unit->dpds =
            unit->AllocDMAMem(unit->card, DPD_SIZE * TX_SLOT_COUNT, 8);
         next_upd = unit->upds =
            unit->AllocDMAMem(unit->card, UPD_SIZE * RX_SLOT_COUNT, 8);
         if(unit->tx_requests == NULL || unit->headers == NULL
            || unit->dpds == NULL || next_upd == NULL)
            success = FALSE;
      }

      if(unit->rx_buffer == NULL || unit->tx_buffer == NULL)
         success = FALSE;
   }

   if(success)
   {
      if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
      {
         /* Construct RX ring */

         buffer = unit->rx_buffer;
         for(i = 0; i < RX_SLOT_COUNT; i++)
         {
            upd = next_upd;
            next_upd = upd + UPD_SIZE / sizeof(ULONG);
            upd[EL3UPD_NEXT] = MakeLELong((ULONG)next_upd);
            upd[EL3UPD_STATUS] = 0;
            fragment = upd + EL3UPD_FIRSTFRAG;
            fragment[EL3FRAG_ADDR] = MakeLELong((ULONG)buffer);
            fragment[EL3FRAG_LEN] =
               MakeLELong(EL3FRAG_LENF_LAST | ETH_MAXPACKETSIZE);
            buffer += ETH_MAXPACKETSIZE;
         }
         upd[EL3UPD_NEXT] = MakeLELong((ULONG)unit->upds);
         unit->next_upd = unit->upds;

         dma_size = UPD_SIZE * RX_SLOT_COUNT;
         CachePreDMA(unit->upds, &dma_size, 0);
         dma_size = ETH_MAXPACKETSIZE * RX_SLOT_COUNT;
         CachePreDMA(unit->rx_buffer, &dma_size, 0);
      }

      /* Record maximum speed in BPS */

      if((unit->capabilities & EL3ROM_CAPABILITIESF_100MBPS) != 0)
         unit->speed = 100000000;
      else
         unit->speed = 10000000;

      /* Initialise status, transmit and receive interrupts */

      unit->status_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->status_int.is_Code = (APTR)StatusInt;
      unit->status_int.is_Data = unit;

      if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
         rx_int_function = DMARXInt;
      else
         rx_int_function = RXInt;
      unit->rx_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->rx_int.is_Code = rx_int_function;
      unit->rx_int.is_Data = unit;

      if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
         tx_int_function = DMATXInt;
      else
         tx_int_function = TXInt;
      unit->tx_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->tx_int.is_Code = tx_int_function;
      unit->tx_int.is_Data = unit;

      unit->tx_end_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->tx_end_int.is_Code = DMATXEndInt;
      unit->tx_end_int.is_Data = unit;

      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
   }

   if(success)
   {
      /* Create a new task */

      unit->task = task =
         AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
      if(task == NULL)
         success = FALSE;
   }

   if(success)
   {
      stack = AllocMem(STACK_SIZE, MEMF_PUBLIC);
      if(stack == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Initialise and start task */

      task->tc_Node.ln_Type = NT_TASK;
      task->tc_Node.ln_Pri = TASK_PRIORITY;
      task->tc_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      task->tc_SPUpper = stack + STACK_SIZE;
      task->tc_SPLower = stack;
      task->tc_SPReg = stack + STACK_SIZE;
      NewList(&task->tc_MemEntry);

      if(AddTask(task, UnitTask, NULL) == NULL)
         success = FALSE;
   }

   /* Send the unit to the new task */

   if(success)
      task->tc_UserData = unit;

   if(!success)
   {
      DeleteUnit(unit, base);
      unit = NULL;
   }

   return unit;
}



/****i* etherlink3.device/DeleteUnit ***************************************
*
*   NAME
*	DeleteUnit -- Delete a unit.
*
*   SYNOPSIS
*	DeleteUnit(unit)
*
*	VOID DeleteUnit(struct DevUnit *);
*
*   FUNCTION
*	Deletes a unit.
*
*   INPUTS
*	unit - Device unit (can be NULL).
*
*   RESULT
*	None.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

VOID DeleteUnit(struct DevUnit *unit, struct DevBase *base)
{
   UBYTE i;
   struct Task *task;

   if(unit != NULL)
   {
      task = unit->task;
      if(task != NULL)
      {
         if(task->tc_UserData != NULL)
         {
            RemTask(task);
            FreeMem(task->tc_SPLower, STACK_SIZE);
         }
         FreeMem(task, sizeof(struct Task));
      }

      for(i = 0; i < REQUEST_QUEUE_COUNT; i++)
      {
         if(unit->request_ports[i] != NULL)
            FreeMem(unit->request_ports[i], sizeof(struct MsgPort));
      }

      if((unit->flags & UNITF_HAVEADAPTER) != 0)   /* Needed! */
         GoOffline(unit, base);

      if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
      {
         unit->FreeDMAMem(unit->card, unit->upds);
         unit->FreeDMAMem(unit->card, unit->dpds);
         unit->FreeDMAMem(unit->card, unit->headers);
         FreeVec(unit->tx_requests);
      }

      if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
      {
         unit->FreeDMAMem(unit->card, unit->tx_buffer);
         unit->FreeDMAMem(unit->card, unit->rx_buffer);
      }
      else
      {
         FreeVec(unit->tx_buffer);
         FreeVec(unit->rx_buffer);
      }

      FreeMem(unit, sizeof(struct DevUnit));
   }

   return;
}



/****i* etherlink3.device/InitialiseAdapter ********************************
*
*   NAME
*	InitialiseAdapter -- .
*
*   SYNOPSIS
*	InitialiseAdapter(unit, reinsertion)
*
*	BOOL InitialiseAdapter(struct DevUnit *, BOOL);
*
*   FUNCTION
*
*   INPUTS
*	unit
*	reinsertion
*
*   RESULT
*	None.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

BOOL InitialiseAdapter(struct DevUnit *unit, BOOL reinsertion,
   struct DevBase *base)
{
   UBYTE *p, i;
   UWORD address_part, links = 0, ports, new_ports, tp_ports,
      media_status, transceiver, status, advert, ability, modes;
   ULONG config;
   BOOL autoselect;

   /* Reset card. We avoid resetting the receive logic because it stops the
      link status working in the MII Status register */

   unit->LEWordOut(unit->card, EL3REG_COMMAND,
      EL3CMD_RXRESET | EL3CMD_RXRESETF_SKIPNETWORK);
   while((unit->LEWordIn(unit->card, EL3REG_STATUS) &
      EL3REG_STATUSF_CMDINPROGRESS) != 0);
   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_TXRESET);
   while((unit->LEWordIn(unit->card, EL3REG_STATUS) &
      EL3REG_STATUSF_CMDINPROGRESS) != 0);

   /* Select IO addresses and interrupt for PCCard */

   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 0);
   if(unit->bus == PCCARD_BUS)
      unit->LEWordOut(unit->card, EL3REG_RESCONFIG, 0x3f00);
   if(unit->bus == ISA_BUS)
      unit->LEWordOut(unit->card, EL3REG_RESCONFIG, (10 << 12) | 0xf00);

   /* Fully enable an ISA card */

   if(unit->bus == ISA_BUS)
      unit->LEWordOut(unit->card, EL3REG_CONFIG, EL3REG_CONFIGF_ENABLE);

   /* Get card capabilities */

   unit->capabilities = ReadEEPROM(unit, EL3ROM_CAPABILITIES, base);

   /* Get default MAC address */

   p = unit->default_address;

   for(i = 0; i < ETH_ADDRESSSIZE / 2; i++)
   {
      address_part = ReadEEPROM(unit, EL3ROM_ALTADDRESS0 + i, base);
      *p++ = address_part >> 8;
      *p++ = address_part & 0xff;
   }

   /* Get available transceivers */

   if(unit->bus == PCCARD_BUS || unit->bus == ISA_BUS)
   {
      config = unit->LEWordIn(unit->card, EL3REG_CONFIG);
      ports = (config >> 8) &
         (EL3REG_MEDIAOPTIONSF_AUI | EL3REG_MEDIAOPTIONSF_10BASE2);
      if((config & EL3REG_CONFIGF_10BASET) != 0)
         ports |= EL3REG_MEDIAOPTIONSF_10BASET;
   }
   else
   {
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 3);
      ports = unit->LEWordIn(unit->card, EL3REG_MEDIAOPTIONS);
#if 0
      if(ports == 0)
         ports = EL3REG_MEDIAOPTIONSF_MII;   /* fix for 3c916? */
#endif
   }

   /* Get transceiver choice from EEPROM */

   if(unit->bus == PCI_BUS)
   {
      config = unit->LELongIn(unit->card, EL3REG_INTERNALCONFIG);
      transceiver =
         (config & EL3REG_INTERNALCONFIGF_XCVR)
         >> EL3REG_INTERNALCONFIGB_XCVR;
      autoselect = (config & EL3REG_INTERNALCONFIGF_AUTOXCVR) != 0;
   }
   else
   {
      config = ReadEEPROM(unit, EL3ROM_ADDRCONFIG, base);
      transceiver =
         (config & EL3REG_ADDRCONFIGF_XCVR) >> EL3REG_ADDRCONFIGB_XCVR;
      autoselect = (config & EL3REG_ADDRCONFIGF_AUTOSELECT) != 0;
   }

   if(!autoselect)
   {
      /* Check if chosen medium is available */

      new_ports = ports & port_masks[transceiver];
      if(new_ports != 0)
         ports = new_ports;
      else
         autoselect = TRUE;
   }

   /* Auto-select media type */

   if(autoselect)
   {
      /* Get transceivers with an active link */

      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 4);

      if(unit->generation < CYCLONE_GEN)
      {
         tp_ports =
            ports &
            (EL3REG_MEDIAOPTIONSF_10BASET | EL3REG_MEDIAOPTIONSF_100BASETX);
         if(tp_ports != 0)
         {
            SelectMedium(unit, EL3XCVR_10BASET, base);
            media_status = unit->LEWordIn(unit->card, EL3REG_MEDIA);
            if((media_status & EL3REG_MEDIAF_BEAT) != 0)
               links |= tp_ports;
         }
      }

      if((ports & EL3REG_MEDIAOPTIONSF_MII) != 0
         || unit->generation >= CYCLONE_GEN)
      {
         for(i = 0; i < 32; i++)
         {
            if(ReadMII(unit, i, MII_STATUS, &status, base))
            {
               ReadMII(unit, i, MII_STATUS, &status, base);
                                    /* Yes, status reg must be read twice */
               if((status & MII_STATUSF_LINK) != 0)
               {
                  if(i == 24)   /* Built-in transceiver */
                  {
                     if(((status & MII_STATUSF_AUTONEGDONE) != 0)
                        && ((status & MII_STATUSF_EXTREGSET) != 0))
                     {
                        ReadMII(unit, i, MII_AUTONEGADVERT, &advert, base);
                        ReadMII(unit, i, MII_AUTONEGABILITY, &ability,
                           base);
                        modes = advert & ability;

                        if((modes & MII_AUTONEGF_100BASETX) != 0)
                           links |= EL3REG_MEDIAOPTIONSF_100BASETX;
#if 0
                        if((modes & MII_AUTONEGF_100BASET4) != 0)
                           links |= EL3REG_MEDIAOPTIONSF_100BASET4;
#endif
                        if((modes & MII_AUTONEGF_10BASET) != 0)
                           links |= EL3REG_MEDIAOPTIONSF_10BASET;
                     }
                     else
                     {
                        modes = MII_AUTONEGF_10BASET;
                        links |= EL3REG_MEDIAOPTIONSF_10BASET;
                     }
                     unit->autoneg_modes = modes;
                  }
                  else
                  {
                     links |= EL3REG_MEDIAOPTIONSF_MII;
                     unit->mii_phy_no = i;
                  }
               }
            }
         }
      }

#if 0
      if((ports & EL3REG_MEDIAOPTIONSF_10BASE2) != 0)
      {
         SelectMedium(unit, EL3XCVR_10BASE2, base);
         unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_STARTCOAX);

         if(LoopbackTest(base))
            links |= EL3REG_MEDIAOPTIONSF_10BASE2;

         unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_STOPCOAX);
      }
#endif

      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 0);

      new_ports = ports & links;
      if((new_ports) != 0)
         ports = new_ports;
   }

   /* Prioritised choice from remaining transceivers */

   for(i = 0; i < PORT_COUNT; i++)
   {
      transceiver = port_choices[i];
      if((ports & port_masks[transceiver]) != 0)
      {
         unit->transceiver = transceiver;
         i = PORT_COUNT;
      }
   }

   /* Find out whether to use full duplex */

   if((transceiver == EL3XCVR_10BASET || transceiver == EL3XCVR_100BASETX)
      && unit->generation >= CYCLONE_GEN)
   {
      modes = unit->autoneg_modes;
      if
      (
         (modes & MII_AUTONEGF_100BASETXFD) != 0
         ||
         (
            modes
            &
            (
               MII_AUTONEGF_100BASETX
               |
               MII_AUTONEGF_10BASETFD
            )
         )
         ==
         MII_AUTONEGF_10BASETFD
      )
         unit->flags |= UNITF_FULLDUPLEX;
   }

   /* Return */

   return TRUE;
}



/****i* etherlink3.device/ConfigureAdapter *********************************
*
*   NAME
*	ConfigureAdapter -- .
*
*   SYNOPSIS
*	ConfigureAdapter(unit)
*
*	VOID ConfigureAdapter(struct DevUnit *);
*
*   FUNCTION
*
*   INPUTS
*	unit
*
*   RESULT
*	None.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

VOID ConfigureAdapter(struct DevUnit *unit, struct DevBase *base)
{
   UBYTE i;

   /* Set MAC address */

   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 2);

   for(i = 0; i < ETH_ADDRESSSIZE; i++)
      unit->ByteOut(unit->card, EL3REG_ADDRESS0 + i, unit->address[i]);

   if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
   {
      for(i = 0; i < ETH_ADDRESSSIZE; i++)
         unit->ByteOut(unit->card, EL3REG_MASK0 + i, 0);
   }

   /* Enable wider statistics counters */

   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 4);
   if(unit->generation >= BOOMERANG_GEN)
      unit->LEWordOut(unit->card, EL3REG_NETDIAG,
         EL3REG_NETDIAGF_WIDESTATS);

   /* Decide on promiscuous mode */

   if((unit->flags & UNITF_PROM) != 0)
      unit->rx_filter_cmd |= EL3CMD_SETRXFILTERF_PROM;

   /* Select chosen transceiver */

   SelectMedium(unit, unit->transceiver, base);

   /* Go online */

   if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
      unit->LELongOut(unit->card, EL3REG_UPLIST, (ULONG)unit->upds);
   GoOnline(unit, base);

   /* Return */

   return;
}



/****i* etherlink3.device/SelectMedium *************************************
*
*   NAME
*	SelectMedium -- .
*
*   SYNOPSIS
*	SelectMedium(unit, transceiver)
*
*	VOID SelectMedium(struct DevUnit *, UWORD);
*
*   FUNCTION
*
*   INPUTS
*	unit
*	transceiver
*
*   RESULT
*	None.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

VOID SelectMedium(struct DevUnit *unit, UWORD transceiver,
   struct DevBase *base)
{
   ULONG config;
   UWORD old_window;

   if((transceiver == EL3XCVR_10BASET || transceiver == EL3XCVR_100BASETX)
      && unit->generation >= CYCLONE_GEN)
      transceiver = EL3XCVR_AUTONEG;

   /* Select transceiver */

   old_window =
      unit->LEWordIn(unit->card, EL3REG_STATUS) >> EL3REG_STATUSB_WINDOW;
   if(unit->bus == PCI_BUS)
   {
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 3);
      config = unit->LELongIn(unit->card, EL3REG_INTERNALCONFIG);
      config &= ~EL3REG_INTERNALCONFIGF_XCVR;
      config |= transceiver << EL3REG_INTERNALCONFIGB_XCVR;
      unit->LELongOut(unit->card, EL3REG_INTERNALCONFIG, config);
   }
   else
   {
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 0);
      config = unit->LEWordIn(unit->card, EL3REG_ADDRCONFIG);
      config &= ~EL3REG_ADDRCONFIGF_XCVR;
      config |= transceiver << EL3REG_ADDRCONFIGB_XCVR;
      unit->LEWordOut(unit->card, EL3REG_ADDRCONFIG, config);
   }
   unit->LEWordOut(unit->card, EL3REG_COMMAND,
      EL3CMD_SELECTWINDOW | old_window);

   return;
}



/****i* etherlink3.device/GoOnline *****************************************
*
*   NAME
*	GoOnline -- .
*
*   SYNOPSIS
*	GoOnline(unit)
*
*	VOID GoOnline(struct DevUnit *);
*
****************************************************************************
*
*/

VOID GoOnline(struct DevUnit *unit, struct DevBase *base)
{
   UWORD transceiver;

   /* Choose interrupts */

   unit->flags |= UNITF_ONLINE;
   unit->LEWordOut(unit->card, EL3REG_COMMAND,
      EL3CMD_SETINTMASK | unit->int_mask);
   unit->LEWordOut(unit->card, EL3REG_COMMAND,
      EL3CMD_SETZEROMASK | unit->int_mask);

   /* Enable the transceiver */

   if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_UPUNSTALL);
                                                               /* needed? */
   transceiver = unit->transceiver;

   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 4);
   if(transceiver == EL3XCVR_100BASETX)
   {
      unit->LEWordOut(unit->card, EL3REG_MEDIA, EL3REG_MEDIAF_BEATCHECK);
   }
   else if(transceiver == EL3XCVR_10BASET)
   {
      unit->LEWordOut(unit->card, EL3REG_MEDIA,
         EL3REG_MEDIAF_BEATCHECK | EL3REG_MEDIAF_JABBERCHECK);
   }
   else if(transceiver == EL3XCVR_10BASE2)
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_STARTCOAX | 0);

   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 3);
   if((unit->flags & UNITF_FULLDUPLEX) != 0)
      unit->LEWordOut(unit->card, EL3REG_MACCONTROL,
         EL3REG_MACCONTROLF_FULLDUPLEX);

   unit->LEWordOut(unit->card, EL3REG_COMMAND, unit->rx_filter_cmd);
   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_RXENABLE);
   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_TXENABLE);
   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 1);

   /* Enable statistics collection */

   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_STATSENABLE);

   /* Record start time and report Online event */

   GetSysTime(&unit->stats.LastStart);
   ReportEvents(unit, S2EVENT_ONLINE, base);

   return;
}



/****i* etherlink3.device/GoOffline ****************************************
*
*   NAME
*	GoOffline -- .
*
*   SYNOPSIS
*	GoOffline(unit)
*
*	VOID GoOffline(struct DevUnit *);
*
****************************************************************************
*
*/

VOID GoOffline(struct DevUnit *unit, struct DevBase *base)
{
   unit->flags &= ~UNITF_ONLINE;

   if((unit->flags & UNITF_HAVEADAPTER) != 0)
   {
      /* Stop interrupts */

      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SETINTMASK | 0);
      unit->LEWordOut(unit->card, EL3REG_COMMAND,
         EL3CMD_ACKINT | EL3INTF_ANY);

      /* Stop transmission and reception */

      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_RXDISABLE);
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_TXDISABLE);

      if((unit->capabilities & EL3ROM_CAPABILITIESF_FULLMASTER) != 0)
      {
         unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_UPSTALL);
         while((unit->LEWordIn(unit->card, EL3REG_STATUS) &
            EL3REG_STATUSF_CMDINPROGRESS) != 0);
         unit->LELongOut(unit->card, EL3REG_UPLIST, (ULONG)NULL);
         unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_DOWNSTALL);
         while((unit->LEWordIn(unit->card, EL3REG_STATUS)
            & EL3REG_STATUSF_CMDINPROGRESS) != 0);
         unit->LELongOut(unit->card, EL3REG_DOWNLIST, (ULONG)NULL);
      }

      /* Turn off media functions */

      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 4);
      unit->LEWordOut(unit->card, EL3REG_MEDIA, 0);
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 1);
#if 0
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_STOPCOAX | 0);
#endif

      /* Update then disable statistics */

      UpdateStats(unit, base);
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_STATSDISABLE);
   }

   /* Flush pending read and write requests */

   FlushUnit(unit, WRITE_QUEUE, S2ERR_OUTOFSERVICE, base);

   /* Report Offline event and return */

   ReportEvents(unit, S2EVENT_OFFLINE, base);
   return;
}



/****i* etherlink3.device/AddMulticastRange ********************************
*
*   NAME
*	AddMulticastRange -- .
*
*   SYNOPSIS
*	success = AddMulticastRange(unit, lower_bound, upper_bound)
*
*	BOOL AddMulticastRange(struct DevUnit *, UBYTE *, UBYTE *);
*
****************************************************************************
*
*/

BOOL AddMulticastRange(struct DevUnit *unit, const UBYTE *lower_bound,
   const UBYTE *upper_bound, struct DevBase *base)
{
   struct AddressRange *range;
   ULONG lower_bound_left, upper_bound_left;
   UWORD lower_bound_right, upper_bound_right;

   lower_bound_left = BELong(*((ULONG *)lower_bound));
   lower_bound_right = BEWord(*((UWORD *)(lower_bound + 4)));
   upper_bound_left = BELong(*((ULONG *)upper_bound));
   upper_bound_right = BEWord(*((UWORD *)(upper_bound + 4)));

   range = FindMulticastRange(unit, lower_bound_left, lower_bound_right,
      upper_bound_left, upper_bound_right, base);

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

         if(unit->range_count++ == 0)
         {
            unit->rx_filter_cmd |= EL3CMD_SETRXFILTERF_MCAST;
            unit->LEWordOut(unit->card, EL3REG_COMMAND,
               unit->rx_filter_cmd);
         }
      }
   }

   return range != NULL;
}



/****i* etherlink3.device/RemMulticastRange ********************************
*
*   NAME
*	RemMulticastRange -- .
*
*   SYNOPSIS
*	found = RemMulticastRange(unit, lower_bound, upper_bound)
*
*	BOOL RemMulticastRange(struct DevUnit *, UBYTE *, UBYTE *);
*
****************************************************************************
*
*/

BOOL RemMulticastRange(struct DevUnit *unit, const UBYTE *lower_bound,
   const UBYTE *upper_bound, struct DevBase *base)
{
   struct AddressRange *range;
   ULONG lower_bound_left, upper_bound_left;
   UWORD lower_bound_right, upper_bound_right;

   lower_bound_left = BELong(*((ULONG *)lower_bound));
   lower_bound_right = BEWord(*((UWORD *)(lower_bound + 4)));
   upper_bound_left = BELong(*((ULONG *)upper_bound));
   upper_bound_right = BEWord(*((UWORD *)(upper_bound + 4)));

   range = FindMulticastRange(unit, lower_bound_left, lower_bound_right,
      upper_bound_left, upper_bound_right, base);

   if(range != NULL)
   {
      if(--range->add_count == 0)
      {
         Disable();
         Remove((APTR)range);
         Enable();
         FreeMem(range, sizeof(struct AddressRange));

         if(--unit->range_count == 0)
         {
            unit->rx_filter_cmd &= ~EL3CMD_SETRXFILTERF_MCAST;
            unit->LEWordOut(unit->card, EL3REG_COMMAND,
               unit->rx_filter_cmd);
         }
      }
   }

   return range != NULL;
}



/****i* etherlink3.device/FindMulticastRange *******************************
*
*   NAME
*	FindMulticastRange -- .
*
*   SYNOPSIS
*	range = FindMulticastRange(unit, lower_bound_left,
*	    lower_bound_right, upper_bound_left, upper_bound_right)
*
*	struct AddressRange *FindMulticastRange(struct DevUnit *, ULONG,
*	    UWORD, ULONG, UWORD);
*
****************************************************************************
*
*/

static struct AddressRange *FindMulticastRange(struct DevUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
   UWORD upper_bound_right, struct DevBase *base)
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



/****i* etherlink3.device/FindTypeStats ************************************
*
*   NAME
*	FindTypeStats -- .
*
*   SYNOPSIS
*	stats = FindTypeStats(unit, list,
*	    packet_type)
*
*	struct TypeStats *FindTypeStats(struct DevUnit *, struct MinList *,
*	    ULONG);
*
****************************************************************************
*
*/

struct TypeStats *FindTypeStats(struct DevUnit *unit, struct MinList *list,
   ULONG packet_type, struct DevBase *base)
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



/****i* etherlink3.device/FlushUnit ****************************************
*
*   NAME
*	FlushUnit -- .
*
*   SYNOPSIS
*	FlushUnit(unit)
*
*	VOID FlushUnit(struct DevUnit *);
*
****************************************************************************
*
* Includes alternative implementations because of ambiguities in SANA-II
* documentation.
*
*/

VOID FlushUnit(struct DevUnit *unit, UBYTE last_queue, BYTE error,
   struct DevBase *base)
{
   struct IORequest *request;
   UBYTE i;
   struct Opener *opener, *tail;

   /* Abort queued requests */

   for(i = 0; i <= last_queue; i++)
   {
      while((request = (APTR)GetMsg(unit->request_ports[i])) != NULL)
      {
         request->io_Error = IOERR_ABORTED;
         ReplyMsg((APTR)request);
      }
   }

#if 1
   opener = (APTR)unit->openers.mlh_Head;
   tail = (APTR)&unit->openers.mlh_Tail;

   /* Flush every opener's read queue */

   while(opener != tail)
   {
      while((request = (APTR)GetMsg(&opener->read_port)) != NULL)
      {
         request->io_Error = error;
         ReplyMsg((APTR)request);
      }
      opener = (APTR)opener->node.mln_Succ;
   }

#else
   opener = request->ios2_BufferManagement;
   while((request = (APTR)GetMsg(&opener->read_port)) != NULL)
   {
      request->io_Error = IOERR_ABORTED;
      ReplyMsg((APTR)request);
   }
#endif

   /* Return */

   return;
}



/****i* etherlink3.device/StatusInt ****************************************
*
*   NAME
*	StatusInt
*
*   SYNOPSIS
*	finished = StatusInt(unit, int_code)
*
*	BOOL StatusInt(struct DevUnit *, APTR);
*
****************************************************************************
*
* int_code is really in A5, but GCC 2.95.3 doesn't seem able to handle that.
* Since we don't use this parameter, we can lie.
*
*/

BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code))
{
   struct DevBase *base;
   UWORD ints;

   base = unit->device;
   ints = unit->LEWordIn(unit->card, EL3REG_STATUS);

   if((ints & EL3INTF_ANY) != 0)
   {
      /* Handle interrupts */

      if((ints & EL3INTF_UPDONE) != 0)
      {
         unit->LEWordOut(unit->card, EL3REG_COMMAND,
            EL3CMD_ACKINT | EL3INTF_UPDONE);
         Cause(&unit->rx_int);
      }
      if((ints & EL3INTF_DOWNDONE) != 0)
      {
         unit->LEWordOut(unit->card, EL3REG_COMMAND,
            EL3CMD_ACKINT | EL3INTF_DOWNDONE);
         Cause(&unit->tx_end_int);
      }
      if((ints & EL3INTF_UPDATESTATS) != 0)
         UpdateStats(unit, base);
      if((ints & EL3INTF_RXCOMPLETE) != 0)
      {
#ifndef __MORPHOS__
         unit->LEWordOut(unit->card, EL3REG_COMMAND,
            EL3CMD_SETINTMASK | (unit->int_mask & ~EL3INTF_RXCOMPLETE));
#endif
         Cause(&unit->rx_int);
      }
      if((ints & EL3INTF_TXAVAIL) != 0)
      {
         unit->LEWordOut(unit->card, EL3REG_COMMAND,
            EL3CMD_ACKINT | EL3INTF_TXAVAIL);
         Cause(&unit->tx_int);
      }
      if((ints & EL3INTF_TXCOMPLETE) != 0)
         TxError(unit, base);

      /* Acknowledge interrupt request */

      unit->LEWordOut(unit->card, EL3REG_COMMAND,
         EL3CMD_ACKINT | EL3INTF_ANY);
   }

   return FALSE;
}



/****i* etherlink3.device/RXInt ********************************************
*
*   NAME
*	RXInt
*
*   SYNOPSIS
*	RXInt(unit, int_code)
*
*	VOID RXInt(struct DevUnit *, APTR);
*
****************************************************************************
*
*/

static VOID RXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   UWORD rx_status, packet_size;
   struct DevBase *base;
   BOOL is_orphan, accepted;
   ULONG packet_type;
   UBYTE *buffer;
   struct IOSana2Req *request, *request_tail;
   struct Opener *opener, *opener_tail;
   struct TypeStats *tracker;

   base = unit->device;
   buffer = unit->rx_buffer;

   while(((rx_status = unit->LEWordIn(unit->card, EL3REG_RXSTATUS))
      & EL3REG_RXSTATUSF_INCOMPLETE) == 0)
   {
      if((rx_status & EL3REG_RXSTATUSF_ERROR) == 0)
      {
         /* Read packet header */

         is_orphan = TRUE;
         packet_size = rx_status & EL3REG_RXSTATUS_SIZEMASK;
         unit->LongsIn(unit->card, EL3REG_DATA0, (ULONG *)buffer,
            ETH_HEADERSIZE + 3 >> 2);

         if(AddressFilter(unit, buffer + ETH_PACKET_DEST, base))
         {
            packet_type = BEWord(*((UWORD *)(buffer + ETH_PACKET_TYPE)));

            opener = (APTR)unit->openers.mlh_Head;
            opener_tail = (APTR)&unit->openers.mlh_Tail;

            /* Offer packet to every opener */

            while(opener != opener_tail)
            {
               request = (APTR)opener->read_port.mp_MsgList.lh_Head;
               request_tail = (APTR)&opener->read_port.mp_MsgList.lh_Tail;
               accepted = FALSE;

               /* Offer packet to each request until it's accepted */

               while((request != request_tail) && !accepted)
               {
                  if(request->ios2_PacketType == packet_type
                     || request->ios2_PacketType <= ETH_MTU
                     && packet_type <= ETH_MTU)
                  {
                     CopyPacket(unit, request, packet_size, packet_type,
                        buffer, !is_orphan, base);
                     accepted = TRUE;
                  }
                  request =
                     (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;
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
                  CopyPacket(unit,
                     (APTR)unit->request_ports[ADOPT_QUEUE]->
                     mp_MsgList.lh_Head, packet_size, packet_type, buffer,
                     FALSE, base);
               }
            }

            /* Update remaining statistics */

            tracker =
               FindTypeStats(unit, &unit->type_trackers, packet_type, base);
            if(tracker != NULL)
            {
               tracker->stats.PacketsReceived++;
               tracker->stats.BytesReceived += packet_size;
            }
         }
      }
      else
      {
         unit->stats.BadData++;
         ReportEvents(unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX,
            base);
      }

      /* Discard packet */

      Disable();   /* Needed? */
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_RXDISCARD);
      while((unit->LEWordIn(unit->card, EL3REG_STATUS) &
         EL3REG_STATUSF_CMDINPROGRESS) != 0);
      Enable();
   }

   /* Return */

   unit->LEWordOut(unit->card, EL3REG_COMMAND,
      EL3CMD_SETINTMASK | unit->int_mask);
   return;
}



/****i* etherlink3.device/CopyPacket ***************************************
*
*   NAME
*	CopyPacket
*
*   SYNOPSIS
*	CopyPacket(unit, request, packet_size, packet_type,
*	    buffer, all_read)
*
*	VOID CopyPacket(struct DevUnit *, struct IOSana2Req *, UWORD, UWORD,
*	    UBYTE *, BOOL);
*
****************************************************************************
*
*/

static VOID CopyPacket(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD packet_size, UWORD packet_type, UBYTE *buffer, BOOL all_read,
   struct DevBase *base)
{
   struct Opener *opener;
   BOOL filtered = FALSE;

   /* Set multicast and broadcast flags */

   request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
   if((*((ULONG *)(buffer + ETH_PACKET_DEST)) == 0xffffffff) &&
      (*((UWORD *)(buffer + ETH_PACKET_DEST + 4)) == 0xffff))
      request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
   else if((buffer[ETH_PACKET_DEST] & 0x1) != 0)
      request->ios2_Req.io_Flags |= SANA2IOF_MCAST;

   /* Set source and destination addresses and packet type */

   CopyMem(buffer + ETH_PACKET_SOURCE, request->ios2_SrcAddr,
      ETH_ADDRESSSIZE);
   CopyMem(buffer + ETH_PACKET_DEST, request->ios2_DstAddr,
      ETH_ADDRESSSIZE);
   request->ios2_PacketType = packet_type;

   /* Read rest of packet (PIO mode only) */

   if(!all_read)
   {
      unit->LongsIn(unit->card, EL3REG_DATA0,
         (ULONG *)(buffer + ((ETH_PACKET_DATA + 3) & ~0x3)),
         (packet_size - ETH_PACKET_DATA + 1) >> 2);
   }

   /* Adjust for cooked packet request */

   if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
   {
      packet_size -= ETH_PACKET_DATA;
      buffer += ETH_PACKET_DATA;
   }
#ifdef USE_HACKS
   else
      packet_size += 4;   /* Needed for Shapeshifter & Fusion */
#endif
   request->ios2_DataLength = packet_size;

   /* Filter packet */

   opener = request->ios2_BufferManagement;
   if((request->ios2_Req.io_Command == CMD_READ) &&
      (opener->filter_hook != NULL))
      if(!CallHookPkt(opener->filter_hook, request, buffer))
         filtered = TRUE;

   if(!filtered)
   {
      /* Copy packet into opener's buffer and reply packet */

      if(!opener->rx_function(request->ios2_Data, buffer, packet_size))
      {
         request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
         request->ios2_WireError = S2WERR_BUFF_ERROR;
         ReportEvents(unit,
            S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_RX,
            base);
      }
      Remove((APTR)request);
      ReplyMsg((APTR)request);
   }

   return;
}



/****i* etherlink3.device/AddressFilter ************************************
*
*   NAME
*	AddressFilter
*
*   SYNOPSIS
*	accept = AddressFilter(unit, address)
*
*	BOOL AddressFilter(struct DevUnit *, UBYTE *);
*
****************************************************************************
*
*/

static BOOL AddressFilter(struct DevUnit *unit, UBYTE *address,
   struct DevBase *base)
{
   struct AddressRange *range, *tail;
   BOOL accept = TRUE;
   ULONG address_left;
   UWORD address_right;

   /* Check whether address is unicast/broadcast or multicast */

   address_left = BELong(*((ULONG *)address));
   address_right = BEWord(*((UWORD *)(address + 4)));

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
            address_left == range->lower_bound_left &&
            address_right >= range->lower_bound_right) &&
            (address_left < range->upper_bound_left ||
            address_left == range->upper_bound_left &&
            address_right <= range->upper_bound_right))
            accept = TRUE;
         range = (APTR)range->node.mln_Succ;
      }

      if(!accept)
         unit->special_stats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
   }

   return accept;
}



/****i* etherlink3.device/TXInt ********************************************
*
*   NAME
*	TXInt
*
*   SYNOPSIS
*	TXInt(unit)
*
*	VOID TXInt(struct DevUnit *);
*
****************************************************************************
*
*/

static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   UWORD packet_size, data_size, send_size;
   struct DevBase *base;
   struct IOSana2Req *request;
   BOOL proceed = TRUE;
   struct Opener *opener;
   ULONG *buffer, wire_error;
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   BYTE error;
   struct MsgPort *port;
   struct TypeStats *tracker;

   base = unit->device;
   port = unit->request_ports[WRITE_QUEUE];

   while(proceed && (!IsMsgPortEmpty(port)))
   {
      error = 0;

      request = (APTR)port->mp_MsgList.lh_Head;
      data_size = packet_size = request->ios2_DataLength;

      if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
         packet_size += ETH_PACKET_DATA;

      if(unit->LEWordIn(unit->card, EL3REG_TXSPACE)
         > EL3_PREAMBLESIZE + packet_size)
      {
         /* Write packet preamble */

         unit->LELongOut(unit->card, EL3REG_DATA0, packet_size);

         /* Write packet header */

         send_size = (packet_size + 3) & ~0x3;
         if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
         {
            unit->LongOut(unit->card, EL3REG_DATA0,
               *((ULONG *)request->ios2_DstAddr));
            unit->WordOut(unit->card, EL3REG_DATA0,
               *((UWORD *)(request->ios2_DstAddr + 4)));
            unit->WordOut(unit->card, EL3REG_DATA0,
               *((UWORD *)unit->address));
            unit->LongOut(unit->card, EL3REG_DATA0,
               *((ULONG *)(unit->address + 2)));
            unit->BEWordOut(unit->card, EL3REG_DATA0,
               request->ios2_PacketType);
            send_size -= ETH_HEADERSIZE;
         }

         /* Get packet data */

         opener = (APTR)request->ios2_BufferManagement;
         dma_tx_function = opener->dma_tx_function;
         if(dma_tx_function != NULL)
            buffer = (ULONG *)dma_tx_function(request->ios2_Data);
         else
            buffer = NULL;

         if(buffer == NULL)
         {
            buffer = (ULONG *)unit->tx_buffer;
            if(!opener->tx_function(buffer, request->ios2_Data, data_size))
            {
               error = S2ERR_NO_RESOURCES;
               wire_error = S2WERR_BUFF_ERROR;
               ReportEvents(unit,
                  S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF
                  | S2EVENT_TX, base);
            }
         }

         /* Write packet data */

         if(error == 0)
         {
            unit->LongsOut(unit->card, EL3REG_DATA0, buffer,
               send_size >> 2);
            buffer += (send_size >> 2);
            if((send_size & 0x3) != 0)
               unit->WordOut(unit->card, EL3REG_DATA0, *((UWORD *)buffer));
         }

         /* Reply packet */

         request->ios2_Req.io_Error = error;
         request->ios2_WireError = wire_error;
         Remove((APTR)request);
         ReplyMsg((APTR)request);

         /* Update statistics */

         if(error == 0)
         {
            tracker = FindTypeStats(unit, &unit->type_trackers,
               request->ios2_PacketType, base);
            if(tracker != NULL)
            {
               tracker->stats.PacketsSent++;
               tracker->stats.BytesSent += packet_size;
            }
         }
      }
      else
         proceed = FALSE;
   }

   if(proceed)
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
   else
   {
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SETTXTHRESH
         | ((EL3_PREAMBLESIZE + packet_size) >> unit->size_shift));
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;
   }

   return;
}



/****i* etherlink3.device/TxError ******************************************
*
*   NAME
*	TxError
*
*   SYNOPSIS
*	TxError(unit)
*
*	VOID TxError(struct DevUnit *);
*
****************************************************************************
*
*/

static VOID TxError(struct DevUnit *unit, struct DevBase *base)
{
   UPINT window1_offset;
   UBYTE tx_status, flags = 0;

   window1_offset = unit->window1_offset;

   /* Gather all errors */

   while(((tx_status =
      unit->ByteIn(unit->card, window1_offset + EL3REG_TXSTATUS))
      & EL3REG_TXSTATUSF_COMPLETE) != 0)
   {
      flags |= tx_status;
      unit->ByteOut(unit->card, window1_offset + EL3REG_TXSTATUS, 0);
    }

   /* Restart transmitter if necessary */

   if((flags & EL3REG_TXSTATUSF_JABBER) != 0)
   {
      Disable();
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_TXRESET);
      while((unit->LEWordIn(unit->card, EL3REG_STATUS) &
         EL3REG_STATUSF_CMDINPROGRESS) != 0);
      Enable();
   }

   if((flags & (EL3REG_TXSTATUSF_JABBER | EL3REG_TXSTATUSF_OVERFLOW
      | EL3REG_TXSTATUSF_RECLAIMERROR)) != 0)
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_TXENABLE);

   /* Report the error(s) */

   ReportEvents(unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_TX, base);

   return;
}



/****i* etherlink3.device/UpdateStats **************************************
*
*   NAME
*	UpdateStats
*
*   SYNOPSIS
*	UpdateStats(unit)
*
*	VOID UpdateStats(struct DevUnit *);
*
****************************************************************************
*
*/

VOID UpdateStats(struct DevUnit *unit, struct DevBase *base)
{
   UBYTE frame_counts_upper;
   UWORD generation, old_window;

   generation = unit->generation;

   old_window =
      unit->LEWordIn(unit->card, EL3REG_STATUS) >> EL3REG_STATUSB_WINDOW;
   if(generation < VORTEX_GEN)
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_STATSDISABLE);

   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 6);
   unit->ByteIn(unit->card, EL3REG_CARRIERLOST);
   unit->ByteIn(unit->card, EL3REG_SQEERRORS);
   unit->ByteIn(unit->card, EL3REG_MULTIPLECOLLISIONS);
   unit->ByteIn(unit->card, EL3REG_SINGLECOLLISIONS);
   unit->ByteIn(unit->card, EL3REG_LATECOLLISIONS);
   unit->stats.Overruns += unit->ByteIn(unit->card, EL3REG_RXOVERRUNS);
   unit->stats.PacketsSent += unit->ByteIn(unit->card, EL3REG_TXFRAMESOK);
   unit->stats.PacketsReceived +=
      unit->ByteIn(unit->card, EL3REG_RXFRAMESOK);
   unit->special_stats[S2SS_ETHERNET_RETRIES & 0xffff] +=
      unit->ByteIn(unit->card, EL3REG_FRAMESDEFERRED);
   unit->LEWordIn(unit->card, EL3REG_RXBYTESOK);
   unit->LEWordIn(unit->card, EL3REG_TXBYTESOK);
   if(generation >= VORTEX_GEN)
   {
      frame_counts_upper = unit->ByteIn(unit->card, EL3REG_FRAMESOKUPPER);
      unit->stats.PacketsReceived += (frame_counts_upper & 0x3) << 8;
      unit->stats.PacketsSent += (frame_counts_upper & 0x30) << 4;
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_SELECTWINDOW | 4);
      unit->ByteIn(unit->card, EL3REG_BADSSD);
      if(generation >= BOOMERANG_GEN)
         unit->ByteIn(unit->card, EL3REG_BYTESOKUPPER);
   }

   if(generation < VORTEX_GEN)
      unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_STATSENABLE);
   unit->LEWordOut(unit->card, EL3REG_COMMAND,
      EL3CMD_SELECTWINDOW | old_window);
   return;
}



/****i* etherlink3.device/DMARXInt *****************************************
*
*   NAME
*	DMARXInt
*
*   SYNOPSIS
*	DMARXInt(unit, int_code)
*
*	VOID DMARXInt(struct DevUnit *, APTR);
*
****************************************************************************
*
*/

static VOID DMARXInt(REG(a1, struct DevUnit *unit),
   REG(a5, APTR int_code))
{
   UWORD packet_size;
   struct DevBase *base;
   BOOL is_orphan, accepted;
   ULONG rx_status, packet_type, *upd, *fragment, dma_size;
   UBYTE *buffer;
   struct IOSana2Req *request, *request_tail;
   struct Opener *opener, *opener_tail;
   struct TypeStats *tracker;

   base = unit->device;
   upd = unit->next_upd;

   dma_size = UPD_SIZE * RX_SLOT_COUNT;
   CachePostDMA(unit->upds, &dma_size, 0);

   while(((rx_status = LELong(upd[EL3UPD_STATUS]))
      & EL3UPD_STATUSF_COMPLETE) != 0)
   {
      fragment = upd + EL3UPD_FIRSTFRAG;
      buffer = (UBYTE *)LELong(fragment[EL3FRAG_ADDR]);

      dma_size = ETH_MAXPACKETSIZE;
      CachePostDMA(buffer, &dma_size, 0);

      if((rx_status & EL3UPD_STATUSF_ERROR) == 0)
      {
         is_orphan = TRUE;
         packet_size = rx_status & EL3UPD_STATUSF_SIZE;

         if(AddressFilter(unit, buffer + ETH_PACKET_DEST, base))
         {
            packet_type = BEWord(*((UWORD *)(buffer + ETH_PACKET_TYPE)));

            opener = (APTR)unit->openers.mlh_Head;
            opener_tail = (APTR)&unit->openers.mlh_Tail;

            /* Offer packet to every opener */

            while(opener != opener_tail)
            {
               request = (APTR)opener->read_port.mp_MsgList.lh_Head;
               request_tail = (APTR)&opener->read_port.mp_MsgList.lh_Tail;
               accepted = FALSE;

               /* Offer packet to each request until it's accepted */

               while((request != request_tail) && !accepted)
               {
                  if(request->ios2_PacketType == packet_type
                     || request->ios2_PacketType <= ETH_MTU
                     && packet_type <= ETH_MTU)
                  {
                     CopyPacket(unit, request, packet_size, packet_type,
                        buffer, TRUE, base);
                     accepted = TRUE;
                  }
                  request =
                     (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;
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
                  CopyPacket(unit,
                     (APTR)unit->request_ports[ADOPT_QUEUE]->
                     mp_MsgList.lh_Head, packet_size, packet_type, buffer,
                     TRUE, base);
               }
            }

            /* Update remaining statistics */

            tracker =
               FindTypeStats(unit, &unit->type_trackers, packet_type, base);
            if(tracker != NULL)
            {
               tracker->stats.PacketsReceived++;
               tracker->stats.BytesReceived += packet_size;
            }
         }
      }
      else
      {
         unit->stats.BadData++;
         ReportEvents(unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX,
            base);
      }

      upd[EL3UPD_STATUS] = 0;

      dma_size = ETH_MAXPACKETSIZE;
      CachePreDMA(buffer, &dma_size, 0);

      upd = (ULONG *)LELong(upd[EL3UPD_NEXT]);
   }

   dma_size = UPD_SIZE * RX_SLOT_COUNT;
   CachePreDMA(unit->upds, &dma_size, 0);

   /* Return */

   unit->next_upd = upd;
   unit->LEWordOut(unit->card, EL3REG_COMMAND, EL3CMD_UPUNSTALL);
#if 1 /* ??? */
   unit->LEWordOut(unit->card, EL3REG_COMMAND,
      EL3CMD_SETINTMASK | unit->int_mask);
#endif
   return;
}



/****i* etherlink3.device/DMATXInt *****************************************
*
*   NAME
*	DMATXInt
*
*   SYNOPSIS
*	DMATXInt(unit, int_code)
*
*	VOID DMATXInt(struct DevUnit *, APTR);
*
****************************************************************************
*
*/

static VOID DMATXInt(REG(a1, struct DevUnit *unit),
   REG(a5, APTR int_code))
{
   UWORD packet_size, data_size, slot, new_slot, *p, *q, i;
   struct DevBase *base;
   struct IOSana2Req *request;
   BOOL proceed = TRUE;
   struct Opener *opener;
   ULONG wire_error, *dpd, *last_dpd, *next_dpd, *fragment, dma_size;
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   BYTE error;
   UBYTE *buffer;
   struct MsgPort *port;

   base = unit->device;
   port = unit->request_ports[WRITE_QUEUE];

   while(proceed && (!IsMsgPortEmpty(port)))
   {
      slot = unit->tx_in_slot;
      new_slot = (slot + 1) % TX_SLOT_COUNT;

      if(new_slot != unit->tx_out_slot
         && (unit->flags & UNITF_TXBUFFERINUSE) == 0)
      {
         error = 0;

         request = (APTR)port->mp_MsgList.lh_Head;
         data_size = packet_size = request->ios2_DataLength;

         if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
            packet_size += ETH_HEADERSIZE;
         dpd = unit->dpds + ((DPD_SIZE / sizeof(ULONG)) * slot);

         /* Write packet preamble */

         Remove((APTR)request);
         unit->tx_requests[slot] = request;
         dpd[EL3DPD_NEXT] = (ULONG)NULL;
         dpd[EL3DPD_HEADER] =
            MakeLELong(EL3DPD_HEADERF_DLINT | packet_size);
         fragment = dpd + EL3DPD_FIRSTFRAG;

         /* Write packet header */

         if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
         {
            buffer = unit->headers + ETH_HEADERSIZE * slot;
            fragment[EL3FRAG_ADDR] = MakeLELong((ULONG)buffer);
            fragment[EL3FRAG_LEN] = MakeLELong(ETH_HEADERSIZE);

            p = (UWORD *)buffer;
            for(i = 0, q = (UWORD *)request->ios2_DstAddr;
               i < ETH_ADDRESSSIZE / 2; i++)
               *p++ = *q++;
            for(i = 0, q = (UWORD *)unit->address;
               i < ETH_ADDRESSSIZE / 2; i++)
               *p++ = *q++;
            *p++ = MakeBEWord(request->ios2_PacketType);
            buffer = (UBYTE *)p;

            dma_size = ETH_HEADERSIZE;
            CachePreDMA(buffer, &dma_size, DMA_ReadFromRAM);
            fragment += EL3_FRAGLEN;
         }

         /* Get packet data */

         opener = (APTR)request->ios2_BufferManagement;
         dma_tx_function = opener->dma_tx_function;
         if(dma_tx_function != NULL)
            buffer = dma_tx_function(request->ios2_Data);
         else
            buffer = NULL;

         if(buffer == NULL)
         {
            buffer = unit->tx_buffer;
            if(opener->tx_function(buffer, request->ios2_Data, data_size))
               unit->flags |= UNITF_TXBUFFERINUSE;
            else
            {
               error = S2ERR_NO_RESOURCES;
               wire_error = S2WERR_BUFF_ERROR;
               ReportEvents(unit,
                  S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF
                  | S2EVENT_TX, base);
            }
         }

         /* Write packet data */

         if(error == 0)
         {
            fragment[EL3FRAG_ADDR] = MakeLELong((ULONG)buffer);
            fragment[EL3FRAG_LEN] =
               MakeLELong(EL3FRAG_LENF_LAST | data_size);
            dma_size = data_size;
            CachePreDMA(buffer, &dma_size, DMA_ReadFromRAM);

            /* Pass packet to adapter */

            last_dpd = (ULONG *)unit->LELongIn(unit->card, EL3REG_DOWNLIST);
            if(last_dpd != NULL)
            {
               unit->LEWordOut(unit->card, EL3REG_COMMAND,
                  EL3CMD_DOWNSTALL);
               while((unit->LEWordIn(unit->card, EL3REG_STATUS)
                  & EL3REG_STATUSF_CMDINPROGRESS) != 0);
               while((next_dpd = (ULONG *)LELong(last_dpd[EL3DPD_NEXT]))
                  != NULL)
                  last_dpd = next_dpd;
               last_dpd[EL3DPD_NEXT] = MakeLELong((ULONG)dpd);
               dma_size = DPD_SIZE * TX_SLOT_COUNT;
               CachePreDMA(unit->dpds, &dma_size, 0);
               unit->LEWordOut(unit->card, EL3REG_COMMAND,
                  EL3CMD_DOWNUNSTALL);
            }
            else
            {
               dma_size = DPD_SIZE * TX_SLOT_COUNT;
               CachePreDMA(unit->dpds, &dma_size, 0);
               unit->LELongOut(unit->card, EL3REG_DOWNLIST, (ULONG)dpd);
            }

            unit->tx_in_slot = new_slot;
         }
         else
         {
            /* Reply packet */

            request->ios2_Req.io_Error = error;
            request->ios2_WireError = wire_error;
            ReplyMsg((APTR)request);
         }
      }
      else
         proceed = FALSE;
   }

   if(proceed)
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
   else
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;

   return;
}



/****i* etherlink3.device/DMATXEndInt **************************************
*
*   NAME
*	TXInt
*
*   SYNOPSIS
*	TXInt(unit, int_code)
*
*	VOID TXInt(struct DevUnit *, APTR);
*
*   NOTES
*	I think it's safe to assume that there will always be at least one
*	completed packet whenever this interrupt is called.
*
****************************************************************************
*
*/

static VOID DMATXEndInt(REG(a1, struct DevUnit *unit),
   REG(a5, APTR int_code))
{
   UWORD data_size, packet_size, new_out_slot, i;
   UBYTE *buffer;
   struct DevBase *base;
   struct IOSana2Req *request;
   ULONG *dpd, *fragment, dma_size;
   struct TypeStats *tracker;

   /* Find out which packets have completed */

   base = unit->device;
   dpd = (UPINT *)unit->LELongIn(unit->card, EL3REG_DOWNLIST);
   if(dpd != NULL)
      new_out_slot = (dpd - unit->dpds) / (sizeof(ULONG) * DPD_SIZE);
   else
      new_out_slot = unit->tx_in_slot;

   /* Retire sent packets */

   for(i = unit->tx_out_slot; i != new_out_slot;
      i = (i + 1) % TX_SLOT_COUNT)
   {
      /* Mark end of DMA */

      request = unit->tx_requests[i];
      data_size = packet_size = request->ios2_DataLength;
      if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
         packet_size += ETH_HEADERSIZE;

      dpd = unit->dpds + ((DPD_SIZE / sizeof(ULONG)) * i);
      fragment = dpd + EL3DPD_FIRSTFRAG;
      dma_size = DPD_SIZE * TX_SLOT_COUNT;
      CachePostDMA(unit->dpds, &dma_size, 0);

      if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
      {
         buffer = (APTR)LELong(fragment[EL3FRAG_ADDR]);
         dma_size = ETH_HEADERSIZE;
         CachePostDMA(buffer, &dma_size, DMA_ReadFromRAM);
         fragment += EL3_FRAGLEN;
      }

      buffer = (APTR)LELong(fragment[EL3FRAG_ADDR]);
      dma_size = data_size;
      CachePostDMA(buffer, &dma_size, DMA_ReadFromRAM);

      /* Check if unit's TX buffer is now free */

      if(buffer == unit->tx_buffer)
         unit->flags &= ~UNITF_TXBUFFERINUSE;

      /* Update statistics */

      tracker = FindTypeStats(unit, &unit->type_trackers,
         request->ios2_PacketType, base);
      if(tracker != NULL)
      {
         tracker->stats.PacketsSent++;
         tracker->stats.BytesSent += packet_size;
      }

      /* Reply request */

      request->ios2_Req.io_Error = 0;
      ReplyMsg((APTR)request);
   }

   unit->tx_out_slot = new_out_slot;

   /* Restart downloads if they had stopped */

   if(unit->request_ports[WRITE_QUEUE]->mp_Flags == PA_IGNORE)
      Cause(&unit->tx_int);

   return;
}



/****i* etherlink3.device/ReportEvents *************************************
*
*   NAME
*	ReportEvents -- .
*
*   SYNOPSIS
*	ReportEvents(unit, events)
*
*	VOID ReportEvents(struct DevUnit *, ULONG);
*
*   INPUTS
*	unit - A unit of this device.
*	events - A mask of events to report.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

static VOID ReportEvents(struct DevUnit *unit, ULONG events,
   struct DevBase *base)
{
   struct IOSana2Req *request, *tail, *next_request;
   struct List *list;

   list = &unit->request_ports[EVENT_QUEUE]->mp_MsgList;
   next_request = (APTR)list->lh_Head;
   tail = (APTR)&list->lh_Tail;

   Disable();
   while(next_request != tail)
   {
      request = next_request;
      next_request = (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

      if((request->ios2_WireError & events) != 0)
      {
         request->ios2_WireError = events;
         Remove((APTR)request);
         ReplyMsg((APTR)request);
      }
   }
   Enable();

   return;
}



/****i* etherlink3.device/UnitTask *****************************************
*
*   NAME
*	UnitTask
*
*   SYNOPSIS
*	UnitTask()
*
*	VOID UnitTask();
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

static VOID UnitTask()
{
   struct Task *task;
   struct IORequest *request;
   struct DevUnit *unit;
   struct DevBase *base;
   struct MsgPort *general_port;
   ULONG signals, wait_signals, card_removed_signal, card_inserted_signal,
      general_port_signal;

   /* Get parameters */

   task = AbsExecBase->ThisTask;
   unit = task->tc_UserData;
   base = unit->device;

   /* Activate general request port */

   general_port = unit->request_ports[GENERAL_QUEUE];
   general_port->mp_SigTask = task;
   general_port->mp_SigBit = AllocSignal(-1);
   general_port_signal = 1 << general_port->mp_SigBit;
   general_port->mp_Flags = PA_SIGNAL;

   /* Allocate a signal for notification of card removal */

   card_removed_signal = unit->card_removed_signal = 1 << AllocSignal(-1);
   card_inserted_signal = unit->card_inserted_signal = 1 << AllocSignal(-1);
   wait_signals = (1 << general_port->mp_SigBit) | card_removed_signal
      | card_inserted_signal;

   /* Tell ourselves to check port for old messages */

   Signal(task, general_port_signal);

   /* Infinite loop to service requests and signals */

   while(TRUE)
   {
      signals = Wait(wait_signals);

      if((signals & card_inserted_signal) != 0)
      {
         if(unit->insertion_function(unit->card, base))
         {
            unit->flags |= UNITF_HAVEADAPTER;
            if((unit->flags & UNITF_CONFIGURED) != 0)
               ConfigureAdapter(unit, base);
         }
      }

      if((signals & card_removed_signal) != 0)
      {
         unit->removal_function(unit->card, base);
         GoOffline(unit, base);
      }

      if((signals & general_port_signal) != 0)
      {
         while((request = (APTR)GetMsg(general_port)) != NULL)
         {
            /* Service the request as soon as the unit is free */

            ObtainSemaphore(&unit->access_lock);
            ServiceRequest((APTR)request, base);
         }
      }
   }
}



/****i* etherlink3.device/ReadEEPROM ***************************************
*
*   NAME
*	ReadEEPROM -- Read a location on card's EEPROM.
*
*   SYNOPSIS
*	value = ReadEEPROM(unit, index)
*
*	UWORD ReadEEPROM(struct DevUnit *, UWORD);
*
*   INPUTS
*	unit - Device unit.
*	index - Offset within EEPROM.
*
*   RESULT
*	value - Contents of specified EEPROM location.
*
****************************************************************************
*
*/

static UWORD ReadEEPROM(struct DevUnit *unit, UWORD index,
   struct DevBase *base)
{
   unit->LEWordOut(unit->card, EL3REG_EEPROMCMD, EL3ECMD_READ | index);
   while((unit->LEWordIn(unit->card, EL3REG_EEPROMCMD) &
      EL3REG_EEPROMCMDF_BUSY) != 0);

   return unit->LEWordIn(unit->card, EL3REG_EEPROMDATA);
}



/****i* etherlink3.device/ReadMII ******************************************
*
*   NAME
*	ReadMII -- Read a register on an MII PHY.
*
*   SYNOPSIS
*	success = ReadMII(unit, phy_no, reg_no, value)
*
*	BOOL ReadMII(struct DevUnit *, UWORD, UWORD, UWORD *);
*
*   FUNCTION
*	Reads a register on an MII PHY. Window 4 must be selected before
*	calling this function.
*
*   INPUTS
*	unit - Device unit.
*	phy_no - .
*	reg_no - .
*	value - Pointer to location to store value read from MII register.
*
*   RESULT
*	success - Success indicator.
*
****************************************************************************
*
*/

static BOOL ReadMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   UWORD *value, struct DevBase *base)
{
   BOOL success = TRUE;

   WriteMIIBits(unit, 0xffffffff, 32, base);
   WriteMIIBits(unit, 0x6, 4, base);
   WriteMIIBits(unit, phy_no, 5, base);
   WriteMIIBits(unit, reg_no, 5, base);
   ReadMIIBit(unit, base);
   if(ReadMIIBit(unit, base))
      success = FALSE;
   *value = ReadMIIBits(unit, 16, base);
   ReadMIIBit(unit, base);

   return success;
}



/****i* etherlink3.device/WriteMII *****************************************
*
*   NAME
*	WriteMII -- Write to a register on an MII PHY.
*
*   SYNOPSIS
*	WriteMII(unit, phy_no, reg_no, value)
*
*	VOID WriteMII(struct DevUnit *, UWORD, UWORD, UWORD);
*
*   INPUTS
*	unit - Device unit.
*	phy_no - .
*	reg_no - .
*	value - value to write to MII register.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

#if 0
static VOID WriteMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   UWORD value, struct DevBase *base)
{
   WriteMIIBits(unit, 0xffffffff, 32, base);
   WriteMIIBits(unit, 0x5, 4, base);
   WriteMIIBits(unit, phy_no, 5, base);
   WriteMIIBits(unit, reg_no, 5, base);
   WriteMIIBit(unit, TRUE, base);
   WriteMIIBits(unit, value, 16, base);
   DoMIIZCycle(unit, base);

   return;
}
#endif



/****i* etherlink3.device/ReadMIIBits **************************************
*
*   NAME
*	ReadMIIBits
*
*   SYNOPSIS
*	value = ReadMIIBits(unit, count)
*
*	ULONG ReadMIIBits(struct DevUnit *, UBYTE);
*
****************************************************************************
*
*/

static ULONG ReadMIIBits(struct DevUnit *unit, UBYTE count,
   struct DevBase *base)
{
   UBYTE i;
   ULONG value = 0;

/*   LEWordOut(io_addr, LEWordIn(reg) & ~EL3REG_PHYMGMTF_WRITE);*/
   for(i = 0; i < count; i++)
   {
      value <<= 1;
      if(ReadMIIBit(unit, base))
         value |= 1;
unit->LEWordIn(unit->card, EL3REG_PHYMGMT);
   }
/*      ReadMIIBit(unit, base)? value |= 1;*/

   return value;
}


/****i* etherlink3.device/WriteMIIBits *************************************
*
*   NAME
*	WriteMIIBits
*
*   SYNOPSIS
*	WriteMIIBits(unit, value, count)
*
*	VOID WriteMIIBits(struct DevUnit *, ULONG, UBYTE);
*
****************************************************************************
*
*/

static VOID WriteMIIBits(struct DevUnit *unit, ULONG value, UBYTE count,
   struct DevBase *base)
{
   ULONG mask;

   for(mask = 1 << (count - 1); mask != 0; mask >>= 1)
      WriteMIIBit(unit, (value & mask) != 0, base);

   return;
}



/****i* etherlink3.device/ReadMIIBit ***************************************
*
*   NAME
*	ReadMIIBit
*
*   SYNOPSIS
*	is_one = ReadMIIBit(unit)
*
*	BOOL ReadMIIBit(struct DevUnit *);
*
****************************************************************************
*
*/

static BOOL ReadMIIBit(struct DevUnit *unit, struct DevBase *base)
{
   BOOL is_one;

   unit->LEWordOut(unit->card, EL3REG_PHYMGMT, 0);
   BusyMicroDelay(1, base);
   unit->LEWordOut(unit->card, EL3REG_PHYMGMT, EL3REG_PHYMGMTF_CLK);
   BusyMicroDelay(1, base);
   is_one =
      (unit->LEWordIn(unit->card, EL3REG_PHYMGMT) & EL3REG_PHYMGMTF_DATA)
      != 0;
   BusyMicroDelay(1, base);

   return is_one;
}



/****i* etherlink3.device/WriteMIIBit **************************************
*
*   NAME
*	WriteMIIBit
*
*   SYNOPSIS
*	WriteMIIBit(unit, is_one)
*
*	VOID WriteMIIBit(struct DevUnit *, BOOL);
*
****************************************************************************
*
*/

static VOID WriteMIIBit(struct DevUnit *unit, BOOL is_one,
   struct DevBase *base)
{
   unit->LEWordOut(unit->card, EL3REG_PHYMGMT, EL3REG_PHYMGMTF_WRITE);
   BusyMicroDelay(1, base);
   if(is_one)
      unit->LEWordOut(unit->card, EL3REG_PHYMGMT,
         EL3REG_PHYMGMTF_WRITE | EL3REG_PHYMGMTF_CLK |
         EL3REG_PHYMGMTF_DATA);
   else
      unit->LEWordOut(unit->card, EL3REG_PHYMGMT,
         EL3REG_PHYMGMTF_WRITE | EL3REG_PHYMGMTF_CLK);
   BusyMicroDelay(1, base);

   return;
}



/****i* etherlink3.device/DoMIIZCycle **************************************
*
*   NAME
*	DoMIIZCycle
*
*   SYNOPSIS
*	DoMIIZCycle(unit)
*
*	VOID DoMIIZCycle(struct DevUnit *);
*
****************************************************************************
*
*/

#if 0
static VOID DoMIIZCycle(struct DevUnit *unit, struct DevBase *base)
{
   unit->LEWordOut(unit->card, EL3REG_PHYMGMT,
      unit->LEWordIn(unit->card, EL3REG_PHYMGMT) & ~EL3REG_PHYMGMTF_CLK);
   BusyMicroDelay(1, base);
   unit->LEWordOut(unit->card, EL3REG_PHYMGMT, EL3REG_PHYMGMTF_CLK);
   BusyMicroDelay(1, base);

   return;
}
#endif



#if 0
static VOID BusyMicroDelay(ULONG micros, struct DevBase *base)
{
   struct timeval time, end_time;

   GetSysTime(&end_time);
   time.tv_secs = 0;
   time.tv_micro = micros;
   AddTime(&end_time, &time);

   while(CmpTime(&end_time, &time) < 0)
      GetSysTime(&time);

   return;
}


#else
static VOID BusyMicroDelay(ULONG micros, struct DevBase *base)
{
   struct EClockVal time, end_time;
   ULONG rate;

   rate = ReadEClock(&time);
   end_time.ev_hi = time.ev_hi;
   end_time.ev_lo = time.ev_lo + (micros * rate + 1) / 1000000;
   if(end_time.ev_lo < time.ev_lo)
      end_time.ev_hi++;

   while(time.ev_lo < end_time.ev_lo || time.ev_hi < end_time.ev_hi)
      ReadEClock(&time);

   return;
}
#endif



