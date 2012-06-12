/*

Copyright (C) 2001-2012 Neil Cafferkey

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
#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include "device.h"
#include "rhine.h"

#include "unit_protos.h"
#include "request_protos.h"
#include "timer_protos.h"


#define TASK_PRIORITY 0
#define STACK_SIZE 4096
#define INT_MASK 0xffff
#define TX_DESC_SIZE (RH_DESCSIZE * 2 + ETH_HEADERSIZE + 2)

#ifndef AbsExecBase
#define AbsExecBase sys_base
#endif

VOID DeinitialiseAdapter(struct DevUnit *unit, struct DevBase *base);
static struct AddressRange *FindMulticastRange(struct DevUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
   UWORD upper_bound_right, struct DevBase *base);
static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base);
static BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID RXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
static VOID DistributeRXPacket(struct DevUnit *unit, const UBYTE *packet,
   UWORD packet_size, struct DevBase *base);
static VOID CopyPacket(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD packet_size, UWORD packet_type, const UBYTE *buffer,
   struct DevBase *base);
static BOOL AddressFilter(struct DevUnit *unit, UBYTE *address,
   struct DevBase *base);
static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
static VOID TXEndInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID RetireTXSlot(struct DevUnit *unit, UWORD slot,
   struct DevBase *base);
static VOID ResetHandler(REG(a1, struct DevUnit *unit),
   REG(a6, APTR int_code));
static VOID ReportEvents(struct DevUnit *unit, ULONG events,
   struct DevBase *base);
static VOID UnitTask(struct ExecBase *sys_base);
UWORD ReadMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   struct DevBase *base);


static const UBYTE broadcast_address[] =
   {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


#ifdef __AROS__
#undef AddTask
#define AddTask(task, initial_pc, final_pc) \
   ({ \
      struct TagItem _task_tags[] = \
         {{TASKTAG_ARG1, (IPTR)SysBase}, {TAG_END, 0}}; \
      NewAddTask(task, initial_pc, final_pc, _task_tags); \
   })
#endif



/****i* rhine.device/CreateUnit ********************************************
*
*   NAME
*	CreateUnit -- Create a unit.
*
*   SYNOPSIS
*	unit = CreateUnit(index, io_base, id, card,
*	    io_tags, bus)
*
*	struct DevUnit *CreateUnit(ULONG, APTR, UWORD, APTR,
*	    struct TagItem *, UWORD);
*
*   FUNCTION
*	Creates a new unit.
*
****************************************************************************
*
*/

struct DevUnit *CreateUnit(ULONG index, APTR card,
   const struct TagItem *io_tags, UWORD bus, struct DevBase *base)
{
   BOOL success = TRUE;
   struct DevUnit *unit;
   struct Task *task;
   struct MsgPort *port;
   UWORD i;
   ULONG *desc, desc_p, buffer_p, dma_size;
   APTR stack;

   unit = AllocMem(sizeof(struct DevUnit), MEMF_CLEAR | MEMF_PUBLIC);
   if(unit == NULL)
      success = FALSE;

   if(success)
   {
      /* Initialise lists etc. */

      NewList((APTR)&unit->openers);
      NewList((APTR)&unit->type_trackers);
      NewList((APTR)&unit->multicast_ranges);

      unit->index = index;
      unit->device = base;
      unit->card = card;
      unit->bus = bus;

      /* Store I/O hooks */

      unit->ByteIn =
         (APTR)GetTagData(IOTAG_ByteIn, (UPINT)NULL, io_tags);
      unit->ByteOut =
         (APTR)GetTagData(IOTAG_ByteOut, (UPINT)NULL, io_tags);
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
      if(unit->ByteIn == NULL
         || unit->ByteOut == NULL
         || unit->LEWordIn == NULL
         || unit->LELongIn == NULL
         || unit->LEWordOut == NULL
         || unit->LELongOut == NULL
         || unit->AllocDMAMem == NULL
         || unit->FreeDMAMem == NULL)
         success = FALSE;
   }

   if(success)
   {
      InitSemaphore(&unit->access_lock);

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
         }
      }
      if(success)
         unit->request_ports[WRITE_QUEUE]->mp_SigTask = &unit->tx_int;
   }

   if(success)
   {
      /* Allocate TX descriptors */

      desc = unit->AllocDMAMem(unit->card, TX_DESC_SIZE * TX_SLOT_COUNT, 16);
      if(desc == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Fill arrays of virtual and physical addresses of TX descriptors */

      dma_size = TX_DESC_SIZE * TX_SLOT_COUNT;
      desc_p = (ULONG)(UPINT)CachePreDMA(desc, &dma_size, 0);
      if(dma_size != TX_DESC_SIZE * TX_SLOT_COUNT)
         success = FALSE;
      CachePostDMA(desc, &dma_size, 0);

      for(i = 0; i < TX_SLOT_COUNT; i++)
      {
         unit->tx_descs[i] = desc;
         desc += TX_DESC_SIZE / sizeof(ULONG);
         unit->tx_descs_p[i] = desc_p;
         desc_p += TX_DESC_SIZE;
      }
   }

   if(success)
   {
      /* Allocate RX descriptors */

      desc = unit->AllocDMAMem(unit->card, RH_DESCSIZE * RX_SLOT_COUNT, 16);
      if(desc == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Fill arrays of virtual and physical addresses of TX descriptors */

      dma_size = RH_DESCSIZE * RX_SLOT_COUNT;
      desc_p = (ULONG)(UPINT)CachePreDMA(desc, &dma_size, 0);
      if(dma_size != RH_DESCSIZE * RX_SLOT_COUNT)
         success = FALSE;
      CachePostDMA(desc, &dma_size, 0);

      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         unit->rx_descs[i] = desc;
         desc += RH_DESCSIZE / sizeof(ULONG);
         unit->rx_descs_p[i] = desc_p;
         desc_p += RH_DESCSIZE;
      }

      /* Allocate packet buffers */

      unit->tx_buffer = unit->AllocDMAMem(unit->card, FRAME_BUFFER_SIZE, 4);
      if(unit->tx_buffer == NULL)
         success = FALSE;

      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         unit->rx_buffers[i] = unit->AllocDMAMem(unit->card,
            FRAME_BUFFER_SIZE, 4);
         if(unit->rx_buffers[i] == NULL)
            success = FALSE;
      }

      unit->tx_requests = AllocVec(sizeof(APTR) * TX_SLOT_COUNT,
         MEMF_PUBLIC);
      if(unit->tx_requests == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Construct RX ring */

      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         desc = unit->rx_descs[i];
         desc[RH_DESC_RXSTATUS] = MakeLELong(RH_DESC_RXSTATUSF_INUSE);
         desc[RH_DESC_RXCONTROL] = MakeLELong(FRAME_BUFFER_SIZE);
         dma_size = FRAME_BUFFER_SIZE;
         buffer_p =
            (ULONG)(UPINT)CachePreDMA(unit->rx_buffers[i], &dma_size, 0);
         if(dma_size != FRAME_BUFFER_SIZE)
            success = FALSE;
         desc[RH_DESC_DATA] = MakeLELong(buffer_p);
         desc[RH_DESC_NEXT] =
            MakeLELong(unit->rx_descs_p[(i + 1) % RX_SLOT_COUNT]);
         desc += RH_DESCSIZE / sizeof(ULONG);
      }
      dma_size = RH_DESCSIZE * RX_SLOT_COUNT;
      CachePreDMA(unit->rx_descs, &dma_size, 0);
   }

   if(success)
   {
      /* Initialise network adapter hardware */

      success = InitialiseAdapter(unit, FALSE, base);
      unit->flags |= UNITF_HAVEADAPTER;
   }

   if(success)
   {
      /* Record maximum speed in BPS */

      unit->speed = 100000000;

      /* Initialise interrupts */

      unit->status_int.is_Code = (APTR)StatusInt;
      unit->status_int.is_Data = unit;

      unit->rx_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->rx_int.is_Code = (APTR)RXInt;
      unit->rx_int.is_Data = unit;

      unit->tx_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->tx_int.is_Code = (APTR)TXInt;
      unit->tx_int.is_Data = unit;

      unit->tx_end_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->tx_end_int.is_Code = (APTR)TXEndInt;
      unit->tx_end_int.is_Data = unit;

      unit->reset_handler.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->reset_handler.is_Code = (APTR)ResetHandler;
      unit->reset_handler.is_Data = unit;

      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

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
      task->tc_Node.ln_Name = base->device.dd_Library.lib_Node.ln_Name;
      task->tc_SPUpper = stack + STACK_SIZE;
      task->tc_SPLower = stack;
      task->tc_SPReg = stack + STACK_SIZE;
      NewList(&task->tc_MemEntry);

      if(AddTask(task, UnitTask, NULL) == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Send the unit to the new task */

      task->tc_UserData = unit;
   }

   if(!success)
   {
      DeleteUnit(unit, base);
      unit = NULL;
   }

   return unit;
}



/****i* rhine.device/DeleteUnit ********************************************
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
*	unit - Device unit (may be NULL).
*
*   RESULT
*	None.
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

      if((unit->flags & UNITF_ONLINE) != 0)   /* Needed! */
         GoOffline(unit, base);

      if((unit->flags & UNITF_HAVEADAPTER) != 0)
         DeinitialiseAdapter(unit, base);

      for(i = 0; i < RX_SLOT_COUNT; i++)
         unit->FreeDMAMem(unit->card, unit->rx_buffers[i]);
      unit->FreeDMAMem(unit->card, unit->tx_buffer);
      unit->FreeDMAMem(unit->card, unit->rx_descs[0]);
      unit->FreeDMAMem(unit->card, unit->tx_descs[0]);

      FreeVec(unit->tx_requests);

      FreeMem(unit, sizeof(struct DevUnit));
   }

   return;
}



/****i* rhine.device/InitialiseAdapter *************************************
*
*   NAME
*	InitialiseAdapter
*
*   SYNOPSIS
*	success = InitialiseAdapter(unit, reinsertion)
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
*	success - Success indicator.
*
****************************************************************************
*
*/

BOOL InitialiseAdapter(struct DevUnit *unit, BOOL reinsertion,
   struct DevBase *base)
{
   BOOL success = TRUE;
   UBYTE *p, eeprom_reg;
   UWORD i;

   /* Reload data from EEPROM */

   eeprom_reg = unit->ByteIn(unit->card, RH_REG_EEPROM);
   unit->ByteOut(unit->card, RH_REG_EEPROM,
      eeprom_reg | RH_REG_EEPROMF_LOAD);

   BusyMicroDelay(1, base);
   while((unit->ByteIn(unit->card, RH_REG_EEPROM) & RH_REG_EEPROMF_LOAD)
      != 0);

   /* Get default MAC address */

   p = unit->default_address;
   for(i = 0; i < ETH_ADDRESSSIZE; i++)
      *p++ = unit->ByteIn(unit->card, RH_REG_ADDRESS + i);

   /* Send reset command */

   unit->ByteOut(unit->card, RH_REG_CONTROL + 1, RH_REG_CONTROL1F_RESET);
   BusyMicroDelay(110, base);
   while((unit->ByteIn(unit->card, RH_REG_CONTROL + 1)
      & RH_REG_CONTROL1F_RESET) != 0);

   /* Disable interrupts */

   unit->LEWordOut(unit->card, RH_REG_INTSTATUS, INT_MASK);
   unit->LEWordOut(unit->card, RH_REG_INTMASK, 0);
   if(TRUE)
      unit->LEWordOut(unit->card, RH_REG_MIIINTMASK, 0);

   /* Configure MII */

   unit->ByteOut(unit->card, RH_REG_MIICTRL,
      unit->ByteIn(unit->card, RH_REG_MIICTRL) | RH_REG_MIICTRLF_AUTOPOLL);

   if(TRUE)
      unit->mii_phy_no = unit->ByteIn(unit->card, RH_REG_MIICONFIG)
         & RH_REG_MIICONFIGF_PHYADDR;
   else
      unit->mii_phy_no = 1;

   /* Return */

   return success;
}



/****i* rhine.device/DeinitialiseAdapter ***********************************
*
*   NAME
*	DeinitialiseAdapter
*
*   SYNOPSIS
*	DeinitialiseAdapter(unit)
*
*	VOID DeinitialiseAdapter(struct DevUnit *);
*
*   FUNCTION
*
*   INPUTS
*	unit
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

VOID DeinitialiseAdapter(struct DevUnit *unit, struct DevBase *base)
{
   return;
}



/****i* rhine.device/ConfigureAdapter **************************************
*
*   NAME
*	ConfigureAdapter -- Set up card for transmission/reception.
*
*   SYNOPSIS
*	ConfigureAdapter(unit)
*
*	VOID ConfigureAdapter(struct DevUnit *);
*
****************************************************************************
*
*/

VOID ConfigureAdapter(struct DevUnit *unit, struct DevBase *base)
{
   const UBYTE *p;
   UWORD i;

   /* Set MAC address */

   p = unit->address;
   for(i = 0; i < ETH_ADDRESSSIZE; i++)
      unit->ByteOut(unit->card, RH_REG_ADDRESS + i, *p++);

   /* Set DMA rings */

   unit->LELongOut(unit->card, RH_REG_TXLIST, unit->tx_descs_p[0]);
   unit->LELongOut(unit->card, RH_REG_RXLIST, unit->rx_descs_p[0]);

   /* Choose packet types to receive */

   unit->LEWordOut(unit->card, RH_REG_PCIBUSCONFIG, 6);
   unit->ByteOut(unit->card, RH_REG_RXCONFIG,
      RH_REG_RXCONFIGF_BCAST | RH_REG_RXCONFIGF_MCAST);

   /* Return */

   return;
}



/****i* rhine.device/GoOnline **********************************************
*
*   NAME
*	GoOnline -- Enable transmission/reception.
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
   /* Enable interrupts */

   unit->flags |= UNITF_ONLINE;
   unit->LEWordOut(unit->card, RH_REG_INTMASK, INT_MASK);

   /* Enable frame transmission and reception */

   unit->LEWordOut(unit->card, RH_REG_CONTROL, RH_REG_CONTROLF_TXENABLE
      | RH_REG_CONTROLF_RXENABLE | RH_REG_CONTROLF_START);

   /* Record start time and report Online event */

   GetSysTime(&unit->stats.LastStart);
   ReportEvents(unit, S2EVENT_ONLINE, base);

   return;
}



/****i* rhine.device/GoOffline *********************************************
*
*   NAME
*	GoOffline -- Disable transmission/reception.
*
*   SYNOPSIS
*	GoOffline(unit)
*
*	VOID GoOffline(struct DevUnit *);
*
*   FUNCTION
*
*   INPUTS
*	unit
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

VOID GoOffline(struct DevUnit *unit, struct DevBase *base)
{
   unit->flags &= ~UNITF_ONLINE;

   /* Flush pending read and write requests */

   FlushUnit(unit, MGMT_QUEUE, S2ERR_OUTOFSERVICE, base);

   /* Report Offline event and return */

   ReportEvents(unit, S2EVENT_OFFLINE, base);
   return;
}



/****i* rhine.device/AddMulticastRange *************************************
*
*   NAME
*	AddMulticastRange
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
         unit->range_count++;
         SetMulticast(unit, base);
         Enable();
      }
   }

   return range != NULL;
}



/****i* rhine.device/RemMulticastRange *************************************
*
*   NAME
*	RemMulticastRange
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
         unit->range_count--;
         SetMulticast(unit, base);
         Enable();
         FreeMem(range, sizeof(struct AddressRange));
      }
   }

   return range != NULL;
}



/****i* rhine.device/FindMulticastRange ************************************
*
*   NAME
*	FindMulticastRange
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

   while(range != tail && !found)
   {
      if(lower_bound_left == range->lower_bound_left &&
         lower_bound_right == range->lower_bound_right &&
         upper_bound_left == range->upper_bound_left &&
         upper_bound_right == range->upper_bound_right)
         found = TRUE;
      else
         range = (APTR)range->node.mln_Succ;
   }

   if(!found)
      range = NULL;

   return range;
}



/****i* rhine.device/SetMulticast ******************************************
*
*   NAME
*	SetMulticast
*
*   SYNOPSIS
*	SetMulticast(unit)
*
*	VOID SetMulticast(struct DevUnit *);
*
****************************************************************************
*
*/

static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base)
{
   if(!IsListEmpty(&unit->multicast_ranges))
   {
      unit->LELongOut(unit->card, RH_REG_MCASTFILTER, 0xffffffff);
      unit->LELongOut(unit->card, RH_REG_MCASTFILTER + 4, 0xffffffff);
   }
   else
   {
      unit->LELongOut(unit->card, RH_REG_MCASTFILTER, 0);
      unit->LELongOut(unit->card, RH_REG_MCASTFILTER + 4, 0);
   }

   return;
}



/****i* rhine.device/FindTypeStats *****************************************
*
*   NAME
*	FindTypeStats
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



/****i* rhine.device/FlushUnit *********************************************
*
*   NAME
*	FlushUnit
*
*   SYNOPSIS
*	FlushUnit(unit, last_queue, error)
*
*	VOID FlushUnit(struct DevUnit *, UBYTE, BYTE);
*
****************************************************************************
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

   /* Flush every opener's read queues */

   while(opener != tail)
   {
      while((request = (APTR)GetMsg(&opener->read_port)) != NULL)
      {
         request->io_Error = error;
         ReplyMsg((APTR)request);
      }
      while((request = (APTR)GetMsg(&opener->mgmt_port)) != NULL)
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
   while((request = (APTR)GetMsg(&opener->mgmt_port)) != NULL)
   {
      request->io_Error = IOERR_ABORTED;
      ReplyMsg((APTR)request);
   }
#endif

   /* Return */

   return;
}



/****i* rhine.device/StatusInt *********************************************
*
*   NAME
*       StatusInt
*
*   SYNOPSIS
*       finished = StatusInt(unit)
*
*       BOOL StatusInt(struct DevUnit *);
*
*   INPUTS
*       unit - A unit of this device.
*
*   RESULT
*       finished - Always FALSE.
*
****************************************************************************
*
*/

static BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   struct DevBase *base;
   UWORD ints;

   base = unit->device;
   ints = unit->LEWordIn(unit->card, RH_REG_INTSTATUS);

   if(ints != 0)
   {
      /* Acknowledge interrupts */

      unit->LEWordOut(unit->card, RH_REG_INTSTATUS, ints);

/* FIXME: Need IO-sync here for PPC etc.? */

      /* Handle interrupts */

      if((ints & (RH_INTF_TXERR | RH_INTF_TXOK)) != 0)
         Cause(&unit->tx_end_int);
      if((ints & (RH_INTF_RXERR | RH_INTF_RXOK)) != 0)
         Cause(&unit->rx_int);
   }

   return FALSE;
}



/****i* rhine.device/RXInt *************************************************
*
*   NAME
*	RXInt -- Soft interrupt for packet reception.
*
*   SYNOPSIS
*	RXInt(unit)
*
*	VOID RXInt(struct DevUnit *);
*
*   FUNCTION
*
*   INPUTS
*	unit - A unit of this device.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

static VOID RXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   UWORD slot, packet_size;
   struct DevBase *base;
   ULONG rx_status, *desc, dma_size;
   UBYTE *buffer;

   base = unit->device;
   slot = unit->rx_slot;
   desc = unit->rx_descs[slot];

   dma_size = TX_DESC_SIZE;
   CachePostDMA(unit->rx_descs, &dma_size, 0);
   while(((rx_status = LELong(desc[RH_DESC_RXSTATUS]))
      & RH_DESC_RXSTATUSF_INUSE) == 0)
   {
      if((rx_status & RH_DESC_RXSTATUSF_OK) != 0)
      {
         packet_size = ((rx_status & RH_DESC_RXSTATUSF_LENGTH)
            >> RH_DESC_RXSTATUSB_LENGTH) - ETH_CRCSIZE;
         buffer = unit->rx_buffers[slot];

         if(AddressFilter(unit, buffer + ETH_PACKET_DEST, base))
         {
            unit->stats.PacketsReceived++;
            DistributeRXPacket(unit, buffer, packet_size, base);
         }
      }
      else
      {
         unit->stats.BadData++;
         ReportEvents(unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX,
            base);
      }

      /* Mark descriptor as free for next time */

      desc[RH_DESC_RXSTATUS] = MakeLELong(RH_DESC_RXSTATUSF_INUSE);

      /* Get next descriptor */

      slot = (slot + 1) % RX_SLOT_COUNT;
      desc = unit->rx_descs[slot];
   }

   /* Return */

   unit->rx_slot = slot;
   return;
}



/****i* rhine.device/DistributeRXPacket ************************************
*
*   NAME
*	DistributeRXPacket -- Send a packet to all appropriate destinations.
*
*   SYNOPSIS
*	DistributeRXPacket(unit, frame)
*
*	VOID DistributeRXPacket(struct DevUnit *, UBYTE *);
*
****************************************************************************
*
*/

static VOID DistributeRXPacket(struct DevUnit *unit, const UBYTE *packet,
   UWORD packet_size, struct DevBase *base)
{
   BOOL is_orphan = TRUE, accepted;
   ULONG packet_type;
   struct IOSana2Req *request, *request_tail;
   struct Opener *opener, *opener_tail;
   struct TypeStats *tracker;

   /* Offer packet to every opener */

   opener = (APTR)unit->openers.mlh_Head;
   opener_tail = (APTR)&unit->openers.mlh_Tail;
   packet_type = BEWord(*((UWORD *)(packet + ETH_PACKET_TYPE)));

   while(opener != opener_tail)
   {
      request = (APTR)opener->read_port.mp_MsgList.lh_Head;
      request_tail = (APTR)&opener->read_port.mp_MsgList.lh_Tail;
      accepted = FALSE;

      /* Offer packet to each request until it's accepted */

      while(request != request_tail && !accepted)
      {
         if(request->ios2_PacketType == packet_type)
         {
            CopyPacket(unit, request, packet_size, packet_type,
               packet, base);
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
            (APTR)unit->request_ports[ADOPT_QUEUE]->mp_MsgList.lh_Head,
            packet_size, packet_type, packet, base);
      }
   }

   /* Update remaining statistics */

   if(packet_type <= ETH_MTU)
      packet_type = ETH_MTU;
   tracker =
      FindTypeStats(unit, &unit->type_trackers, packet_type, base);
   if(tracker != NULL)
   {
      tracker->stats.PacketsReceived++;
      tracker->stats.BytesReceived += packet_size;
   }

   return;
}



/****i* rhine.device/CopyPacket ********************************************
*
*   NAME
*	CopyPacket -- Copy packet to client's buffer.
*
*   SYNOPSIS
*	CopyPacket(unit, request, packet_size, packet_type,
*	    buffer)
*
*	VOID CopyPacket(struct DevUnit *, struct IOSana2Req *, UWORD, UWORD,
*	    UBYTE *);
*
****************************************************************************
*
*/

static VOID CopyPacket(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD packet_size, UWORD packet_type, const UBYTE *buffer,
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
   if(request->ios2_Req.io_Command == CMD_READ &&
      opener->filter_hook != NULL)
      if(!CallHookPkt(opener->filter_hook, request, (APTR)buffer))
         filtered = TRUE;

   if(!filtered)
   {
      /* Copy packet into opener's buffer and reply packet */

      if(!opener->rx_function(request->ios2_Data, (APTR)buffer,
         packet_size))
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



/****i* rhine.device/AddressFilter *****************************************
*
*   NAME
*	AddressFilter -- Determine if an RX packet should be accepted.
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

   if(((address_left & 0x01000000) != 0) &&
      !((address_left == 0xffffffff) && (address_right == 0xffff)))
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



/****i* rhine.device/TXInt *************************************************
*
*   NAME
*	TXInt -- Soft interrupt for packet transmission.
*
*   SYNOPSIS
*	TXInt(unit)
*
*	VOID TXInt(struct DevUnit *);
*
*   FUNCTION
*
*   INPUTS
*	unit - A unit of this device.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   struct DevBase *base;
   UWORD packet_size, data_size, slot, new_slot, *p, *q, i;
   struct IOSana2Req *request;
   BOOL proceed = TRUE;
   struct Opener *opener;
   ULONG wire_error, *desc, dma_size, desc_p, buffer_p, txcontrol_value;
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   BYTE error;
   UBYTE *buffer;
   struct MsgPort *port;

   base = unit->device;
   port = unit->request_ports[WRITE_QUEUE];

   while(proceed && !IsMsgPortEmpty(port))
   {
      slot = unit->tx_in_slot;
      new_slot = (slot + 1) % TX_SLOT_COUNT;

      if(new_slot != unit->tx_out_slot
         && (unit->flags & UNITF_TXBUFFERINUSE) == 0)
      {
         error = 0;

         /* Get request and DMA frame descriptor */

         request = (APTR)port->mp_MsgList.lh_Head;

         Remove((APTR)request);
         unit->tx_requests[slot] = request;
         desc = unit->tx_descs[slot];
         desc_p = unit->tx_descs_p[slot];

         data_size = packet_size = request->ios2_DataLength;
         if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
            packet_size += ETH_HEADERSIZE;
         if(packet_size < ETH_MINSIZE)
            packet_size = ETH_MINSIZE;

         /* Decide if one or two descriptors are needed, and generate
            Ethernet header if necessary */

         if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
         {
            /* Use first descriptor for Ethernet header */

            buffer = (UBYTE *)desc + 2 * RH_DESCSIZE;
            buffer_p = desc_p + 2 * RH_DESCSIZE;

            desc[RH_DESC_TXCONTROL] =
               MakeLELong(RH_DESC_TXCONTROLF_FIRSTFRAG
               | RH_DESC_TXCONTROLF_CHAIN | ETH_HEADERSIZE);
            desc[RH_DESC_DATA] = MakeLELong(buffer_p);
            desc[RH_DESC_NEXT] = MakeLELong(desc_p + RH_DESCSIZE);

            /* Write Ethernet header */

            p = (UWORD *)buffer;
            for(i = 0, q = (UWORD *)request->ios2_DstAddr;
               i < ETH_ADDRESSSIZE / 2; i++)
               *p++ = *q++;
            for(i = 0, q = (UWORD *)unit->address;
               i < ETH_ADDRESSSIZE / 2; i++)
               *p++ = *q++;
            *p++ = MakeBEWord(request->ios2_PacketType);

            /* Use spare second descriptor for frame data */

            desc += RH_DESCSIZE / 4;
            desc_p += RH_DESCSIZE;

            txcontrol_value = RH_DESC_TXCONTROLF_INT
               | RH_DESC_TXCONTROLF_LASTFRAG
               | RH_DESC_TXCONTROLF_CHAIN
               | packet_size - ETH_HEADERSIZE;
         }
         else
         {
            txcontrol_value = RH_DESC_TXCONTROLF_INT
               | RH_DESC_TXCONTROLF_FIRSTFRAG | RH_DESC_TXCONTROLF_LASTFRAG
               | RH_DESC_TXCONTROLF_CHAIN
               | packet_size;
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
            if(opener->tx_function(buffer, request->ios2_Data,
               data_size))
            {
               unit->flags |= UNITF_TXBUFFERINUSE;
            }
            else
            {
               error = S2ERR_NO_RESOURCES;
               wire_error = S2WERR_BUFF_ERROR;
               ReportEvents(unit,
                  S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF
                  | S2EVENT_TX, base);
            }
         }
         unit->tx_buffers[slot] = buffer;

         /* Fill in descriptor for frame data */

         if(error == 0)
         {
            dma_size = data_size;
            buffer_p = (ULONG)(UPINT)CachePreDMA(buffer, &dma_size,
               DMA_ReadFromRAM);
            desc[RH_DESC_TXCONTROL] = MakeLELong(txcontrol_value);
            desc[RH_DESC_DATA] = MakeLELong(buffer_p);
            desc[RH_DESC_NEXT] = MakeLELong(unit->tx_descs_p[new_slot]);

            if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
               desc -= RH_DESCSIZE / 4;

            desc[RH_DESC_TXSTATUS] = MakeLELong(RH_DESC_TXSTATUSF_INUSE);
         }

         if(error == 0)
         {
            /* Restart transmission if it had stopped */

            dma_size = TX_DESC_SIZE * TX_SLOT_COUNT;
            CachePreDMA(unit->tx_descs, &dma_size, 0);
            unit->ByteOut(unit->card, RH_REG_CONTROL,
               unit->ByteIn(unit->card, RH_REG_CONTROL)
               | RH_REG_CONTROLF_TXPOLL);

/* FIXME: Need IO-sync here for PPC etc.? */

            unit->tx_in_slot = new_slot;
         }
         else
         {
            /* Return failed request */

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



/****i* rhine.device/TXEndInt **********************************************
*
*   NAME
*       TXEndInt -- Clean up after a frame has been sent.
*
*   SYNOPSIS
*       TXEndInt(unit, int_code)
*
*       VOID TXEndInt(struct DevUnit *, APTR);
*
*   INPUTS
*       unit - A unit of this device.
*       int_code - Unused.
*
*   RESULT
*       None.
*
****************************************************************************
*
* It appears to be safe to assume that there will always be at least one
* completed packet whenever this interrupt is called.
*
*/

static VOID TXEndInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   UWORD data_size, i = 0;
   UBYTE *buffer;
   struct DevBase *base;
   ULONG *desc, dma_size;
   BOOL proceed = TRUE;

   /* Retire sent packets */

   base = unit->device;
   dma_size = TX_DESC_SIZE * TX_SLOT_COUNT;
   CachePostDMA(unit->tx_descs, &dma_size, 0);

   i = unit->tx_out_slot;
   while(proceed)
   {
      /* Skip to "spare" descriptor if valid */

      desc = unit->tx_descs[i];
      if((desc[RH_DESC_TXCONTROL] & RH_DESC_TXCONTROLF_LASTFRAG) == 0)
         desc += 4;

      /* Check that descriptor is not in use */

      if((desc[RH_DESC_TXSTATUS] & MakeLELong(RH_DESC_TXSTATUSF_INUSE))
         == 0 && i != unit->tx_in_slot)
      {
         buffer = unit->tx_buffers[i];
         dma_size = data_size;
         CachePostDMA(buffer, &dma_size, DMA_ReadFromRAM);

         /* Check if unit's buffer is now free */

         if(buffer == unit->tx_buffer)
            unit->flags &= ~UNITF_TXBUFFERINUSE;

         RetireTXSlot(unit, i, base);

         i = (i + 1) % TX_SLOT_COUNT;
      }
      else
         proceed = FALSE;
   }

   unit->tx_out_slot = i;

   dma_size = TX_DESC_SIZE * TX_SLOT_COUNT;
   CachePreDMA(unit->tx_descs, &dma_size, 0);

   /* Restart downloads if they had stopped */

   if(unit->request_ports[WRITE_QUEUE]->mp_Flags == PA_IGNORE)
      Cause(&unit->tx_int);

   return;
}



/****i* rhine.device/RetireTXSlot ******************************************
*
*   NAME
*	RetireTXSlot -- Reply finished TX request.
*
*   SYNOPSIS
*	RetireTXSlot(unit, slot)
*
*	VOID RetireTXSlot(struct DevUnit *, UWORD);
*
****************************************************************************
*
*/

static VOID RetireTXSlot(struct DevUnit *unit, UWORD slot,
   struct DevBase *base)
{
   UWORD frame_size;
   struct IOSana2Req *request;
   struct TypeStats *tracker;

   /* Update statistics */

   slot = unit->tx_out_slot;
   request = unit->tx_requests[slot];
   frame_size = request->ios2_DataLength;
   if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
      frame_size += ETH_HEADERSIZE;

   unit->stats.PacketsSent++;

   tracker = FindTypeStats(unit, &unit->type_trackers,
      request->ios2_PacketType, base);
   if(tracker != NULL)
   {
      tracker->stats.PacketsSent++;
      tracker->stats.BytesSent += frame_size;
   }

   /* Reply request */

   request->ios2_Req.io_Error = 0;
   ReplyMsg((APTR)request);

   return;
}



/****i* rhine.device/ResetHandler ******************************************
*
*   NAME
*       ResetHandler -- Disable hardware before a reboot.
*
*   SYNOPSIS
*       ResetHandler(unit, int_code)
*
*       VOID ResetHandler(struct DevUnit *, APTR);
*
****************************************************************************
*
*/

static VOID ResetHandler(REG(a1, struct DevUnit *unit),
   REG(a6, APTR int_code))
{
   if((unit->flags & UNITF_HAVEADAPTER) != 0)
   {
      /* Disable frame transmission and reception */

      unit->LEWordOut(unit->card, RH_REG_CONTROL, RH_REG_CONTROLF_STOP);

      /* Stop interrupts */

      unit->LEWordOut(unit->card, RH_REG_INTMASK, 0);

   }

   return;
}



/****i* rhine.device/ReportEvents ******************************************
*
*   NAME
*	ReportEvents
*
*   SYNOPSIS
*	ReportEvents(unit, events)
*
*	VOID ReportEvents(struct DevUnit *, ULONG);
*
*   FUNCTION
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



/****i* rhine.device/UnitTask **********************************************
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
*	Completes deferred requests, and handles card insertion and removal
*	in conjunction with the relevant interrupts.
*
****************************************************************************
*
*/

static VOID UnitTask(struct ExecBase *sys_base)
{
   struct Task *task;
   struct IORequest *request;
   struct DevUnit *unit;
   struct DevBase *base;
   struct MsgPort *general_port;
   ULONG signals = 0, wait_signals, card_removed_signal,
      card_inserted_signal, general_port_signal;

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

   /* Allocate signals for notification of card removal and insertion */

   card_removed_signal = unit->card_removed_signal = 1 << AllocSignal(-1);
   card_inserted_signal = unit->card_inserted_signal = 1 << AllocSignal(-1);
   wait_signals = (1 << general_port->mp_SigBit) | card_removed_signal
      | card_inserted_signal | SIGBREAKF_CTRL_C;

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
            if((unit->flags & UNITF_WASONLINE) != 0)
            {
               GoOnline(unit, base);
               unit->flags &= ~UNITF_WASONLINE;
            }
         }
      }

      if((signals & card_removed_signal) != 0)
      {
         unit->removal_function(unit->card, base);
         if((unit->flags & UNITF_WASONLINE) != 0)
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



/****i* rhine.device/ReadMII ***********************************************
*
*   NAME
*       ReadMII -- Read a register in an MII PHY.
*
*   SYNOPSIS
*       value = ReadMII(unit, phy_no, reg_no)
*
*       UWORD ReadMII(struct DevUnit *, UWORD, UWORD);
*
*   INPUTS
*       unit - A unit of this device.
*       phy_no - Index of PHY to use.
*       reg_no - MII register to read.
*
*   RESULT
*       value - Value read from MII register.
*
****************************************************************************
*
*/

UWORD ReadMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   struct DevBase *base)
{
   while((unit->ByteIn(unit->card, RH_REG_MIICTRL)
      & (RH_REG_MIICTRLF_READ | RH_REG_MIICTRLF_WRITE)) != 0);

   unit->ByteOut(unit->card, RH_REG_MIICTRL, 0);
   unit->ByteOut(unit->card, RH_REG_MIICONFIG, phy_no);
   unit->ByteOut(unit->card, RH_REG_MIIREGNO, reg_no);
   unit->ByteOut(unit->card, RH_REG_MIICTRL, RH_REG_MIICTRLF_READ);
   while((unit->ByteIn(unit->card, RH_REG_MIICTRL)
      & (RH_REG_MIICTRLF_READ | RH_REG_MIICTRLF_WRITE)) != 0);

   return unit->LEWordIn(unit->card, RH_REG_MIIDATA);
}



