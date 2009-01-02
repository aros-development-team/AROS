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
#include "intelpro100.h"
#include "mii.h"
#include "dp83840.h"

#include "unit_protos.h"
#include "request_protos.h"


#define TASK_PRIORITY 0
#define STACK_SIZE 4096
#define CONFIG_DATA_LEN 6
#define STAT_COUNT 16
#define MAX_MCAST_ENTRIES 100
#define MCAST_CB_SIZE (PROCB_COUNT * sizeof(ULONG) + sizeof(UWORD) \
   + ETH_ADDRESSSIZE * MAX_MCAST_ENTRIES)

#ifndef AbsExecBase
#define AbsExecBase (*(struct ExecBase **)4)
#endif

static VOID InitialiseAdapter(struct DevUnit *unit, struct DevBase *base);
static VOID FillConfigData(struct DevUnit *unit, ULONG *tcb,
   struct DevBase *base);
static struct AddressRange *FindMulticastRange(struct DevUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
   UWORD upper_bound_right, struct DevBase *base);
static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base);
static BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID RXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID CopyPacket(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD packet_size, UWORD packet_type, UBYTE *buffer,
   struct DevBase *base);
static BOOL AddressFilter(struct DevUnit *unit, UBYTE *address,
   struct DevBase *base);
static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID TXEndInt(REG(a1, struct DevUnit *unit),
   REG(a5, APTR int_code));
static VOID ReportEvents(struct DevUnit *unit, ULONG events,
   struct DevBase *base);
static VOID UnitTask();
static UWORD GetEEPROMAddressSize(struct DevUnit *unit,
   struct DevBase *base);
static UWORD ReadEEPROM(struct DevUnit *unit, UWORD index,
   struct DevBase *base);
static ULONG ReadEEPROMBits(struct DevUnit *unit, UBYTE count,
   struct DevBase *base);
static VOID WriteEEPROMBits(struct DevUnit *unit, ULONG value, UBYTE count,
   struct DevBase *base);
static BOOL ReadEEPROMBit(struct DevUnit *unit, struct DevBase *base);
static BOOL WriteEEPROMBit(struct DevUnit *unit, BOOL is_one,
   struct DevBase *base);
static UWORD ReadMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   struct DevBase *base);
static VOID WriteMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   UWORD value, struct DevBase *base);
static VOID BusyMicroDelay(ULONG micros, struct DevBase *base);



static const ULONG config_data[] =
{
   8 << PROCB_CF0B_RXFIFOLIM | 22 << PROCB_CF0B_SIZE,
   1 << PROCB_CF1B_UNDERRUNRETRIES | PROCB_CF1F_DISCARDRUNTS
      | PROCB_CF1F_STDSTATS | PROCB_CF1F_STDTXBLOCK | 0x00020000,
   2 << PROCB_CF2B_PREAMBLETYPE | PROCB_CF2F_NOSOURCEINSERT | 0x00060001,
   0xf200 << PROCB_CF3B_ARPFILTER | 6 << PROCB_CF3B_IFS | 0x48000000,
   PROCB_CF4F_USEFDPIN | PROCB_CF4F_PADDING | 0x00f04000,
   0x053f
};


#ifdef __amigaos4__
#undef AddTask
#define AddTask(task, initial_pc, final_pc) \
   IExec->AddTask(task, initial_pc, final_pc, NULL)
#endif
#ifdef __MORPHOS__
static const struct EmulLibEntry mos_task_trap =
{
   TRAP_LIB,
   0,
   (APTR)UnitTask
};
#define UnitTask &mos_task_trap
#endif


/****i* intelpro100.device/CreateUnit **************************************
*
*   NAME
*	CreateUnit -- Create a unit.
*
*   SYNOPSIS
*	unit = CreateUnit(index, card, io_tags, bus)
*
*	struct DevUnit *CreateUnit(ULONG, APTR, struct TagItem *, UWORD);
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
   UBYTE i;
   APTR stack;
   ULONG *tcb, *next_tcb, *rcb, *next_rcb, dma_size;
   APTR rx_int_function, tx_int_function;

   unit = AllocMem(sizeof(struct DevUnit), MEMF_CLEAR | MEMF_PUBLIC);
   if(unit == NULL)
      success = FALSE;

   if(success)
   {
      /* Initialise lists etc. */

      InitSemaphore(&unit->access_lock);
      NewList((APTR)&unit->openers);
      NewList((APTR)&unit->type_trackers);
      NewList((APTR)&unit->multicast_ranges);
      NewList((APTR)&unit->tx_requests);

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
         || unit->LEWordOut == NULL || unit->LELongIn == NULL
         || unit->LELongOut == NULL
         || unit->AllocDMAMem == NULL || unit->FreeDMAMem == NULL)
         success = FALSE;

      /* Allocate buffer for adapter to writes statistics to */

      unit->stats_buffer =
         AllocVec(sizeof(ULONG) * (STAT_COUNT + 1), MEMF_PUBLIC);
      if(unit->stats_buffer == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Initialise network adapter hardware */

      InitialiseAdapter(unit, base);
      unit->flags |= UNITF_HAVEADAPTER;

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

      /* Allocate and initialise packet/command descriptors */

      unit->tx_buffer = AllocVec(ETH_MAXPACKETSIZE, MEMF_PUBLIC);
      next_tcb = unit->first_tcb = unit->tcbs =
         AllocVec(TCB_SIZE * TX_SLOT_COUNT, MEMF_PUBLIC | MEMF_CLEAR);
      next_rcb = unit->rcbs =
         AllocVec(RCB_SIZE * RX_SLOT_COUNT, MEMF_PUBLIC | MEMF_CLEAR);
      unit->multicast_cb = AllocVec(PROCB_COUNT * sizeof(ULONG)
         + sizeof(UWORD) + ETH_ADDRESSSIZE * MAX_MCAST_ENTRIES,
         MEMF_PUBLIC | MEMF_CLEAR);

      if(next_tcb == NULL || next_rcb == NULL || unit->tx_buffer == NULL
         || unit->multicast_cb == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Construct TX ring */

      for(i = 0; i < TX_SLOT_COUNT; i++)
      {
         tcb = next_tcb;
         next_tcb = tcb + TCB_SIZE / sizeof(ULONG);
         tcb[PROCB_NEXT] = MakeLELong((ULONG)next_tcb);
      }
      tcb[PROCB_NEXT] = MakeLELong((ULONG)unit->tcbs);
      unit->last_tcb = tcb;

      /* Construct RX ring */

      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         rcb = next_rcb;
         next_rcb = rcb + RCB_SIZE / sizeof(ULONG);
         rcb[PROCB_NEXT] = MakeLELong((ULONG)next_rcb);
         rcb[PROCB_RXINFO] =
            MakeLELong(ETH_MAXPACKETSIZE << PROCB_RXINFOB_BUFFERSIZE);
      }
      rcb[PROCB_CONTROL] = MakeLELong(PROCB_CONTROLF_SUSPEND);
      rcb[PROCB_NEXT] = MakeLELong((ULONG)unit->rcbs);
      unit->last_rcb = rcb;
      dma_size = RCB_SIZE * RX_SLOT_COUNT;
      CachePreDMA(unit->rcbs, &dma_size, 0);

      /* Record maximum speed in BPS */

      unit->speed = 100000000;

      /* Initialise status, transmit and receive interrupts */

      unit->status_int.is_Code = (APTR)StatusInt;
      unit->status_int.is_Data = unit;

      rx_int_function = RXInt;
      unit->rx_int.is_Node.ln_Name = (TEXT *)device_name;
      unit->rx_int.is_Node.ln_Pri = 16;
      unit->rx_int.is_Code = rx_int_function;
      unit->rx_int.is_Data = unit;

      tx_int_function = TXInt;
      unit->tx_int.is_Node.ln_Name = (TEXT *)device_name;
      unit->tx_int.is_Code = tx_int_function;
      unit->tx_int.is_Data = unit;

      unit->tx_end_int.is_Node.ln_Name = (TEXT *)device_name;
      unit->tx_end_int.is_Code = TXEndInt;
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
      /* Allocate stack */

      stack = AllocMem(STACK_SIZE, MEMF_PUBLIC);
      if(stack == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Initialise and start task */

      task->tc_Node.ln_Type = NT_TASK;
      task->tc_Node.ln_Pri = TASK_PRIORITY;
      task->tc_Node.ln_Name = (APTR)device_name;
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



/****i* intelpro100.device/DeleteUnit **************************************
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
            RemTask(task);
         if(task->tc_SPLower != NULL)
            FreeMem(task->tc_SPLower, STACK_SIZE);
         FreeMem(task, sizeof(struct Task));
      }

      for(i = 0; i < REQUEST_QUEUE_COUNT; i++)
      {
         if(unit->request_ports[i] != NULL)
            FreeMem(unit->request_ports[i], sizeof(struct MsgPort));
      }

      if((unit->flags & UNITF_ONLINE) != 0)   /* Needed! */
         GoOffline(unit, base);

      FreeVec(unit->multicast_cb);
      FreeVec(unit->rcbs);
      FreeVec(unit->tcbs);
      FreeVec(unit->tx_buffer);
      FreeVec(unit->stats_buffer);

      FreeMem(unit, sizeof(struct DevUnit));
   }

   return;
}



/****i* intelpro100.device/InitialiseAdapter *******************************
*
*   NAME
*	InitialiseAdapter -- Initialise network adapter hardware.
*
*   SYNOPSIS
*	InitialiseAdapter(unit)
*
*	VOID InitialiseAdapter(struct DevUnit *);
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

static VOID InitialiseAdapter(struct DevUnit *unit, struct DevBase *base)
{
   UBYTE *p, i;
   UWORD address_part;

   /* Reset card */

   unit->LELongOut(unit->card, PROREG_PORT, 0);
   BusyMicroDelay(10, base);

   /* Get default MAC address */

   unit->eeprom_addr_size = GetEEPROMAddressSize(unit, base);
   p = unit->default_address;

   for(i = 0; i < ETH_ADDRESSSIZE / sizeof(UWORD); i++)
   {
      address_part = ReadEEPROM(unit, PROROM_ADDRESS0 + i, base);
      *p++ = address_part & 0xff;
      *p++ = address_part >> 8;
   }

   /* Set up statistics dump area */

   unit->LELongOut(unit->card, PROREG_GENPTR, (ULONG)unit->stats_buffer);
   unit->LEWordOut(unit->card, PROREG_COMMAND, PRO_CUCMD_SETSTATSBUFFER);
   while(unit->LEWordIn(unit->card, PROREG_COMMAND) != 0);

   /* Return */

   return;
}



/****i* intelpro100.device/ConfigureAdapter ********************************
*
*   NAME
*	ConfigureAdapter -- Put adapter online for the first time.
*
*   SYNOPSIS
*	ConfigureAdapter(unit)
*
*	VOID ConfigureAdapter(struct DevUnit *);
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

VOID ConfigureAdapter(struct DevUnit *unit, struct DevBase *base)
{
   ULONG *tcb, dma_size;
   UWORD phy_type, phy_no, config;

   /* Set MAC address */

   tcb = (ULONG *)LELong(unit->last_tcb[PROCB_NEXT]);
   tcb[PROCB_CONTROL] = MakeLELong(PROACT_SETADDRESS);
   CopyMem(&unit->address, tcb + PROCB_ADDRESS, ETH_ADDRESSSIZE);

   /* Set other parameters */

   tcb = (ULONG *)LELong(tcb[PROCB_NEXT]);
   tcb[PROCB_CONTROL] =
      MakeLELong(PROACT_CONFIGURE | PROCB_CONTROLF_SUSPEND);
   unit->phy_info = ReadEEPROM(unit, PROROM_PHYINFO0, base);
   FillConfigData(unit, tcb, base);
   unit->last_tcb = tcb;

   dma_size = TCB_SIZE * TX_SLOT_COUNT;
   CachePreDMA(unit->tcbs, &dma_size, 0);

   /* DP83840-specific configuration */

   phy_type = unit->phy_info & PROROM_PHYINFO0F_TYPE;
   if(phy_type == PRO_PHY_DP83840 || phy_type == PRO_PHY_DP83840A)
   {
      phy_no = unit->phy_info & PROROM_PHYINFO0F_ADDR;
      config = ReadMII(unit, phy_no, MII_PCR, base);
      config |= 0x400 | MII_PCRF_NOLINKMON | MII_PCRF_LED4DUPLEX;
      WriteMII(unit, phy_no, MII_PCR, config, base);
   }

   /* Go online */

   unit->LELongOut(unit->card, PROREG_GENPTR, (ULONG)unit->tcbs);
   unit->LEWordOut(unit->card, PROREG_COMMAND, PRO_CUCMD_START);
   while(unit->LEWordIn(unit->card, PROREG_COMMAND) != 0);
   GoOnline(unit, base);

   /* Return */

   return;
}



/****i* intelpro100.device/FillConfigData **********************************
*
*   NAME
*	FillConfigData -- Fill in the data for a configuration command.
*
*   SYNOPSIS
*	FillConfigData(unit, tcb)
*
*	VOID FillConfigData(struct DevUnit *, ULONG *);
*
*   INPUTS
*	unit - A unit of this device.
*	tcb - The configuration command block.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

static VOID FillConfigData(struct DevUnit *unit, ULONG *tcb,
   struct DevBase *base)
{
   UBYTE i;
   const ULONG *p;
   ULONG *q;

   /* Copy constant parameters from template */

   for(p = config_data, q = tcb + PROCB_CF0, i = 0; i < CONFIG_DATA_LEN;
      i++)
      *q++ = MakeLELong(*p++);
   unit->last_tcb = tcb;

   /* Decide on promiscuous mode */

   if((unit->flags & UNITF_PROM) != 0)
      tcb[PROCB_CF3] |= MakeLELong(PROCB_CF3F_PROM);

   /* AUI or MII? */

   if((unit->phy_info & PROROM_PHYINFO0F_AUI) != 0)
      tcb[PROCB_CF3] |= MakeLELong(PROCB_CF3F_CDT);
   else
      tcb[PROCB_CF2] |= MakeLELong(PROCB_CF2F_MIIMODE);

   /* Accept all multicasts? */

   if((unit->flags & UNITF_ALLMCAST ) != 0)
   {
      tcb[PROCB_CF5] |= MakeLELong(PROCB_CF5F_ALLMCAST);
   }

   /* Return */

   return;
}



/****i* intelpro100.device/GoOnline ****************************************
*
*   NAME
*	GoOnline -- Put the adapter online.
*
*   SYNOPSIS
*	GoOnline(unit)
*
*	VOID GoOnline(struct DevUnit *);
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

VOID GoOnline(struct DevUnit *unit, struct DevBase *base)
{
   /* Enable the transceiver */

   unit->flags |= UNITF_ONLINE;

   unit->LELongOut(unit->card, PROREG_GENPTR,
      LELong(unit->last_rcb[PROCB_NEXT]));
   unit->LEWordOut(unit->card, PROREG_COMMAND, PRO_RUCMD_START);
   while(unit->LEWordIn(unit->card, PROREG_COMMAND) != 0);

   /* Record start time and report Online event */

   GetSysTime(&unit->stats.LastStart);
   ReportEvents(unit, S2EVENT_ONLINE, base);

   return;
}



/****i* intelpro100.device/GoOffline ***************************************
*
*   NAME
*	GoOffline -- Put the adpater offline.
*
*   SYNOPSIS
*	GoOffline(unit)
*
*	VOID GoOffline(struct DevUnit *);
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

VOID GoOffline(struct DevUnit *unit, struct DevBase *base)
{
   unit->flags &= ~UNITF_ONLINE;

   if((unit->flags & UNITF_HAVEADAPTER) != 0)
   {
      /* Stop reception */

      unit->LEWordOut(unit->card, PROREG_COMMAND, PRO_RUCMD_ABORT);

      /* Update statistics */

      UpdateStats(unit, base);
   }

   /* Flush pending read and write requests */

   FlushUnit(unit, WRITE_QUEUE, S2ERR_OUTOFSERVICE, base);

   /* Report Offline event and return */

   ReportEvents(unit, S2EVENT_OFFLINE, base);
   return;
}



/****i* intelpro100.device/AddMulticastRange *******************************
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
         Enable();
      }
   }

   return range != NULL;
}



/****i* intelpro100.device/RemMulticastRange *******************************
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
         Enable();
         FreeMem(range, sizeof(struct AddressRange));
      }
   }

   return range != NULL;
}



/****i* intelpro100.device/FindMulticastRange ******************************
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



/****i* intelpro100.device/SetMulticast ************************************
*
*   NAME
*	SetMulticast
*
*   SYNOPSIS
*	SetMulticast(unit)
*
*	VOID SetMulticast(struct DevUnit *);
*
*   FUNCTION
*	Fills in the unit's multicast list TCB, but does not send it to the
*	device.
*
****************************************************************************
*
*/

static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base)
{
   ULONG *tcb, address_left;
   UWORD address_right, i = 0, *p;
   struct AddressRange *range, *tail;
   BOOL range_ended;

   tcb = unit->multicast_cb;

   if((unit->flags & UNITF_PROM) == 0)
   {
      /* Fill in multicast list */

      range = (APTR)unit->multicast_ranges.mlh_Head;
      tail = (APTR)&unit->multicast_ranges.mlh_Tail;
      p = (UWORD *)(tcb + PROCB_COUNT);
      p++;
      while(range != tail && i < MAX_MCAST_ENTRIES)
      {
         address_left = range->lower_bound_left;
         address_right = range->lower_bound_right;
         range_ended = FALSE;

         while(!range_ended && i++ < MAX_MCAST_ENTRIES)
         {
            *p++ = MakeBEWord((UWORD)(address_left >> 16));
            *p++ = MakeBEWord((UWORD)(address_left));
            *p++ = MakeBEWord((UWORD)(address_right));

            if(address_left == range->upper_bound_left &&
               address_right == range->upper_bound_right)
            {
               range_ended = TRUE;
            }
            if(++address_right == 0)
               address_left++;
         }

         if(range_ended)
            range = (APTR)range->node.mln_Succ;
      }
      p = (UWORD *)(tcb + PROCB_COUNT);
      *p = MakeLEWord(ETH_ADDRESSSIZE * i);

      /* Accept all multicasts if there are too many addresses */

      if(range != tail)
         unit->flags |= UNITF_ALLMCAST;
      else
         unit->flags &= ~UNITF_ALLMCAST;
   }

   return;
}



/****i* intelpro100.device/FindTypeStats ***********************************
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

   while((stats != tail) && !found)
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



/****i* intelpro100.device/FlushUnit ***************************************
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



/****i* intelpro100.device/StatusInt ***************************************
*
*   NAME
*	StatusInt
*
*   SYNOPSIS
*	finished = StatusInt(unit)
*
*	BOOL StatusInt(struct DevUnit *);
*
*   INPUTS
*	unit - A unit of this device.
*
*   RESULT
*	finished - Always FALSE.
*
****************************************************************************
*
*/

static BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   struct DevBase *base;
   UWORD ints;

   base = unit->device;
   ints = unit->ByteIn(unit->card, PROREG_INTSTATUS);

   if(ints != 0)
   {
      /* Handle interrupts */

      if((ints & PROINTF_GENERAL) != 0)
         Cause(&unit->tx_end_int);
      if((ints & PROINTF_RXDONE) != 0)
         Cause(&unit->rx_int);

      /* Acknowledge all interrupts */

      unit->ByteOut(unit->card, PROREG_INTSTATUS, 0xff);
   }

   return FALSE;
}



/****i* intelpro100.device/RXInt *******************************************
*
*   NAME
*	RXInt
*
*   SYNOPSIS
*	RXInt(unit)
*
*	VOID RXInt(struct DevUnit *);
*
*   INPUTS
*	unit - A unit of this device.
*	int_code - Unused.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

static VOID RXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   UWORD packet_size;
   struct DevBase *base;
   BOOL is_orphan, accepted;
   ULONG rx_status_le, rx_info, packet_type, *rcb, *last_rcb, dma_size;
   UBYTE *buffer;
   struct IOSana2Req *request, *request_tail;
   struct Opener *opener, *opener_tail;
   struct TypeStats *tracker;

   base = unit->device;
   last_rcb = unit->last_rcb;
   rcb = (ULONG *)LELong(last_rcb[PROCB_NEXT]);

   dma_size = RCB_SIZE;
   CachePostDMA(rcb, &dma_size, 0);
   while(((rx_status_le = rcb[PROCB_CONTROL])
      & MakeLELong(PROCB_CONTROLF_DONE)) != 0)
   {
      if((rx_status_le & MakeLELong(PROCB_CONTROLF_OK)) != 0)
      {
         is_orphan = TRUE;
         rx_info = LELong(rcb[PROCB_RXINFO]);
         packet_size = rx_info & PROCB_RXINFOF_FRAMESIZE;
         buffer = (UBYTE *)(rcb + PROCB_BUFFER);

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
                  if(request->ios2_PacketType == packet_type)
                  {
                     CopyPacket(unit, request, packet_size, packet_type,
                        buffer, base);
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
                     base);
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

      rcb[PROCB_CONTROL] = MakeLELong(PROCB_CONTROLF_SUSPEND);
      rcb[PROCB_RXINFO] =
         MakeLELong(ETH_MAXPACKETSIZE << PROCB_RXINFOB_BUFFERSIZE);

      /* Clear suspend flag from previous CB without touching bits that
         adapter may be writing to; and resume execution */

      *(((UBYTE *)last_rcb + PROCB_CONTROL) + 3) = 0;
      dma_size = RCB_SIZE;
      CachePreDMA(last_rcb, &dma_size, 0);
      if(TRUE)
         unit->LEWordOut(unit->card, PROREG_COMMAND, PRO_RUCMD_RESUME);

      last_rcb = rcb;
      rcb = (ULONG *)LELong(rcb[PROCB_NEXT]);
   }

   /* Return */

   unit->last_rcb = last_rcb;
   return;
}



/****i* intelpro100.device/CopyPacket **************************************
*
*   NAME
*	CopyPacket
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
   UWORD packet_size, UWORD packet_type, UBYTE *buffer,
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



/****i* intelpro100.device/AddressFilter ***********************************
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
         if(((address_left > range->lower_bound_left) ||
            (address_left == range->lower_bound_left) &&
            (address_right >= range->lower_bound_right)) &&
            ((address_left < range->upper_bound_left) ||
            (address_left == range->upper_bound_left) &&
            (address_right <= range->upper_bound_right)))
            accept = TRUE;
         range = (APTR)range->node.mln_Succ;
      }

      if(!accept)
         unit->special_stats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
   }

   return accept;
}



/****i* intelpro100.device/TXInt *******************************************
*
*   NAME
*	TXInt
*
*   SYNOPSIS
*	TXInt(unit)
*
*	VOID TXInt(struct DevUnit *);
*
*   INPUTS
*	unit - A unit of this device.
*	int_code - Unused.
*
*   RESULT
*	None.
*
****************************************************************************
*
* Note that when the CU is resumed, the adapter examines the suspend flag
* again in the command that caused the suspension. If the flag is set, the
* CU will be suspended without executing any new commands. This means that
* all TCBs can't be in use at the same time, and the dynamically inserted
* multicast list command can't have its suspend flag set.
*
*/

static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   struct DevBase *base;
   UWORD packet_size, data_size, *p, *q, i;
   struct IOSana2Req *request;
   BOOL proceed = TRUE;
   struct Opener *opener;
   ULONG wire_error, *tcb, *last_tcb, *mcast_cb, *fragment, dma_size,
      action;
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   BYTE error;
   UBYTE *buffer;
   struct MsgPort *port;

   base = unit->device;
   port = unit->request_ports[WRITE_QUEUE];

   while(proceed && !IsMsgPortEmpty(port))
   {
      last_tcb = unit->last_tcb;
      tcb = (ULONG *)LELong(last_tcb[PROCB_NEXT]);

      /* Ensure there are at least two free CBs available (two are needed
         for setting the multicast filter) and that neither the TX nor the
         multicast buffer is currently in use */

      if((ULONG *)LELong(((ULONG *)LELong(tcb[PROCB_NEXT]))[PROCB_NEXT])
         != unit->first_tcb
         && (unit->flags & (UNITF_TXBUFFERINUSE | UNITF_MCASTBUFFERINUSE))
         == 0)
      {
         error = 0;
         request = (APTR)GetMsg(port);
         request->ios2_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;

         switch(request->ios2_Req.io_Command)
         {
         case CMD_WRITE:
         case S2_MULTICAST:
         case S2_BROADCAST:
            action = PROACT_TX;
            break;
         default:
            action = PROACT_CONFIGURE;
         }

         if(action == PROACT_TX)
         {
            /* Handle TX request */

            data_size = packet_size = request->ios2_DataLength;

            if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
               packet_size += ETH_HEADERSIZE;

            /* Write packet preamble */

            tcb[PROCB_CONTROL] =
               MakeLELong(PROACT_TX | PROCB_CONTROLF_SUSPEND
               | PROCB_CONTROLF_INT | PROCB_CONTROLF_FLEXIBLE);
            fragment = tcb + PROCB_EXTFRAGS;
            tcb[PROCB_FRAGMENTS] = MakeLELong((ULONG)fragment);

            /* Write packet header */

            if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
            {
               buffer = (UBYTE *)(tcb + PROCB_EXTBUFFER);
               tcb[PROCB_TXINFO] =
                  MakeLELong(2 << PROCB_TXINFOB_FRAGCOUNT
                  | 1 << PROCB_TXINFOB_THRESHOLD
                  | PROCB_TXINFOF_EOF);
               fragment[PROFRAG_ADDR] = MakeLELong((ULONG)buffer);
               fragment[PROFRAG_LEN] = MakeLELong(ETH_HEADERSIZE);

               p = (UWORD *)buffer;
               for(i = 0, q = (UWORD *)request->ios2_DstAddr;
                  i < ETH_ADDRESSSIZE / 2; i++)
                  *p++ = *q++;
               for(i = 0, q = (UWORD *)unit->address;
                  i < ETH_ADDRESSSIZE / 2; i++)
                  *p++ = *q++;
               *p++ = MakeBEWord(request->ios2_PacketType);
               buffer = (UBYTE *)p;

               fragment += PRO_FRAGLEN;
            }
            else
            {
               tcb[PROCB_TXINFO] =
                  MakeLELong(1 << PROCB_TXINFOB_FRAGCOUNT
                  | 1 << PROCB_TXINFOB_THRESHOLD
                  | PROCB_TXINFOF_EOF);
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

            /* Put pointer to packet data into descriptor */

            if(error == 0)
            {
               dma_size = data_size;
               CachePreDMA(buffer, &dma_size, DMA_ReadFromRAM);
               fragment[PROFRAG_ADDR] = MakeLELong((ULONG)buffer);
               fragment[PROFRAG_LEN] = MakeLELong(data_size);
            }
         }
         else
         {
            /* Update multicast reception filter */

            SetMulticast(unit, base);
            if((unit->flags & UNITF_ALLMCAST) == 0)
            {
               tcb[PROCB_CONTROL] = MakeLELong(PROACT_NOP);
               mcast_cb = unit->multicast_cb;
               mcast_cb[PROCB_CONTROL] = MakeLELong(PROACT_SETMCAST);
               mcast_cb[PROCB_NEXT] = tcb[PROCB_NEXT];
               tcb[PROCB_NEXT] = MakeLELong((ULONG)mcast_cb);
               unit->link_cb = tcb;
               tcb = (ULONG *)LELong(mcast_cb[PROCB_NEXT]);
               tcb[PROCB_CONTROL] = MakeLELong(PROACT_CONFIGURE
                  | PROCB_CONTROLF_SUSPEND | PROCB_CONTROLF_INT);
               FillConfigData(unit, tcb, base);
               unit->flags |= UNITF_MCASTBUFFERINUSE;
            }
            else
            {
               tcb[PROCB_CONTROL] = MakeLELong(PROACT_CONFIGURE
                  | PROCB_CONTROLF_SUSPEND | PROCB_CONTROLF_INT);
               FillConfigData(unit, tcb, base);
            }
         }

         if(error == 0)
         {
            /* Clear suspend flag from previous CB without touching bits
               that adapter may be writing to; and resume execution */

            if(last_tcb[PROCB_CONTROL] != 0)
               *(((UBYTE *)last_tcb + PROCB_CONTROL) + 3) =
                  PROCB_CONTROLF_INT >> 24;
            dma_size = TCB_SIZE * TX_SLOT_COUNT;
            CachePreDMA(unit->tcbs, &dma_size, 0);
            dma_size = MCAST_CB_SIZE;
            CachePreDMA(unit->multicast_cb, &dma_size, 0);
            if((unit->ByteIn(unit->card, PROREG_STATUS)
               & PROREG_STATUSF_CUSTATE)
               == (PRO_CUSTATE_SUSPENDED << PROREG_STATUSB_CUSTATE))
            {
               unit->LEWordOut(unit->card, PROREG_COMMAND,
                  PRO_CUCMD_RESUME);
            }

            AddTail((APTR)&unit->tx_requests, (APTR)request);
            unit->last_tcb = tcb;
         }
         else
         {
            /* Return failed request */

            request->ios2_Req.io_Error = error;
            request->ios2_WireError = wire_error;
            ReplyMsg((APTR)request);
            tcb[PROCB_CONTROL] = 0;
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



/****i* intelpro100.device/TXEndInt ****************************************
*
*   NAME
*	TXEndInt
*
*   SYNOPSIS
*	TXEndInt(unit)
*
*	VOID TXEndInt(struct DevUnit *);
*
*   INPUTS
*	unit - A unit of this device.
*	int_code - Unused.
*
*   RESULT
*	None.
*
****************************************************************************
*
* It appears to be safe to assume that there will always be at least one
* completed packet whenever this interrupt is called.
*
*/

static VOID TXEndInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   UWORD data_size, packet_size;
   UBYTE *buffer;
   struct DevBase *base;
   struct IOSana2Req *request;
   ULONG *tcb, *fragment, dma_size, action;
   struct TypeStats *tracker;

   /* Retire sent packets and configuration commands */

   base = unit->device;
   dma_size = TCB_SIZE * TX_SLOT_COUNT;
   CachePostDMA(unit->tcbs, &dma_size, 0);
   dma_size = MCAST_CB_SIZE;
   CachePostDMA(unit->multicast_cb, &dma_size, 0);

   for(tcb = unit->first_tcb;
      (tcb[PROCB_CONTROL] & MakeLELong(PROCB_CONTROLF_DONE)) != 0;
      tcb = (ULONG *)LELong(tcb[PROCB_NEXT]))
   {
      action = LELong(tcb[PROCB_CONTROL]) & PROCB_CONTROLF_ACTION;

      if(action == PROACT_TX || action == PROACT_CONFIGURE
         && (unit->link_cb != NULL
         || (tcb[PROCB_CF5] & PROCB_CF5F_ALLMCAST) != 0))
      {
         request = (APTR)RemHead((APTR)&unit->tx_requests);

         if(action == PROACT_TX)
         {
            /* Mark end of DMA */

            data_size = packet_size = request->ios2_DataLength;
            fragment = (ULONG *)LELong(tcb[PROCB_FRAGMENTS]);

            if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
            {
               packet_size += ETH_HEADERSIZE;
               fragment += PRO_FRAGLEN;
            }

            buffer = (UBYTE *)LELong(fragment[PROFRAG_ADDR]);
            dma_size = data_size;
            CachePostDMA(buffer, &dma_size, DMA_ReadFromRAM);

            /* Check if unit's buffer is now free */

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
         }
         else if(action == PROACT_CONFIGURE)
         {
            /* Mark end of multicast update */

            if(unit->link_cb != NULL)
            {
               unit->link_cb[PROCB_NEXT] = MakeLELong((ULONG)tcb);
               unit->flags &= ~UNITF_MCASTBUFFERINUSE;
               unit->link_cb = NULL;
            }
         }

         /* Reply request */

         request->ios2_Req.io_Error = 0;
         ReplyMsg((APTR)request);
      }

      /* Mark CB as unused/clear suspend flag */

      tcb[PROCB_CONTROL] = 0;
   }

   dma_size = TCB_SIZE * TX_SLOT_COUNT;
   CachePreDMA(unit->tcbs, &dma_size, 0);

   unit->first_tcb = tcb;

   /* Restart downloads if they had stopped */

   if(unit->request_ports[WRITE_QUEUE]->mp_Flags == PA_IGNORE)
      Cause(&unit->tx_int);

   return;
}



/****i* intelpro100.device/UpdateStats *************************************
*
*   NAME
*	UpdateStats
*
*   SYNOPSIS
*	UpdateStats(unit)
*
*	VOID UpdateStats(struct DevUnit *);
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

VOID UpdateStats(struct DevUnit *unit, struct DevBase *base)
{
   ULONG dma_size, *buffer;

   buffer = unit->stats_buffer;
   buffer[STAT_COUNT] = 0;
   dma_size = sizeof(ULONG) * STAT_COUNT;
   CachePreDMA(unit->stats_buffer, &dma_size, 0);
   unit->ByteOut(unit->card, PROREG_COMMAND, PRO_CUCMD_DUMPRESETSTATS);
   while(buffer[STAT_COUNT] != MakeLELong(0xa007))
   {
      dma_size = sizeof(ULONG) * STAT_COUNT;
      CachePostDMA(unit->stats_buffer, &dma_size, 0);
   }

   unit->stats.Overruns += LELong(buffer[PROSTAT_RXOVERRUNS]);
   unit->stats.PacketsSent += LELong(buffer[PROSTAT_TXFRAMESOK]);
   unit->stats.PacketsReceived += LELong(buffer[PROSTAT_RXFRAMESOK]);
   unit->special_stats[S2SS_ETHERNET_RETRIES & 0xffff] +=
      LELong(buffer[PROSTAT_FRAMESDEFERRED]);
   unit->special_stats[S2SS_ETHERNET_FIFO_UNDERRUNS & 0xffff] +=
      LELong(buffer[PROSTAT_TXUNDERRUNS]);

   return;
}



/****i* intelpro100.device/ReportEvents ************************************
*
*   NAME
*	ReportEvents
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



/****i* intelpro100.device/UnitTask ****************************************
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
*	Completes deferred requests.
*
****************************************************************************
*
*/

#ifdef __MORPHOS__
#undef UnitTask
#endif

static VOID UnitTask()
{
   struct Task *task;
   struct IORequest *request;
   struct DevUnit *unit;
   struct DevBase *base;
   struct MsgPort *general_port;
   ULONG signals, wait_signals, general_port_signal;

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

   wait_signals = (1 << general_port->mp_SigBit);

   /* Tell ourselves to check port for old messages */

   Signal(task, general_port_signal);

   /* Infinite loop to service requests and signals */

   while(TRUE)
   {
      signals = Wait(wait_signals);

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



/****i* intelpro100.device/GetEEPROMAddressSize ****************************
*
*   NAME
*	GetEEPROMAddressSize
*
*   SYNOPSIS
*	size = GetEEPROMAddressSize(unit)
*
*	UWORD GetEEPROMAddressSize(struct DevUnit *);
*
*   INPUTS
*	unit - A unit of this device.
*
*   RESULT
*	size - Width of EEPROM addresses.
*
****************************************************************************
*
* Although the manual doesn't make it explicit, chip select must be asserted
* before setting any other bits, at least on the i82557.
*
*/

static UWORD GetEEPROMAddressSize(struct DevUnit *unit,
   struct DevBase *base)
{
   UWORD size;

   unit->LEWordOut(unit->card, PROREG_EEPROM, PROREG_EEPROMF_SELECT);
   WriteEEPROMBits(unit, 0x6, 3, base);
   for(size = 1; WriteEEPROMBit(unit, FALSE, base); size++);
   ReadEEPROMBits(unit, 16, base);
   unit->LEWordOut(unit->card, PROREG_EEPROM, 0);
   BusyMicroDelay(1, base);

   return size;
}



/****i* intelpro100.device/ReadEEPROM **************************************
*
*   NAME
*	ReadEEPROM -- Read an EEPROM location.
*
*   SYNOPSIS
*	value = ReadEEPROM(unit, index)
*
*	UWORD ReadEEPROM(struct DevUnit *, UWORD);
*
*   INPUTS
*	unit - A unit of this device.
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
   UWORD value;

   unit->LEWordOut(unit->card, PROREG_EEPROM, PROREG_EEPROMF_SELECT);
   WriteEEPROMBits(unit, 0x6, 3, base);
   WriteEEPROMBits(unit, index, unit->eeprom_addr_size, base);
   value = ReadEEPROMBits(unit, 16, base);
   unit->LEWordOut(unit->card, PROREG_EEPROM, 0);
   BusyMicroDelay(1, base);

   return value;
}



/****i* intelpro100.device/ReadEEPROMBits **********************************
*
*   NAME
*	ReadEEPROMBits -- Read a stream of bits from the EEPROM.
*
*   SYNOPSIS
*	value = ReadEEPROMBits(unit, count)
*
*	ULONG ReadEEPROMBits(struct DevUnit *, UBYTE);
*
*   INPUTS
*	unit - A unit of this device.
*	count - Number of bits to be read.
*
*   RESULT
*	value - The bits read from the EEPROM, right-justified.
*
****************************************************************************
*
*/

static ULONG ReadEEPROMBits(struct DevUnit *unit, UBYTE count,
   struct DevBase *base)
{
   UBYTE i;
   ULONG value = 0;

   for(i = 0; i < count; i++)
   {
      value <<= 1;
      if(ReadEEPROMBit(unit, base))
         value |= 0x1;
   }

   return value;
}


/****i* intelpro100.device/WriteEEPROMBits *********************************
*
*   NAME
*	WriteEEPROMBits -- Write a stream of bits to the EEPROM.
*
*   SYNOPSIS
*	WriteEEPROMBits(unit, value, count)
*
*	VOID WriteEEPROMBits(struct DevUnit *, ULONG, UBYTE);
*
*   INPUTS
*	unit - A unit of this device.
*	value - The bits to write to the EEPROM, right-justified.
*	count - Number of bits to be Write.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

static VOID WriteEEPROMBits(struct DevUnit *unit, ULONG value, UBYTE count,
   struct DevBase *base)
{
   ULONG mask;

   for(mask = 1 << (count - 1); mask != 0; mask >>= 1)
      WriteEEPROMBit(unit, (value & mask) != 0, base);

   return;
}



/****i* intelpro100.device/ReadEEPROMBit ***********************************
*
*   NAME
*	ReadEEPROMBit -- Read a bit from the EEPROM.
*
*   SYNOPSIS
*	value = ReadEEPROMBit(unit)
*
*	BOOL ReadEEPROMBit(struct DevUnit *);
*
*   INPUTS
*	unit - A unit of this device.
*
*   RESULT
*	value - True for one, false for zero.
*
****************************************************************************
*
*/

static BOOL ReadEEPROMBit(struct DevUnit *unit, struct DevBase *base)
{
   BOOL is_one;

   unit->LEWordOut(unit->card, PROREG_EEPROM,
      PROREG_EEPROMF_SELECT | PROREG_EEPROMF_CLK);
   BusyMicroDelay(2, base);
   is_one =
      (unit->LEWordIn(unit->card, PROREG_EEPROM) & PROREG_EEPROMF_DATAIN)
      != 0;
   unit->LEWordOut(unit->card, PROREG_EEPROM, PROREG_EEPROMF_SELECT);
   BusyMicroDelay(2, base);

   return is_one;
}



/****i* intelpro100.device/WriteEEPROMBit **********************************
*
*   NAME
*	WriteEEPROMBit -- Write a bit to the EEPROM.
*
*   SYNOPSIS
*	data_in = WriteEEPROMBit(unit, is_one)
*
*	BOOL WriteEEPROMBit(struct DevUnit *, BOOL);
*
*   INPUTS
*	unit - A unit of this device.
*	is_one - True if a set bit should be written.
*
*   RESULT
*	data_in - True if data-in bit is set.
*
****************************************************************************
*
*/

static BOOL WriteEEPROMBit(struct DevUnit *unit, BOOL is_one,
   struct DevBase *base)
{
   UWORD data_out;

   if(is_one)
      data_out = PROREG_EEPROMF_DATAOUT;
   else
      data_out = 0;

   unit->LEWordOut(unit->card, PROREG_EEPROM,
      PROREG_EEPROMF_SELECT | data_out);
   unit->LEWordOut(unit->card, PROREG_EEPROM,
      PROREG_EEPROMF_SELECT | PROREG_EEPROMF_CLK | data_out);
   BusyMicroDelay(2, base);
   unit->LEWordOut(unit->card, PROREG_EEPROM,
      PROREG_EEPROMF_SELECT | data_out);
   BusyMicroDelay(2, base);

   return
      (unit->LEWordIn(unit->card, PROREG_EEPROM) & PROREG_EEPROMF_DATAIN)
      != 0;
}



/****i* intelpro100.device/ReadMII *****************************************
*
*   NAME
*	ReadMII -- Read a register in an MII PHY.
*
*   SYNOPSIS
*	value = ReadMII(unit, phy_no, reg_no)
*
*	UWORD ReadMII(struct DevUnit *, UWORD, UWORD);
*
*   INPUTS
*	unit - A unit of this device.
*	phy_no - Index of PHY to use.
*	reg_no - MII register to read.
*
*   RESULT
*	value - Value read from MII register.
*
****************************************************************************
*
*/

static UWORD ReadMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   struct DevBase *base)
{
   ULONG value;

   unit->LELongOut(unit->card, PROREG_MIICONTROL,
      2 << PROREG_MIICONTROLB_CMD
      | phy_no << PROREG_MIICONTROLB_PHYNO
      | reg_no << PROREG_MIICONTROLB_REGNO);
   do
      value = unit->LELongIn(unit->card, PROREG_MIICONTROL);
   while((value & PROREG_MIICONTROLF_READY) == 0);

   return value & PROREG_MIICONTROLF_DATA;
}



/****i* intelpro100.device/WriteMII ****************************************
*
*   NAME
*	WriteMII -- Write to a register in an MII PHY.
*
*   SYNOPSIS
*	WriteMII(unit, phy_no, reg_no, value)
*
*	VOID WriteMII(struct DevUnit *, UWORD, UWORD, UWORD);
*
*   INPUTS
*	unit - A unit of this device.
*	phy_no - Index of PHY to use.
*	reg_no - MII register to write to.
*	value - Value to write to MII register.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

static VOID WriteMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   UWORD value, struct DevBase *base)
{
   unit->LELongOut(unit->card, PROREG_MIICONTROL,
      1 << PROREG_MIICONTROLB_CMD
      | phy_no << PROREG_MIICONTROLB_PHYNO
      | reg_no << PROREG_MIICONTROLB_REGNO
      | value);
   while
   (
      (
         unit->LELongIn(unit->card, PROREG_MIICONTROL)
         & PROREG_MIICONTROLF_READY
      )
      == 0
   );

   return;
}



/****i* prism2.device/BusyMicroDelay ***************************************
*
*   NAME
*	BusyMilliDelay - Busy-wait for specified number of microseconds.
*
*   SYNOPSIS
*	BusyMilliDelay(micros)
*
*	VOID BusyMilliDelay(ULONG);
*
****************************************************************************
*
*/

#if 1
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



