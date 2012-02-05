/*

Copyright (C) 2001-2011 Neil Cafferkey

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

#include "unit_protos.h"
#include "request_protos.h"
#include "radio_protos.h"
#include "eeprom_protos.h"
#include "encryption_protos.h"
#include "timer_protos.h"
#include "realtek8187.h"


#define TASK_PRIORITY 0
#define STACK_SIZE 4096
#define INT_MASK 0xffff
#define TX_TRIES 7
#define SIFS_TIME 14

#ifndef AbsExecBase
#define AbsExecBase (*(struct ExecBase **)4)
#endif

VOID DeinitialiseAdapter(struct DevUnit *unit, struct DevBase *base);
static struct AddressRange *FindMulticastRange(struct DevUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
   UWORD upper_bound_right, struct DevBase *base);
static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base);
static UBYTE *GetRXBuffer(struct DevUnit *unit, const UBYTE *address,
   UWORD frag_no, UWORD *buffer_no, struct DevBase *base);
static VOID DistributeRXPacket(struct DevUnit *unit, const UBYTE *frame,
   struct DevBase *base);
static VOID CopyPacket(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD packet_size, UWORD packet_type, UBYTE *buffer,
   struct DevBase *base);
static BOOL AddressFilter(struct DevUnit *unit, UBYTE *address,
   struct DevBase *base);
static VOID DistributeMgmtFrame(struct DevUnit *unit, UBYTE *frame,
   UWORD frame_size, struct DevBase *base);
static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
static VOID MgmtTXInt(REG(a1, struct DevUnit *unit),
   REG(a6, APTR int_code));
static VOID ReportEvents(struct DevUnit *unit, ULONG events,
   struct DevBase *base);
static UWORD GetDuration(struct DevUnit *unit, UWORD length, UWORD rate,
   BOOL short_preamble, struct DevBase *base);
static UWORD AckRate(struct DevUnit *unit, UWORD data_rate,
   struct DevBase *base);
static VOID UnitTask();


static const UBYTE snap_template[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
static const UBYTE broadcast_address[] =
   {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


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



/****i* realtek8180.device/CreateUnit **************************************
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
      unit->generation = RTL8187B1_GEN;
      unit->tx_rate = 11;
      unit->tx_rate_code = 3;   /* 11 Mbps */
      unit->mgmt_rate = 1;
      unit->mgmt_rate_code = 0; /* 1 Mbps */
      unit->channel = 1;

      /* Store I/O hooks */

      unit->ByteOut =
         (APTR)GetTagData(IOTAG_ByteOut, (UPINT)NULL, io_tags);
      unit->ByteIn =
         (APTR)GetTagData(IOTAG_ByteIn, (UPINT)NULL, io_tags);
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
      unit->SendFrame =
         (APTR)GetTagData(IOTAG_SendFrame, (UPINT)NULL, io_tags);
      unit->ReceiveFrame =
         (APTR)GetTagData(IOTAG_ReceiveFrame, (UPINT)NULL, io_tags);
      if(unit->ByteIn == NULL
         || unit->ByteOut == NULL
         || unit->LEWordIn == NULL
         || unit->LELongIn == NULL
         || unit->LEWordOut == NULL
         || unit->LELongOut == NULL
         || unit->AllocDMAMem == NULL
         || unit->FreeDMAMem == NULL
         || unit->SendFrame == NULL
         || unit->ReceiveFrame == NULL)
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
      {
         unit->request_ports[WRITE_QUEUE]->mp_SigTask = &unit->tx_int;
         unit->request_ports[MGMT_QUEUE]->mp_SigTask = &unit->mgmt_int;
      }

      /* Allocate buffers and descriptors */

      unit->tx_buffer = AllocVec(ETH_MAXPACKETSIZE, MEMF_PUBLIC);
      for(i = 0; i < TX_SLOT_COUNT; i++)
      {
         unit->tx_descs[i] = unit->AllocDMAMem(unit->card,
            R8180_MAXDESCSIZE + FRAME_BUFFER_SIZE, 4);
         if(unit->tx_descs[i] != NULL)
            unit->tx_buffers[i] = unit->tx_descs[i] + R8180_MAXDESCSIZE;
         else
            success = FALSE;
      }
      unit->rx_buffer = AllocVec(FRAME_BUFFER_SIZE, MEMF_PUBLIC);
      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         if(unit->bus != USB_BUS)
         {
            unit->rx_descs[i] = unit->AllocDMAMem(unit->card,
               R8180_MAXDESCSIZE, 4);
            if(unit->rx_descs[i] == NULL)
               success = FALSE;
         }
         unit->rx_buffers[i] = unit->AllocDMAMem(unit->card,
            FRAME_BUFFER_SIZE + R8180_MAXDESCSIZE, 4);
         if(unit->rx_buffers[i] == NULL)
            success = FALSE;
      }
      unit->rx_frames =
         AllocVec(FRAME_BUFFER_SIZE * FRAME_BUFFER_COUNT, MEMF_PUBLIC);
      for(i = 0; i < FRAME_BUFFER_COUNT; i++)
         unit->rx_fragment_nos[i] = -1;
      unit->tx_requests = AllocVec(sizeof(APTR) * TX_SLOT_COUNT,
         MEMF_PUBLIC);
      if(unit->tx_buffer == NULL
         || unit->rx_buffer == NULL
         || unit->rx_frames == NULL
         || unit->tx_requests == NULL)
         success = FALSE;
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

      unit->speed = 54000000;

      /* Initialise interrupts */

      unit->rx_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->rx_int.is_Code = (APTR)RXInt;
      unit->rx_int.is_Data = unit;

      unit->tx_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->tx_int.is_Code = (APTR)TXInt;
      unit->tx_int.is_Data = unit;

      unit->mgmt_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->mgmt_int.is_Code = (APTR)MgmtTXInt;
      unit->mgmt_int.is_Data = unit;

      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
      unit->request_ports[MGMT_QUEUE]->mp_Flags = PA_SOFTINT;

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

      /* Set default wireless options */

      unit->mode = S2PORT_MANAGED;
   }

   if(!success)
   {
      DeleteUnit(unit, base);
      unit = NULL;
   }

   return unit;
}



/****i* realtek8180.device/DeleteUnit **************************************
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

      for(i = 0; i < TX_SLOT_COUNT; i++)
         unit->FreeDMAMem(unit->card, unit->tx_descs[i]);
      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         unit->FreeDMAMem(unit->card, unit->rx_buffers[i]);
         if(unit->bus != USB_BUS)
            unit->FreeDMAMem(unit->card, unit->rx_descs[i]);
      }

      FreeVec(unit->tx_buffer);
      FreeVec(unit->rx_frames);
      FreeVec(unit->tx_requests);
      FreeVec(unit->rx_buffer);

      FreeMem(unit, sizeof(struct DevUnit));
   }

   return;
}



/****i* realtek8180.device/InitialiseAdapter *******************************
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
   BOOL success = FALSE;
   UBYTE reg_62, revision;
   UWORD *p, i;
   ULONG rx_conf;

   /* Initialise EEPROM */

   rx_conf = unit->LELongIn(unit->card, 0x100 + R8180REG_RXCONF);
   unit->eeprom_addr_size = ((rx_conf & (1 << 6)) != 0) ? 8 : 6;

   BusyMicroDelay(10, base);
   unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, R8180ECMD_CONFIG);

   /* Get default MAC address */

   p = (UWORD *)unit->default_address;
   for(i = 0; i < ETH_ADDRESSSIZE / sizeof(UWORD); i++)
      *p++ = LEWord(ReadEEPROM(unit, R8180ROM_ADDRESS0 + i, base));

   /* Refine main chip revision */

   if(unit->generation == RTL8187B1_GEN)
   {
      revision = unit->ByteIn(unit->card, 0x100 + 0xe1);
      if(revision == 1 || revision == 2)
         unit->generation = RTL8187B2_GEN;
   }
   else
   {
      if((unit->LELongIn(unit->card, 0x100 + R8180REG_TXCONF)
         & R8180REG_TXCONFF_HWVER) == 6 << R8180REG_TXCONFB_HWVER)
         unit->generation = RTL8187B0_GEN;
   }

   /* Set up power tables */

   GetPower(unit, base);

   /* Tune the radio */

   unit->ByteOut(unit->card, 0x100 + R8180REG_CONFIG3,
      unit->ByteIn(unit->card, 0x100 + R8180REG_CONFIG3)
      | R8180REG_CONFIG3F_ANAPARAMWRITE | R8180REG_CONFIG3F_GNTSELECT);
   unit->LELongOut(unit->card, 0x100 + R8180REG_ANAPARAM2, 0x727f3f52);
   unit->LELongOut(unit->card, 0x100 + R8180REG_ANAPARAM1, 0x45090658);
   unit->ByteOut(unit->card, 0x100 + R8180REG_ANAPARAM3, 0);

   unit->ByteOut(unit->card, 0x100 + R8180REG_CONFIG3,
      unit->ByteIn(unit->card, 0x100 + R8180REG_CONFIG3)
      & ~R8180REG_CONFIG3F_ANAPARAMWRITE);

   /* Reset PLL sequence */

   unit->ByteOut(unit->card, 0x161, 0x10);
   reg_62 = unit->ByteIn(unit->card, 0x162);
   unit->ByteOut(unit->card, 0x162, reg_62  & ~(1 << 5));
   unit->ByteOut(unit->card, 0x162, reg_62  | (1 << 5));

   /* Send reset command */

   unit->ByteOut(unit->card, 0x100 + R8180REG_COMMAND,
      unit->ByteIn(unit->card, 0x100 + R8180REG_COMMAND)
      & 0x2 | R8180REG_COMMANDF_RESET);

   for(i = 0; i < 10 && !success; i++)
   {
      BusyMilliDelay(2, base);
      if((unit->ByteIn(unit->card, 0x100 + R8180REG_COMMAND)
         & R8180REG_COMMANDF_RESET) == 0)
         success = TRUE;
   }

   if(success && unit->generation == RTL8187L_GEN)
   {
      /* Reload registers from EEPROM */

      unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, R8180ECMD_LOAD);

      for(i = 0, success = FALSE; i < 10 && !success; i++)
      {
         BusyMilliDelay(4, base);
         if((unit->ByteIn(unit->card, 0x100 + R8180REG_EEPROM)
            & R8180REG_EEPROMF_COMMAND) == 0)
            success = TRUE;
      }
   }

   if(success)
   {
      /* Set up rates */

      if(unit->generation == RTL8187L_GEN)
         unit->LEWordOut(unit->card, 0x12d, 0xfff);
      else
         unit->LEWordOut(unit->card, 0x134, 0xfff);
      unit->LEWordOut(unit->card, 0x12c, 0x1ff);
      unit->ByteOut(unit->card, 0x100 + R8180REG_CWCONF,
         unit->ByteIn(unit->card, 0x100 + R8180REG_CWCONF)
         | R8180REG_CWCONFF_PPRETRYSHIFT);
      unit->ByteOut(unit->card, 0x100 + R8180REG_TXAGCCTL,
         unit->ByteIn(unit->card, 0x100 + R8180REG_TXAGCCTL)
         | R8180REG_TXAGCCTLF_PPGAINSHIFT);

      unit->LEWordOut(unit->card, 0x1e0 | 1 << 16, 0xfff);
      unit->ByteOut(unit->card, 0x100 + R8180REG_RATEFALLBACK,
         unit->ByteIn(unit->card, 0x100 + R8180REG_RATEFALLBACK)
         | R8180REG_RATEFALLBACKF_ENABLE);

      unit->ByteOut(unit->card, 0x100 + R8180REG_MSR,
         unit->ByteIn(unit->card, 0x100 + R8180REG_MSR) & 0xf3);
      unit->ByteOut(unit->card, 0x100 + R8180REG_MSR,
         unit->ByteIn(unit->card, 0x100 + R8180REG_MSR)
         | R8180REG_MSRF_ENEDCA);
      unit->ByteOut(unit->card, 0x100 + R8180REG_ACMCONTROL, 0);

      unit->LEWordOut(unit->card, 0x100 + R8180REG_ATIMWINDOW, 2);
      unit->LEWordOut(unit->card, 0x100 + R8180REG_BEACONINTERVAL, 100);
      unit->LEWordOut(unit->card, 0x1d4 | 1 << 16, 0xffff);

      unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, R8180ECMD_CONFIG);
      unit->ByteOut(unit->card, 0x100 + R8180REG_CONFIG1,
         unit->ByteIn(unit->card, 0x100 + R8180REG_CONFIG1) & 0x3f | 0x80);
      unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, 0);

      unit->ByteOut(unit->card, 0x100 + R8180REG_WPACONF, 0);

      unit->ByteOut(unit->card, 0x1f0, 0x32);
      unit->ByteOut(unit->card, 0x1f1, 0x32);
      unit->ByteOut(unit->card, 0x1f2, 0x0);
      unit->ByteOut(unit->card, 0x1f3, 0x0);
      unit->ByteOut(unit->card, 0x1f4, 0x32);
      unit->ByteOut(unit->card, 0x1f5, 0x43);
      unit->ByteOut(unit->card, 0x1f6, 0x0);
      unit->ByteOut(unit->card, 0x1f7, 0x0);
      unit->ByteOut(unit->card, 0x1f8, 0x46);
      unit->ByteOut(unit->card, 0x1f9, 0xa4);
      unit->ByteOut(unit->card, 0x1fa, 0x0);
      unit->ByteOut(unit->card, 0x1fb, 0x0);
      unit->ByteOut(unit->card, 0x1fc, 0x96);
      unit->ByteOut(unit->card, 0x1fd, 0xa4);
      unit->ByteOut(unit->card, 0x1fe, 0x0);
      unit->ByteOut(unit->card, 0x1ff, 0x0);

      unit->ByteOut(unit->card, 0x158 | 1 << 16, 0x4b);
      unit->ByteOut(unit->card, 0x159 | 1 << 16, 0x0);
      unit->ByteOut(unit->card, 0x15a | 1 << 16, 0x4b);
      unit->ByteOut(unit->card, 0x15b | 1 << 16, 0x0);
      unit->ByteOut(unit->card, 0x160 | 1 << 16, 0x4b);
      unit->ByteOut(unit->card, 0x161 | 1 << 16, 0x9);
      unit->ByteOut(unit->card, 0x162 | 1 << 16, 0x4b);
      unit->ByteOut(unit->card, 0x163 | 1 << 16, 0x9);
      unit->ByteOut(unit->card, 0x1ce | 1 << 16, 0xf);
      unit->ByteOut(unit->card, 0x1cf | 1 << 16, 0x0);
      unit->ByteOut(unit->card, 0x1e0 | 1 << 16, 0xff);
      unit->ByteOut(unit->card, 0x1e1 | 1 << 16, 0xf);
      unit->ByteOut(unit->card, 0x1e2 | 1 << 16, 0x0);
      unit->ByteOut(unit->card, 0x1f0 | 1 << 16, 0x4e);
      unit->ByteOut(unit->card, 0x1f1 | 1 << 16, 0x1);
      unit->ByteOut(unit->card, 0x1f2 | 1 << 16, 0x2);
      unit->ByteOut(unit->card, 0x1f3 | 1 << 16, 0x3);
      unit->ByteOut(unit->card, 0x1f4 | 1 << 16, 0x4);
      unit->ByteOut(unit->card, 0x1f5 | 1 << 16, 0x5);
      unit->ByteOut(unit->card, 0x1f6 | 1 << 16, 0x6);
      unit->ByteOut(unit->card, 0x1f7 | 1 << 16, 0x7);
      unit->ByteOut(unit->card, 0x1f8 | 1 << 16, 0x8);

      unit->ByteOut(unit->card, 0x14e | 2 << 16, 0x0);
      unit->ByteOut(unit->card, 0x10c | 2 << 16, 0x4);
      unit->ByteOut(unit->card, 0x121 | 2 << 16, 0x61);
      unit->ByteOut(unit->card, 0x122 | 2 << 16, 0x68);
      unit->ByteOut(unit->card, 0x123 | 2 << 16, 0x6f);
      unit->ByteOut(unit->card, 0x124 | 2 << 16, 0x76);
      unit->ByteOut(unit->card, 0x125 | 2 << 16, 0x7d);
      unit->ByteOut(unit->card, 0x126 | 2 << 16, 0x84);
      unit->ByteOut(unit->card, 0x127 | 2 << 16, 0x8d);
      unit->ByteOut(unit->card, 0x14d | 2 << 16, 0x8);
      unit->ByteOut(unit->card, 0x150 | 2 << 16, 0x5);
      unit->ByteOut(unit->card, 0x151 | 2 << 16, 0xf5);
      unit->ByteOut(unit->card, 0x152 | 2 << 16, 0x4);
      unit->ByteOut(unit->card, 0x153 | 2 << 16, 0xa0);
      unit->ByteOut(unit->card, 0x154 | 2 << 16, 0x1f);
      unit->ByteOut(unit->card, 0x155 | 2 << 16, 0x23);
      unit->ByteOut(unit->card, 0x156 | 2 << 16, 0x45);
      unit->ByteOut(unit->card, 0x157 | 2 << 16, 0x67);
      unit->ByteOut(unit->card, 0x158 | 2 << 16, 0x8);
      unit->ByteOut(unit->card, 0x159 | 2 << 16, 0x8);
      unit->ByteOut(unit->card, 0x15a | 2 << 16, 0x8);
      unit->ByteOut(unit->card, 0x15b | 2 << 16, 0x8);
      unit->ByteOut(unit->card, 0x160 | 2 << 16, 0x8);
      unit->ByteOut(unit->card, 0x161 | 2 << 16, 0x8);
      unit->ByteOut(unit->card, 0x162 | 2 << 16, 0x8);
      unit->ByteOut(unit->card, 0x163 | 2 << 16, 0x8);
      unit->ByteOut(unit->card, 0x164 | 2 << 16, 0xcf);
      unit->ByteOut(unit->card, 0x172 | 2 << 16, 0x56);
      unit->ByteOut(unit->card, 0x173 | 2 << 16, 0x9a);

      unit->ByteOut(unit->card, 0x134, 0xf0);
      unit->ByteOut(unit->card, 0x135, 0xf);
      unit->ByteOut(unit->card, 0x15b, 0x40);
      unit->ByteOut(unit->card, 0x184, 0x88);
      unit->ByteOut(unit->card, 0x185, 0x24);
      unit->ByteOut(unit->card, 0x188, 0x54);
      unit->ByteOut(unit->card, 0x18b, 0xb8);
      unit->ByteOut(unit->card, 0x18c, 0x7);
      unit->ByteOut(unit->card, 0x18d, 0x0);
      unit->ByteOut(unit->card, 0x194, 0x1b);
      unit->ByteOut(unit->card, 0x195, 0x12);
      unit->ByteOut(unit->card, 0x196, 0x0);
      unit->ByteOut(unit->card, 0x197, 0x6);
      unit->ByteOut(unit->card, 0x19d, 0x1a);
      unit->ByteOut(unit->card, 0x19f, 0x10);
      unit->ByteOut(unit->card, 0x1b4, 0x22);
      unit->ByteOut(unit->card, 0x1be, 0x80);
      unit->ByteOut(unit->card, 0x1db, 0x0);
      unit->ByteOut(unit->card, 0x1ee, 0x0);
      unit->ByteOut(unit->card, 0x191, 0x3);
      unit->ByteOut(unit->card, 0x14c | 2 << 16, 0x0);

      unit->ByteOut(unit->card, 0x19f | 3 << 16, 0x0);
      unit->ByteOut(unit->card, 0x18c, 0x1);
      unit->ByteOut(unit->card, 0x18d, 0x10);
      unit->ByteOut(unit->card, 0x18e, 0x8);
      unit->ByteOut(unit->card, 0x18f, 0x0);

      unit->LEWordOut(unit->card, 0x100 + R8180REG_TIDACMAP, 0xfa50);
      unit->LEWordOut(unit->card, 0x100 + R8180REG_INTMIG, 0);

      unit->LELongOut(unit->card, 0x1f0 | 1 << 16, 0);
      unit->LELongOut(unit->card, 0x1f4 | 1 << 16, 0);
      unit->ByteOut(unit->card, 0x1f8 | 1 << 16, 0);

      unit->LELongOut(unit->card, 0x100 + R8180REG_RFTIMING, 0x4001);

      unit->LEWordOut(unit->card, 0x172 | 2 << 16, 0x569a);

      unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, R8180ECMD_CONFIG);

      unit->ByteOut(unit->card, 0x100 + R8180REG_CONFIG3,
         unit->ByteIn(unit->card, 0x100 + R8180REG_CONFIG3)
         | R8180REG_CONFIG3F_ANAPARAMWRITE);
      unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, 0);

      /* Turn LED on */

      unit->ByteOut(unit->card, 0x100 + R8180REG_GPIO0, 1);
      unit->ByteOut(unit->card, 0x100 + R8180REG_GPENABLE, 0);

      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT, 0x480);
      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSSELECT, 0x2488);
      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSENABLE, 0x1fff);
      BusyMilliDelay(100, base);

      /* Initialise radio */

      success = InitialiseRadio(unit, base);

      unit->ByteOut(unit->card, 0x100 + R8180REG_COMMAND,
         R8180REG_COMMANDF_TXENABLE | R8180REG_COMMANDF_RXENABLE);
      unit->LEWordOut(unit->card, 0x100 + R8180REG_INTMASK, INT_MASK);

      unit->ByteOut(unit->card, 0x41, 0xf4);
      unit->ByteOut(unit->card, 0x40, 0);
      unit->ByteOut(unit->card, 0x42, 0);
      unit->ByteOut(unit->card, 0x42, 1);
      unit->ByteOut(unit->card, 0x40, 0xf);
      unit->ByteOut(unit->card, 0x42, 0);
      unit->ByteOut(unit->card, 0x42, 1);

      unit->ByteOut(unit->card, 0x1db,
         unit->ByteIn(unit->card, 0x1db) | 1 << 2);
      unit->LEWordOut(unit->card, 0x172 | 3 << 16, 0x59fa);
      unit->LEWordOut(unit->card, 0x174 | 3 << 16, 0x59d2);
      unit->LEWordOut(unit->card, 0x176 | 3 << 16, 0x59d2);
      unit->LEWordOut(unit->card, 0x178 | 3 << 16, 0x19fa);
      unit->LEWordOut(unit->card, 0x17a | 3 << 16, 0x19fa);
      unit->LEWordOut(unit->card, 0x17c | 3 << 16, 0xd0);
      unit->ByteOut(unit->card, 0x161, 0);
      unit->ByteOut(unit->card, 0x180 | 1 << 16, 0xf);
      unit->ByteOut(unit->card, 0x183 | 1 << 16, 3);
      unit->ByteOut(unit->card, 0x1da, 0x10);
      unit->ByteOut(unit->card, 0x14d | 2 << 16, 8);

      unit->LELongOut(unit->card, 0x100 + R8180REG_HSSIPARA, 0x600321b);

      /* Set maximum RX frame size */

      unit->LEWordOut(unit->card, 0x1ec | 1 << 16, 0x800);

      unit->ByteOut(unit->card, 0x100 + R8180REG_ACMCONTROL, 0);
      unit->ByteOut(unit->card, 0x100 + R8180REG_MSR, R8180REG_MSRF_ENEDCA);
   }

   if(success)
   {
      unit->LELongOut(unit->card, 0x100 + R8180REG_RXCONF,
         R8180REG_RXCONFF_EARLYTHRESH
         | R8180REG_RXCONFF_AUTORESETPHY
         | R8180REG_RXCONFF_CHECKBSSID
         | R8180REG_RXCONFF_MGMT
         | R8180REG_RXCONFF_DATA
         | 7 << R8180REG_RXCONFB_FIFOTHRESH
         | 7 << R8180REG_RXCONFB_MAXDMA
         | R8180REG_RXCONFF_BCAST
         | R8180REG_RXCONFF_MCAST
         | R8180REG_RXCONFF_UCAST);
      unit->ByteOut(unit->card, 0x100 + R8180REG_TXAGCCTL,
         unit->ByteIn(unit->card, 0x100 + R8180REG_TXAGCCTL)
         & ~(R8180REG_TXAGCCTLF_PPGAINSHIFT
         | R8180REG_TXAGCCTLF_PPANTSELSHIFT
         | R8180REG_TXAGCCTLF_FEEDBACKANT));
      unit->LELongOut(unit->card, 0x100 + R8180REG_TXCONF,
         R8180REG_TXCONFF_DISREQQSIZE
         | 7 << R8180REG_TXCONFB_MAXDMA
         | TX_TRIES << R8180REG_TXCONFB_SHORTTRIES
         | TX_TRIES << R8180REG_TXCONFB_LONGTRIES);
   }

   /* Determine features, and get offsets of certain fields within frame
      descriptors */

   unit->retries_offset = R8180FRM_RETRY;
   unit->tx_desc_size = 32;
   unit->rx_desc_size = 20;

   /* Set IV sizes */

   unit->iv_sizes[S2ENC_WEP] = IV_SIZE;
   unit->iv_sizes[S2ENC_TKIP] = EIV_SIZE;
   unit->iv_sizes[S2ENC_CCMP] = EIV_SIZE;

   /* Set encryption functions */

   unit->fragment_encrypt_functions[S2ENC_NONE] = WriteClearFragment;

   if((unit->flags & UNITF_HARDWEP) != 0)
      unit->fragment_encrypt_functions[S2ENC_WEP] = WriteWEPFragment;
   else
      unit->fragment_encrypt_functions[S2ENC_WEP] = EncryptWEPFragment;

   if((unit->flags & UNITF_HARDTKIP) != 0)
      unit->fragment_encrypt_functions[S2ENC_TKIP] = WriteTKIPFragment;
   else
      unit->fragment_encrypt_functions[S2ENC_TKIP] = EncryptTKIPFragment;

   if((unit->flags & UNITF_HARDCCMP) != 0)
      unit->fragment_encrypt_functions[S2ENC_CCMP] = WriteCCMPFragment;
   else
      unit->fragment_encrypt_functions[S2ENC_CCMP] = EncryptCCMPFragment;

   /* Set decryption functions */

   unit->fragment_decrypt_functions[S2ENC_NONE] = ReadClearFragment;

   if((unit->flags & UNITF_HARDWEP) != 0)
      unit->fragment_decrypt_functions[S2ENC_WEP] = ReadWEPFragment;
   else
      unit->fragment_decrypt_functions[S2ENC_WEP] = DecryptWEPFragment;

   if((unit->flags & UNITF_HARDTKIP) != 0)
      unit->fragment_decrypt_functions[S2ENC_TKIP] = ReadTKIPFragment;
   else
      unit->fragment_decrypt_functions[S2ENC_TKIP] = DecryptTKIPFragment;

   if((unit->flags & UNITF_HARDCCMP) != 0)
      unit->fragment_decrypt_functions[S2ENC_CCMP] = ReadCCMPFragment;
   else
      unit->fragment_decrypt_functions[S2ENC_CCMP] = DecryptCCMPFragment;

   /* Return */

   return success;
}



/****i* realtek8180.device/DeinitialiseAdapter *****************************
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
   /* Turn LED off */

   unit->ByteOut(unit->card, 0x100 + R8180REG_GPIO0, 1);
   unit->ByteOut(unit->card, 0x100 + R8180REG_GPENABLE, 1);

   return;
}



/****i* realtek8180.device/ConfigureAdapter ********************************
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
   UBYTE msr;
   UWORD i;

   /* Set BSSID */

   for(i = 0; i < ETH_ADDRESSSIZE; i++)
      unit->ByteOut(unit->card, 0x100 + R8180REG_BSSID + i, unit->bssid[i]);

   /* Set channel and power */

   SetPower(unit, base);

   msr = unit->ByteIn(unit->card, 0x100 + R8180REG_MSR)
      & ~R8180REG_MSRF_LINK;
   if(unit->assoc_id != 0)
   {
      msr |= 2 << R8180REG_MSRB_LINK;
      if(unit->generation >= RTL8187B0_GEN)
         msr |= R8180REG_MSRF_ENEDCA;
   }
   unit->ByteOut(unit->card, 0x100 + R8180REG_MSR, msr);

   /* Set intervals */

   unit->LEWordOut(unit->card, 0x100 + R8180REG_ATIMWINDOW, 2);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_ATIMTRINTERVAL, 100);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_BEACONINTERVAL, 100);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_BEACONINTERVAL2, 100);

   if(unit->generation >= RTL8187B0_GEN)
   {
      unit->ByteOut(unit->card, 0x100 + R8180REG_SIFS, 0x22);
      if(unit->band == S2BAND_G)
         unit->ByteOut(unit->card, 0x100 + R8180REG_SLOT, 0x9);
      else
         unit->ByteOut(unit->card, 0x100 + R8180REG_SLOT, 0x14);
      unit->ByteOut(unit->card, 0x100 + R8180REG_EIFS, 0x5b);
      unit->ByteOut(unit->card, 0x100 + R8180REG_SENSECOUNT, 0x5b);
   }

   /* Return */

   return;
}



/****i* realtek8180.device/GoOnline ****************************************
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
   unit->LEWordOut(unit->card, 0x100 + R8180REG_INTMASK, INT_MASK);

   /* Enable frame transmission and reception */

   unit->ByteOut(unit->card, 0x100 + R8180REG_COMMAND,
      R8180REG_COMMANDF_TXENABLE | R8180REG_COMMANDF_RXENABLE);

   /* Record start time and report Online event */

   GetSysTime(&unit->stats.LastStart);
   ReportEvents(unit, S2EVENT_ONLINE, base);

   return;
}



/****i* realtek8180.device/GoOffline ***************************************
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



/****i* realtek8180.device/SetOptions **************************************
*
*   NAME
*	SetOptions -- Set and use interface options.
*
*   SYNOPSIS
*	reconfigure = SetOptions(unit, tag_list)
*
*	BOOL SetOptions(struct DevUnit *, struct TagItem *);
*
****************************************************************************
*
*/

BOOL SetOptions(struct DevUnit *unit, const struct TagItem *tag_list,
   struct DevBase *base)
{
   struct TagItem *tag_item, *tlist = (struct TagItem *)tag_list;
   BOOL reconfigure = TRUE;

   while((tag_item = NextTagItem(&tlist)) != NULL)
   {
      switch(tag_item->ti_Tag)
      {
      case S2INFO_BSSID:
         CopyMem((APTR)tag_item->ti_Data, unit->bssid, ETH_ADDRESSSIZE);
         break;

      case S2INFO_AssocID:
         unit->assoc_id = tag_item->ti_Data;
         break;

      case S2INFO_Capabilities:
         unit->capabilities = tag_item->ti_Data;
         if((unit->capabilities & (1 << 5)) != 0)
            unit->flags |= UNITF_SHORTPREAMBLE;
         else
            unit->flags &= ~UNITF_SHORTPREAMBLE;
         break;

      case S2INFO_DefaultKeyNo:
         unit->tx_key_no = tag_item->ti_Data;
         break;

      case S2INFO_PortType:
         unit->mode = tag_item->ti_Data;
         break;

      case S2INFO_Channel:
         if(tag_item->ti_Data != unit->channel)
         {
            unit->channel = tag_item->ti_Data;
            reconfigure = TRUE;
         }
         break;

      case S2INFO_Band:
         unit->band = tag_item->ti_Data;
         if(unit->band == S2BAND_G)
         {
            unit->tx_rate = 54;
            unit->tx_rate_code = 11;   /* 54 Mbps */
            unit->mgmt_rate = 2;
            unit->mgmt_rate_code = 1; /* 2 Mbps */
         }
         else if(unit->band == S2BAND_B)
         {
            unit->tx_rate = 11;
            unit->tx_rate_code = 3;   /* 11 Mbps */
            unit->mgmt_rate = 1;
            unit->mgmt_rate_code = 0; /* 1 Mbps */
         }
         break;

      }
   }

   return reconfigure;
}



/****i* realtek8180.device/SetKey ******************************************
*
*   NAME
*	SetKey -- Set an encryption key.
*
*   SYNOPSIS
*	SetKey(unit, index, type, key, key_length,
*	    rx_counter)
*
*	VOID SetKey(struct DevUnit *, ULONG, ULONG, UBYTE *, ULONG,
*	    UBYTE *);
*
****************************************************************************
*
*/

VOID SetKey(struct DevUnit *unit, ULONG index, ULONG type, const UBYTE *key,
   ULONG key_length, const UBYTE *rx_counter, struct DevBase *base)
{
   struct KeyUnion *slot;
   struct EClockVal eclock;
   UWORD i;

   Disable();
   slot = &unit->keys[index];
   switch(type)
   {
      case S2ENC_WEP:
         CopyMem(key, slot->u.wep.key, key_length);
         slot->u.wep.length = key_length;

         if((unit->flags & UNITF_HARDWEP) == 0)
         {
            /* Create a reasonably random IV */

            ReadEClock(&eclock);
            slot->u.wep.tx_iv = FastRand(eclock.ev_lo ^ eclock.ev_hi);
         }

         break;

      case S2ENC_TKIP:
         CopyMem(key, slot->u.tkip.key, 16);
         CopyMem(key + 16, slot->u.tkip.tx_mic_key, MIC_SIZE);
         CopyMem(key + 24, slot->u.tkip.rx_mic_key, MIC_SIZE);
         slot->u.tkip.tx_iv_low = 0;
         slot->u.tkip.tx_iv_high = 0;
         slot->u.tkip.rx_iv_low = LEWord(*(UWORD *)rx_counter);
         slot->u.tkip.rx_iv_high = LELong(*(ULONG *)(rx_counter + 2));
         slot->u.tkip.tx_ttak_set = FALSE;
         slot->u.tkip.rx_ttak_set = FALSE;

         if((unit->flags & UNITF_HARDTKIP) != 0)
         {
         }
         else
         {
            /* Convert key to native endianness */

            for(i = 0; i < 8; i++)
               slot->u.tkip.key[i] = LEWord(slot->u.tkip.key[i]);
         }

         break;

      case S2ENC_CCMP:
         CopyMem(key, slot->u.ccmp.key, 16);
         slot->u.ccmp.tx_iv_low = 0;
         slot->u.ccmp.tx_iv_high = 0;
         slot->u.ccmp.rx_iv_low = LEWord(*(UWORD *)rx_counter);
         slot->u.ccmp.rx_iv_high = LELong(*(ULONG *)(rx_counter + 2));
         slot->u.ccmp.stream_set = FALSE;
   }

   /* Update type of key in selected slot */

   slot->type = type;
   Enable();

   return;
}



/****i* realtek8180.device/AddMulticastRange *******************************
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



/****i* realtek8180.device/RemMulticastRange *******************************
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



/****i* realtek8180.device/FindMulticastRange ******************************
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



/****i* realtek8180.device/SetMulticast ************************************
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
   return;
}



/****i* realtek8180.device/FindTypeStats ***********************************
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



/****i* realtek8180.device/FlushUnit ***************************************
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



/****i* realtek8180.device/RXInt *******************************************
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

VOID RXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code))
{
   UWORD ieee_length, frame_control, frame_type, slot, next_slot,
      frame_size, frame_subtype, encryption, key_no, buffer_no, old_length;
   struct DevBase *base;
   BOOL is_good, proceed = TRUE;
   LONG frag_no;
   ULONG status;
   UBYTE *rx_desc, *buffer, *p, *frame, *data, *snap_frame, *source;

   base = unit->device;
   slot = unit->rx_slot;
   rx_desc = unit->rx_descs[slot];
   next_slot = (slot + 1) % RX_SLOT_COUNT;

   while(proceed)
   {
      is_good = TRUE;
      buffer = unit->rx_buffers[slot];

      status = LELong(*(ULONG *)(rx_desc + R8180FRM_RXSTATUS));
      frame_size = status & R8180FRM_RXSTATUSF_LENGTH;

      if((status & (R8180FRM_RXSTATUSF_DMAERR | R8180FRM_RXSTATUSF_OVERFLOW
         | R8180FRM_RXSTATUSF_RXERR | R8180FRM_RXSTATUSF_BADCRC)) == 0
         && frame_size >= WIFI_FRM_DATA + 4)
      {
         /* Get fragment info */

         frame = buffer;
         ieee_length = frame_size - 4 - WIFI_FRM_DATA;
         data = frame + WIFI_FRM_DATA;
         frame_control =
            LEWord(*(UWORD *)(frame + WIFI_FRM_CONTROL));

         /* Get buffer to store fragment in */

         frag_no = LEWord(*(UWORD *)(frame + WIFI_FRM_SEQCONTROL));
         if(unit->mode == S2PORT_ADHOC)
            source = frame + WIFI_FRM_ADDRESS2;
         else
            source = frame + WIFI_FRM_ADDRESS3;
         snap_frame = GetRXBuffer(unit, source, frag_no, &buffer_no, base);

         /* Get location to put new data */

         if(snap_frame != NULL)
         {
            if((frag_no & 0xf ) > 0)
               old_length =
                  BEWord(*(UWORD *)(snap_frame + ETH_PACKET_IEEELEN));
            else
            {
               /* Create new 802.3 header */

               CopyMem(frame + WIFI_FRM_ADDRESS1, snap_frame,
                  ETH_ADDRESSSIZE);
               CopyMem(source, snap_frame + ETH_PACKET_SOURCE,
                  ETH_ADDRESSSIZE);
               old_length = 0;
            }
            p = snap_frame + ETH_HEADERSIZE + old_length;

            /* Append fragment to frame, decrypting fragment if necessary */

            if((frame_control & WIFI_FRM_CONTROLF_WEP) != 0)
            {
               key_no = data[3] >> 6 & 0x3;
               encryption = unit->keys[key_no].type;
            }
            else
               encryption = S2ENC_NONE;

            /* Decrypt, check and/or copy fragment */

            is_good = unit->fragment_decrypt_functions[encryption](unit,
               frame, data, &ieee_length, p, base);

            /* Update length in frame being built with current fragment, or
               increment bad frame counter if fragment is bad */

            if(is_good)
            {
               ieee_length += old_length;
               *(UWORD *)(snap_frame + ETH_PACKET_IEEELEN) =
                  MakeBEWord(ieee_length);
            }
            else
               unit->stats.BadData++;

            /* If all fragments have arrived, process the complete frame */

            if((frame_control & WIFI_FRM_CONTROLF_MOREFRAGS) == 0)
            {
               if(is_good)
               {
                  /* Decrypt complete frame if necessary */

                  data = snap_frame + ETH_HEADERSIZE;
                  if(encryption == S2ENC_TKIP
                     && (unit->flags & UNITF_HARDTKIP) == 0)
                  {
                     is_good = TKIPDecryptFrame(unit, snap_frame, data,
                        ieee_length, data, key_no, base);
                     ieee_length -= MIC_SIZE;
                     *(UWORD *)(snap_frame + ETH_PACKET_IEEELEN) =
                        MakeBEWord(ieee_length);
                     if(!is_good)
                        unit->stats.BadData++;
                  }
               }

               if(is_good)
               {
                  /* Get frame's 802.11 type and subtype */

                  frame_type = (frame_control & WIFI_FRM_CONTROLF_TYPE)
                     >> WIFI_FRM_CONTROLB_TYPE;
                  frame_subtype =
                     (frame_control & WIFI_FRM_CONTROLF_SUBTYPE)
                     >> WIFI_FRM_CONTROLB_SUBTYPE;

                  /* If it's a management frame, process it separately;
                     otherwise distribute it to clients after filtering */

                  if(frame_type == WIFI_FRMTYPE_MGMT)
                  {
                     if(frame_subtype != 8)
                        DistributeMgmtFrame(unit, frame, frame_size - 4,
                           base);
                  }
                  else if(AddressFilter(unit, snap_frame + ETH_PACKET_DEST,
                     base))
                  {
                     unit->stats.PacketsReceived++;
                     DistributeRXPacket(unit, snap_frame, base);
                  }
               }
            }

            /* Mark fragment buffer as unused for next time */

            unit->rx_fragment_nos[buffer_no] = -1;
         }
         else
            ReportEvents(unit, S2EVENT_ERROR | S2EVENT_RX, base);
      }
      else
      {
         is_good = FALSE;
      }

      if(!is_good)
      {
         ReportEvents(unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX,
            base);
      }

      /* Prepare descriptor for next time */

      unit->ReceiveFrame(unit->card, buffer,
         FRAME_BUFFER_SIZE + R8180_MAXDESCSIZE);

      /* Get next descriptor */

      slot = next_slot;
      if(unit->bus == USB_BUS)
         proceed = FALSE;
   }

   unit->rx_slot = slot;

   return;
}



/****i* realtek8180.device/GetRXBuffer *************************************
*
*   NAME
*	GetRXBuffer -- Find an appropriate RX frame buffer to use.
*
*   SYNOPSIS
*	buffer = GetRXBuffer(unit, address, frag_no)
*
*	UBYTE *GetRXBuffer(struct DevUnit *, UBYTE *, UWORD);
*
****************************************************************************
*
*/

static UBYTE *GetRXBuffer(struct DevUnit *unit, const UBYTE *address,
   UWORD frag_no, UWORD *buffer_no, struct DevBase *base)
{
   UWORD i;
   UBYTE *buffer;
   LONG n;
   BOOL found;

   buffer = unit->rx_frames;
   for(i = 0, found = FALSE; i < FRAME_BUFFER_COUNT * 2 && !found; i++)
   {
      /* Throw away old buffer contents if we didn't find a free slot the
         first time around */

      if(i >= FRAME_BUFFER_COUNT)
         unit->rx_fragment_nos[i % FRAME_BUFFER_COUNT] = -1;

      /* For a frame's first fragment, find an empty slot; for subsequent
         fragments, find a slot with matching source address */

      n = unit->rx_fragment_nos[i % FRAME_BUFFER_COUNT];
      if(n == -1 && (frag_no & 0xf) == 0
         || *((ULONG *)(buffer + ETH_PACKET_SOURCE))
         == *((ULONG *)(address))
         && *((UWORD *)(buffer + ETH_PACKET_SOURCE + 4))
         == *((UWORD *)(address + 4)))
      {
         found = TRUE;
         if(n == -1)
            unit->rx_fragment_nos[i % FRAME_BUFFER_COUNT] = frag_no;
         *buffer_no = i;
      }
      else
         buffer += FRAME_BUFFER_SIZE;
   }

   if(!found)
      buffer = NULL;

   return buffer;
}



/****i* realtek8180.device/DistributeRXPacket ******************************
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

static VOID DistributeRXPacket(struct DevUnit *unit, const UBYTE *frame,
   struct DevBase *base)
{
   UWORD packet_size, ieee_length;
   BOOL is_orphan = TRUE, accepted, is_snap = FALSE;
   ULONG packet_type;
   UBYTE *buffer;
   const UBYTE *template = snap_template;
   struct IOSana2Req *request, *request_tail;
   struct Opener *opener, *opener_tail;
   struct TypeStats *tracker;

   buffer = unit->rx_buffer;
   ieee_length = BEWord(*(UWORD *)(frame + ETH_PACKET_IEEELEN));
   packet_size = ETH_HEADERSIZE + ieee_length;
   if(ieee_length >= SNAP_HEADERSIZE)
      is_snap = *(const ULONG *)(frame + ETH_PACKET_DATA)
         == *(const ULONG *)template;

   /* De-encapsulate SNAP packets and get packet type */

   if(is_snap)
   {
      packet_size -= SNAP_HEADERSIZE;
      CopyMem(frame, buffer, ETH_PACKET_TYPE);
      CopyMem(frame + ETH_HEADERSIZE + SNAP_FRM_TYPE,
         buffer + ETH_PACKET_TYPE, packet_size - ETH_PACKET_TYPE);
   }
   packet_type = BEWord(*((UWORD *)(buffer + ETH_PACKET_TYPE)));

   /* Offer packet to every opener */

   opener = (APTR)unit->openers.mlh_Head;
   opener_tail = (APTR)&unit->openers.mlh_Tail;

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



/****i* realtek8180.device/CopyPacket **************************************
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
      packet_size += 4;   /* Needed for Shapeshifter & Fusion? */
#endif
   request->ios2_DataLength = packet_size;

   /* Filter packet */

   opener = request->ios2_BufferManagement;
   if(request->ios2_Req.io_Command == CMD_READ &&
      opener->filter_hook != NULL)
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



/****i* realtek8180.device/AddressFilter ***********************************
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



/****i* realtek8180.device/DistributeMgmtFrame *****************************
*
*   NAME
*	DistributeMgmtFrame -- Send a management frame to clients.
*
*   SYNOPSIS
*	DistributeMgmtFrame(unit, frame, frame_size)
*
*	VOID DistributeMgmtFrame(struct DevUnit *, UBYTE *, UWORD);
*
****************************************************************************
*
*/

static VOID DistributeMgmtFrame(struct DevUnit *unit, UBYTE *frame,
   UWORD frame_size, struct DevBase *base)
{
   struct IOSana2Req *request;
   struct Opener *opener, *opener_tail;

   /* Send packet to every opener */

   opener = (APTR)unit->openers.mlh_Head;
   opener_tail = (APTR)&unit->openers.mlh_Tail;

   while(opener != opener_tail)
   {
      request = (APTR)RemHead(&opener->mgmt_port.mp_MsgList);

      if(request != NULL)
      {
         /* Copy packet into opener's buffer and reply packet */

         if(frame_size <= request->ios2_DataLength)
         {
            CopyMem(frame, request->ios2_Data, frame_size);
            request->ios2_DataLength = frame_size;
         }
         else
         {
            request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
            request->ios2_WireError = S2WERR_BUFF_ERROR;
            ReportEvents(unit,
               S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_RX,
               base);
         }
         ReplyMsg((APTR)request);
         request =
            (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;
      }

      opener = (APTR)opener->node.mln_Succ;
   }

   return;
}



/****i* realtek8180.device/TXInt *******************************************
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

static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code))
{
   struct DevBase *base;
   UWORD i, frame_size, data_size, packet_type, body_size, slot, new_slot,
      encryption, subtype, duration;
   UBYTE *buffer, *q, *plaintext, *ciphertext, *frame,
      mic_header[ETH_ADDRESSSIZE * 2];
   const UBYTE *p, *dest, *source;
   struct IOSana2Req *request;
   BOOL proceed = TRUE, is_ieee, has_bssid;
   struct Opener *opener;
   ULONG wire_error, control_value;
   UBYTE *tx_desc;
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   BYTE error;
   struct MsgPort *port;
   struct TypeStats *tracker;

   base = unit->device;
   port = unit->request_ports[WRITE_QUEUE];

   while(proceed && (!IsMsgPortEmpty(port)))
   {
      slot = unit->tx_in_slot;
      new_slot = (slot + 1) % TX_SLOT_COUNT;

      if(new_slot != unit->tx_out_slot)
      {
         error = 0;
         body_size = 0;

         /* Get request and DMA frame descriptor */

         request = (APTR)port->mp_MsgList.lh_Head;

         Remove((APTR)request);
         unit->tx_requests[slot] = request;
         tx_desc = unit->tx_descs[slot];
         frame = unit->tx_buffers[slot];

         /* Get packet data */

         opener = request->ios2_BufferManagement;
         dma_tx_function = opener->dma_tx_function;
         if(dma_tx_function != NULL)
            buffer = dma_tx_function(request->ios2_Data);
         else
            buffer = NULL;

         if(buffer == NULL)
         {
            buffer = unit->tx_buffer;
            if(!opener->tx_function(buffer, request->ios2_Data,
               request->ios2_DataLength))
            {
               error = S2ERR_NO_RESOURCES;
               wire_error = S2WERR_BUFF_ERROR;
               ReportEvents(unit,
                  S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF
                  | S2EVENT_TX, base);
            }
         }

         if(error == 0)
         {
            /* Get packet type and/or length */

            data_size = request->ios2_DataLength;
            if((request->ios2_Req.io_Flags & SANA2IOF_RAW) != 0)
            {
               data_size -= ETH_PACKET_DATA;
               packet_type = BEWord(*(UWORD *)(buffer + ETH_PACKET_TYPE));
            }
            else
               packet_type = request->ios2_PacketType;
            is_ieee = packet_type <= ETH_MTU;

            /* Determine encryption type and frame subtype */

            if(data_size > 0)
            {
               encryption = unit->keys[unit->tx_key_no].type;
               subtype = 0;
            }
            else
            {
               encryption = S2ENC_NONE;
               subtype = 4;
            }

            /* Get source and destination addresses */

            if((request->ios2_Req.io_Flags & SANA2IOF_RAW) != 0)
            {
               dest = buffer;
               source = buffer + ETH_ADDRESSSIZE;
               buffer += ETH_ADDRESSSIZE * 2 + 2;
            }
            else
            {
               dest = request->ios2_DstAddr;
               source = unit->address;
            }

            /* Write 802.11 header */

            q = frame;
            *(UWORD *)q = MakeLEWord(
               (encryption == S2ENC_NONE ? 0 : WIFI_FRM_CONTROLF_WEP)
               | (unit->mode == S2PORT_ADHOC ? 0 : WIFI_FRM_CONTROLF_TODS)
               | subtype << WIFI_FRM_CONTROLB_SUBTYPE
               | WIFI_FRMTYPE_DATA << WIFI_FRM_CONTROLB_TYPE);
            q += 4;

            if(unit->mode == S2PORT_ADHOC)
               p = dest;
            else
               p = unit->bssid;
            for(i = 0; i < ETH_ADDRESSSIZE; i++)
               *q++ = *p++;

            for(i = 0, p = source; i < ETH_ADDRESSSIZE; i++)
               *q++ = *p++;

            if(unit->mode == S2PORT_ADHOC)
               p = unit->bssid;
            else
               p = dest;
            for(i = 0; i < ETH_ADDRESSSIZE; i++)
               *q++ = *p++;
            *(UWORD *)q = MakeLEWord(unit->tx_sequence);
            unit->tx_sequence += 0x10;
            q += 2;

            /* Leave room for encryption overhead */

            ciphertext = q;
            q += unit->iv_sizes[encryption];
            plaintext = q;

            /* Write SNAP header */

            if(!is_ieee)
            {
               for(i = 0, p = snap_template;
                  i < SNAP_FRM_TYPE; i++)
                  *q++ = *p++;
               *(UWORD *)q = MakeBEWord(packet_type);
               q += 2;
               body_size += SNAP_HEADERSIZE;
            }

            /* Copy data into frame */

            CopyMem(buffer, q, data_size);
            body_size += data_size;

            /* Append MIC to frame for TKIP */

            if(encryption == S2ENC_TKIP)
            {
               q = mic_header;
               for(i = 0, p = dest; i < ETH_ADDRESSSIZE; i++)
                  *q++ = *p++;
               for(i = 0, p = source; i < ETH_ADDRESSSIZE; i++)
                  *q++ = *p++;
               TKIPEncryptFrame(unit, mic_header, plaintext, body_size,
                  plaintext, base);
               body_size += MIC_SIZE;
            }

            /* Encrypt fragment if applicable */

            unit->fragment_encrypt_functions[encryption](unit, frame,
               plaintext, &body_size, ciphertext, base);

            /* Clear frame descriptor as far as start of 802.11 header */

            q = tx_desc;
            for(i = 0; i < unit->tx_desc_size; i++)
               *q++ = 0;

            /* Set TX control field */

            frame_size = WIFI_FRM_DATA + body_size;
            control_value = R8180FRM_TXCONTROLF_NOENC
               | R8180FRM_TXCONTROLF_FIRSTFRAG
               | R8180FRM_TXCONTROLF_LASTFRAG
               | unit->tx_rate_code << R8180FRM_TXCONTROLB_RATE
               | frame_size;
            *(ULONG *)(tx_desc + R8180FRM_TXCONTROL) =
               MakeLELong(control_value);

            /* Set durations */

            has_bssid = ((frame + WIFI_FRM_ADDRESS1)[0] & 0x1) == 0;
            if(has_bssid)
               duration = SIFS_TIME + GetDuration(unit, 14,
                  AckRate(unit, unit->tx_rate, base), FALSE, base);
            else
               duration = 0;
            *(UWORD *)(frame + WIFI_FRM_DURATION) = MakeLEWord(duration);

            if(unit->generation >= RTL8187B0_GEN)
            {
               duration += GetDuration(unit, frame_size + 4,
                  unit->tx_rate,
                  (unit->flags & UNITF_SHORTPREAMBLE) != 0 && has_bssid,
                  base);
               *(UWORD *)(tx_desc + R8180FRM_TXDUR) = MakeLEWord(duration);
            }

            /* Set max number of retries */

            *(ULONG *)(tx_desc + unit->retries_offset) =
               MakeLELong((TX_TRIES - 1) << 8);

            /* Pass packet to adapter */

            unit->SendFrame(unit->card, tx_desc,
               R8180_MAXDESCSIZE + frame_size);
            unit->tx_in_slot = new_slot;
         }
         else
         {
            /* Reply failed request */

            request->ios2_Req.io_Error = error;
            request->ios2_WireError = wire_error;
            ReplyMsg((APTR)request);
         }

         /* Update statistics */

         if(error == 0)
         {
            unit->stats.PacketsSent++;

            tracker = FindTypeStats(unit, &unit->type_trackers,
               request->ios2_PacketType, base);
            if(tracker != NULL)
            {
               tracker->stats.PacketsSent++;
               tracker->stats.BytesSent += ETH_HEADERSIZE + data_size;
            }
         }
      }
      else
         proceed = FALSE;
   }

   /* Don't try to keep sending packets if there's no space left */

   if(proceed)
   {
      unit->request_ports[MGMT_QUEUE]->mp_Flags = PA_SOFTINT;
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
   }
   else
   {
      unit->request_ports[MGMT_QUEUE]->mp_Flags = PA_IGNORE;
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;
   }

   return;
}



/****i* realtek8180.device/MgmtTXInt ***************************************
*
*   NAME
*	MgmtTXInt -- Soft interrupt for management frame transmission.
*
*   SYNOPSIS
*	MgmtTXInt(unit)
*
*	VOID MgmtTXInt(struct DevUnit *);
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

static VOID MgmtTXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code))
{
   struct DevBase *base;
   UWORD frame_size, slot, new_slot, i, duration;
   UBYTE *desc, *frame, *q;
   struct IOSana2Req *request;
   BOOL proceed = TRUE, has_bssid;
   ULONG control_value;
   struct MsgPort *port;

   base = unit->device;
   port = unit->request_ports[MGMT_QUEUE];

   while(proceed && (!IsMsgPortEmpty(port)))
   {
      slot = unit->tx_in_slot;
      new_slot = (slot + 1) % TX_SLOT_COUNT;

      if(new_slot != unit->tx_out_slot)
      {
         /* Get request and frame descriptor */

         request = (APTR)port->mp_MsgList.lh_Head;

         Remove((APTR)request);
         unit->tx_requests[slot] = request;
         desc = unit->tx_descs[slot];
         frame = unit->tx_buffers[slot];

         /* Get packet length */

         frame_size = request->ios2_DataLength;

         /* Copy frame into transmit buffer */

         CopyMem(request->ios2_Data, frame, frame_size);

         /* Clear frame descriptor as far as start of 802.11 header */

         q = desc;
         for(i = 0; i < unit->tx_desc_size; i++)
            *q++ = 0;

         /* Set TX control field */

         control_value = R8180FRM_TXCONTROLF_NOENC
            | R8180FRM_TXCONTROLF_FIRSTFRAG
            | R8180FRM_TXCONTROLF_LASTFRAG
            | unit->mgmt_rate_code << R8180FRM_TXCONTROLB_RATE
            | frame_size;
         *(ULONG *)(desc + R8180FRM_TXCONTROL) = MakeLELong(control_value);

         /* Set durations */

         has_bssid = ((frame + WIFI_FRM_ADDRESS1)[0] & 0x1) == 0;
         if(has_bssid)
            duration = SIFS_TIME + GetDuration(unit, 14,
               AckRate(unit, unit->mgmt_rate, base), FALSE, base);
         else
            duration = 0;
         *(UWORD *)(frame + WIFI_FRM_DURATION) = MakeLEWord(duration);

         if(unit->generation >= RTL8187B0_GEN)
         {
            duration += GetDuration(unit, frame_size + 4,
               unit->mgmt_rate, FALSE, base);
            *(UWORD *)(desc + R8180FRM_TXDUR) = MakeLEWord(duration);
         }

         /* Set max number of retries */

         *(ULONG *)(desc + unit->retries_offset) =
            MakeLELong((TX_TRIES - 1) << 8);

         /* Set sequence number */

         *(UWORD *)(frame + WIFI_FRM_SEQCONTROL) =
            MakeLEWord(unit->tx_sequence);
         unit->tx_sequence += 0x10;

         /* Pass packet to adapter */

         unit->SendFrame(unit->card, desc, R8180_MAXDESCSIZE + frame_size);
         unit->tx_in_slot = new_slot;
      }
      else
         proceed = FALSE;
   }

   /* Don't try to keep sending packets if there's no space left */

   if(proceed)
   {
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
      unit->request_ports[MGMT_QUEUE]->mp_Flags = PA_SOFTINT;
   }
   else
   {
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;
      unit->request_ports[MGMT_QUEUE]->mp_Flags = PA_IGNORE;
   }

   return;
}



/****i* realtek8180.device/RetireTXSlot ************************************
*
*   NAME
*	RetireTXSlot -- Clean up after a frame has been sent.
*
*   SYNOPSIS
*	RetireTXSlot(unit)
*
*	VOID RetireTXSlot(struct DevUnit *);
*
****************************************************************************
*
*/

VOID RetireTXSlot(struct DevUnit *unit, struct DevBase *base)
{
   UWORD frame_size, slot;
   struct IOSana2Req *request;
   struct TypeStats *tracker;

   /* Update statistics */

   slot = unit->tx_out_slot;
   request = unit->tx_requests[slot];
   if(request->ios2_Req.io_Command != S2_WRITEMGMT)
   {
      frame_size = request->ios2_DataLength;
      if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
         frame_size += ETH_HEADERSIZE;

      tracker = FindTypeStats(unit, &unit->type_trackers,
         request->ios2_PacketType, base);
      if(tracker != NULL)
      {
         tracker->stats.PacketsSent++;
         tracker->stats.BytesSent += frame_size;
      }
   }

   /* Reply request */

   request->ios2_Req.io_Error = 0;
   ReplyMsg((APTR)request);

   unit->tx_out_slot = (slot + 1) % TX_SLOT_COUNT;

   /* Restart downloads if they had stopped */

   if(unit->request_ports[WRITE_QUEUE]->mp_Flags == PA_IGNORE)
      Cause(&unit->tx_int);
   if(unit->request_ports[MGMT_QUEUE]->mp_Flags == PA_IGNORE)
      Cause(&unit->mgmt_int);

   return;
}



/****i* realtek8180.device/ReportEvents ************************************
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



/****i* realtek8180.device/GetDuration *************************************
*
*   NAME
*	GetDuration -- Calculate a duration value.
*
*   SYNOPSIS
*	GetDuration(unit, length, rate, is_mgmt)
*
*	VOID GetDuration(struct DevUnit *);
*
*   FUNCTION
*	Calculates a duration for a frame of given length when transmitted
*	at a given rate. If this is a transmiss
*
*   INPUTS
*	unit - A unit of this device.
*	length - Length of frame whose duration is to be calculated.
*	rate - Rate frame will be transmitted at (in Mbps, rounded down).
*	is_short - Indicates if frame has a short preamble.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

static UWORD GetDuration(struct DevUnit *unit, UWORD length, UWORD rate,
   BOOL short_preamble, struct DevBase *base)
{
   UWORD duration, cycles;

   if (rate % 3 != 0)
   {
      duration = 97 + (length * 8 - 1) / rate;
      if (!short_preamble || rate == 1)
         duration += 96;
   }
   else
   {
      cycles = rate * 4;
      duration = ((length + 29) / cycles + 1) * 4 + 26;
   }

   return duration;
}



/****i* realtek8180.device/AckRate *****************************************
*
*   NAME
*	AckRate -- Get the ACK rate corresponding to a data rate.
*
*   SYNOPSIS
*	ack_rate = AckRate(unit, data_rate)
*
*	UWORD AckRate(struct DevUnit *, UWORD);
*
*   FUNCTION
*	Calculates the rate at which the ACK frame for a data frame with the
*	given rate should be transmitted.
*
*   INPUTS
*	unit - A unit of this device.
*	rate - Rate data frame is transmitted at (in Mbps, rounded down).
*
*   RESULT
*	ack_rate - The rate for the ACK frame (Mbps, rounded down).
*
****************************************************************************
*
*/

static UWORD AckRate(struct DevUnit *unit, UWORD data_rate,
   struct DevBase *base)
{
   UWORD ack_rate;

   switch(data_rate)
   {
   case 54:
   case 48:
   case 36:
   case 24:
      ack_rate = 24;
      break;
   case 18:
   case 12:
      ack_rate = 12;
      break;
   case 9:
   case 6:
      ack_rate = 6;
      break;
   case 11:
   case 5:
   case 2:
      ack_rate = 2;
      break;
   case 1:
      ack_rate = 1;
      break;
   }

   return ack_rate;
}



/****i* realtek8180.device/UnitTask ****************************************
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



