/*

File: unit.c
Author: Neil Cafferkey
Copyright (C) 2001-2006 Neil Cafferkey

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
#include <proto/dos.h>
#include <proto/timer.h>

#include "device.h"
#include "prism2.h"

#include "unit_protos.h"
#include "request_protos.h"


#define TASK_PRIORITY 0
#define STACK_SIZE 4096
#define INT_MASK \
   (P2_EVENTF_INFO | P2_EVENTF_ALLOCMEM | P2_EVENTF_TXFAIL | P2_EVENTF_RX)
#define MAX_S_REC_SIZE 50
#define LUCENT_DBM_OFFSET   149
#define INTERSIL_DBM_OFFSET 100


#ifndef AbsExecBase
#define AbsExecBase (*(struct ExecBase **)4)
#endif

static VOID GetDefaults(struct DevUnit *unit, struct DevBase *base);
static struct AddressRange *FindMulticastRange(struct DevUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
   UWORD upper_bound_right, struct DevBase *base);
static AROS_UFIP(RXInt);
static VOID CopyPacket(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD packet_size, UWORD packet_type, UBYTE *buffer, BOOL all_read,
   UWORD frame_id, struct DevBase *base);
static BOOL AddressFilter(struct DevUnit *unit, UBYTE *address,
   struct DevBase *base);
static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base);
static AROS_UFIP(TXInt);
static AROS_UFIP(InfoInt);
static VOID ReportEvents(struct DevUnit *unit, ULONG events,
   struct DevBase *base);
static BOOL LoadFirmware(struct DevUnit *unit, struct DevBase *base);
static VOID P2DoCmd(struct DevUnit *unit, UWORD command, UWORD param,
   struct DevBase *base);
static BOOL P2Seek(struct DevUnit *unit, UWORD path_no, UWORD rec_no,
   UWORD offset, struct DevBase *base);
static VOID P2SetID(struct DevUnit *unit, UWORD rec_no, const UBYTE *id,
   UWORD length, struct DevBase *base);
static VOID P2SetWord(struct DevUnit *unit, UWORD rec_no, UWORD value,
   struct DevBase *base);
static UWORD P2GetWord(struct DevUnit *unit, UWORD rec_no,
   struct DevBase *base);
static UWORD P2AllocMem(struct DevUnit *unit, UWORD size,
   struct DevBase *base);
static VOID P2SetData(struct DevUnit *unit, UWORD rec_no, const UBYTE *data,
   UWORD length, struct DevBase *base);
static VOID UnitTask();
static VOID BusyMilliDelay(ULONG millis, struct DevBase *base);
static UPINT StrLen(const TEXT *s);


static const UBYTE snap_stuff[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
static const TEXT options_name[] = "Prism 2 options";
static const TEXT firmware_file_name[] = "DEVS:Firmware/HermesII";


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



/****i* prism2.device/CreateUnit *******************************************
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
   ULONG size;

   unit = AllocMem(sizeof(struct DevUnit), MEMF_CLEAR | MEMF_PUBLIC);
   if(unit == NULL)
      success = FALSE;

   if(success)
   {
      unit->index = index;
      unit->device = base;
      unit->card = card;
      unit->bus = bus;
      unit->WordsIn =
         (APTR)GetTagData(IOTAG_WordsIn, (UPINT)NULL, io_tags);
      unit->WordsOut =
         (APTR)GetTagData(IOTAG_WordsOut, (UPINT)NULL, io_tags);
      unit->BEWordOut =
         (APTR)GetTagData(IOTAG_BEWordOut, (UPINT)NULL, io_tags);
      unit->LEWordIn =
         (APTR)GetTagData(IOTAG_LEWordIn, (UPINT)NULL, io_tags);
      unit->LEWordOut =
         (APTR)GetTagData(IOTAG_LEWordOut, (UPINT)NULL, io_tags);
      if(unit->WordsIn == NULL || unit->WordsOut == NULL
         || unit->BEWordOut == NULL || unit->LEWordIn == NULL
         || unit->LEWordOut == NULL)
         success = FALSE;
   }

   if(success)
   {
      InitSemaphore(&unit->access_lock);
      success = InitialiseAdapter(unit, FALSE, base);
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

      size = (ETH_MAXPACKETSIZE + 3) & ~3;
      unit->rx_buffer = AllocVec(size, MEMF_PUBLIC);
      unit->tx_buffer = AllocVec(ETH_MAXPACKETSIZE, MEMF_PUBLIC);

      if(unit->rx_buffer == NULL || unit->tx_buffer == NULL)
         success = FALSE;
   }

   if(success)
   {
      NewList((APTR)&unit->openers);
      NewList((APTR)&unit->type_trackers);
      NewList((APTR)&unit->multicast_ranges);

      /* Record maximum speed in BPS */

      unit->speed = 11000000;

      /* Initialise status, transmit, receive and stats interrupts */

      unit->status_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
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

      unit->info_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->info_int.is_Code = (APTR)InfoInt;
      unit->info_int.is_Data = unit;

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

      /* Get default wireless options */

      GetDefaults(unit, base);
   }

   if(!success)
   {
      DeleteUnit(unit, base);
      unit = NULL;
   }

   return unit;
}



/****i* prism2.device/DeleteUnit *******************************************
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

      FreeVec(unit->tx_buffer);
      FreeVec(unit->rx_buffer);

      FreeMem(unit, sizeof(struct DevUnit));
   }

   return;
}



/****i* prism2.device/InitialiseAdapter ************************************
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
   UWORD vendor, version, revision, i;
   BOOL success = TRUE;
   UBYTE address[ETH_ADDRESSSIZE];
   WORD length;

   /* Reset card */

   if(unit->bus == PCI_BUS)
   {
      unit->LEWordOut(unit->card, P2_REG_PCICOR, 0x80);
      BusyMilliDelay(250, base);
      unit->LEWordOut(unit->card, P2_REG_PCICOR, 0);
      BusyMilliDelay(500, base);
      while((unit->LEWordIn(unit->card, P2_REG_COMMAND)
         & P2_REG_COMMANDF_BUSY) != 0);
   }

   P2DoCmd(unit, P2_CMD_INIT, 0, base);

   /* Determine firmware type and download firmware if necessary */

   P2DoCmd(unit, P2_CMD_ACCESS, P2_REC_PRIIDENTITY, base);
   P2Seek(unit, 1, P2_REC_PRIIDENTITY, 0, base);
   length = unit->LEWordIn(unit->card, P2_REG_DATA1);
   if(length != 5)
   {
      unit->firmware_type = HERMES2_FIRMWARE;
      success = LoadFirmware(unit, base);
   }
   else
   {
      P2DoCmd(unit, P2_CMD_ACCESS, P2_REC_STAIDENTITY, base);

      P2Seek(unit, 1, P2_REC_STAIDENTITY, 4, base);
      unit->LEWordIn(unit->card, P2_REG_DATA1);
      vendor = unit->LEWordIn(unit->card, P2_REG_DATA1);
      version = unit->LEWordIn(unit->card, P2_REG_DATA1);
      revision = unit->LEWordIn(unit->card, P2_REG_DATA1);

      if(vendor == 1)
         unit->firmware_type = LUCENT_FIRMWARE;
      else if(vendor == 2 && (version == 1 || version == 2)
         && revision == 1)
         unit->firmware_type = SYMBOL_FIRMWARE;
      else
         unit->firmware_type = INTERSIL_FIRMWARE;
   }

   if(success)
   {
      /* Get card capabilities and default values */

      if(P2GetWord(unit, P2_REC_HASWEP, base) != 0)
         unit->flags |= UNITF_HASWEP;
      unit->channel = P2GetWord(unit, P2_REC_OWNCHNL, base);

      /* Get default MAC address */

      P2DoCmd(unit, P2_CMD_ACCESS, P2_REC_ADDRESS, base);

      P2Seek(unit, 1, P2_REC_ADDRESS, 4, base);
      unit->WordsIn(unit->card, P2_REG_DATA1, (UWORD *)address,
         ETH_ADDRESSSIZE / 2);

      /* If card has been re-inserted, check it has the same address as
         before */

      if(reinsertion)
      {
         for(i = 0; i < ETH_ADDRESSSIZE; i++)
            if(address[i] != unit->default_address[i])
               success = FALSE;
      }
   }

   if(success)
   {
      CopyMem(address, unit->default_address, ETH_ADDRESSSIZE);

      /* Get initial on-card TX buffer */

      unit->tx_frame_id = P2AllocMem(unit,
         P2_H2FRM_ETHFRAME + ETH_SNAPHEADERSIZE + ETH_MTU, base);
   }

   /* Return */

   return success;
}



/****i* prism2.device/ConfigureAdapter *************************************
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
   UWORD i, key_length, port_type;
   const struct WEPKey *keys;

   /* Set MAC address */

   P2SetData(unit, P2_REC_ADDRESS, unit->address, ETH_ADDRESSSIZE,
      base);

   /* Decide on promiscuous mode */

   P2SetWord(unit, P2_REC_PROMISC, FALSE, base);
   SetMulticast(unit, base);

   /* Set wireless parameters */

   if(unit->mode == S2PORT_ADHOC)
      P2SetWord(unit, P2_REC_OWNCHNL, unit->channel, base);
   if(unit->mode == S2PORT_MANAGED)
      port_type = 1;
   else
      port_type = 0;
   P2SetWord(unit, P2_REC_PORTTYPE, port_type, base);
   P2SetWord(unit, P2_REC_CREATEIBSS, unit->mode == S2PORT_ADHOC, base);

   P2SetID(unit, P2_REC_DESIREDSSID, unit->ssid, unit->ssid_length,
      base);

   /* Transmit at 11Mbps, with fallback */

   if(unit->firmware_type == INTERSIL_FIRMWARE
      || unit->firmware_type == SYMBOL_FIRMWARE)
      P2SetWord(unit, P2_REC_TXRATE, 0xf, base);

   /* Configure encryption */

   if(unit->encryption == S2ENC_WEP)
   {
      if(unit->firmware_type == LUCENT_FIRMWARE
         || unit->firmware_type == HERMES2_FIRMWARE)
      {
         P2SetWord(unit, P2_REC_ALTENCRYPTION, TRUE, base);
         P2SetWord(unit, P2_REC_ALTTXCRYPTKEY, 0, base);
         P2Seek(unit, 1, P2_REC_DEFLTCRYPTKEYS, 0, base);
         unit->LEWordOut(unit->card, P2_REG_DATA1, P2_ALTWEPRECLEN);
         unit->LEWordOut(unit->card, P2_REG_DATA1, P2_REC_DEFLTCRYPTKEYS);
         keys = unit->keys;
         for(i = 0; i < IEEE802_11_WEPKEYCOUNT; i++)
         {
            key_length = keys[i].length;
            if(key_length == 0)
               key_length = IEEE802_11_WEP64LEN;
            unit->LEWordOut(unit->card, P2_REG_DATA1, key_length);
            unit->WordsOut(unit->card, P2_REG_DATA1, (UWORD *)keys[i].key,
               (key_length + 1) / 2);
         }

         P2DoCmd(unit, P2_CMD_ACCESS | P2_CMDF_WRITE,
            P2_REC_DEFLTCRYPTKEYS, base);
      }
      else
      {
         P2SetWord(unit, P2_REC_ENCRYPTION,
            P2_REC_ENCRYPTIONF_NOPLAINTEXT | P2_REC_ENCRYPTIONF_ENABLE,
            base);

         P2SetWord(unit, P2_REC_TXCRYPTKEY, 0, base);

         keys = unit->keys;
         for(i = 0; i < IEEE802_11_WEPKEYCOUNT; i++)
         {
            key_length = keys[i].length;
            if(key_length == 0)
               key_length = keys[unit->key_no].length;
            P2SetData(unit, P2_REC_CRYPTKEY0 + i, keys[i].key,
               key_length, base);
         }
      }
   }
   else
   {
      if(unit->firmware_type == LUCENT_FIRMWARE
         || unit->firmware_type == HERMES2_FIRMWARE)
         P2SetWord(unit, P2_REC_ALTENCRYPTION, FALSE, base);
      else
         P2SetWord(unit, P2_REC_ENCRYPTION,
            P2_REC_ENCRYPTIONF_HOSTDECRYPT | P2_REC_ENCRYPTIONF_HOSTENCRYPT,
            base);
   }

   /* Return */

   return;
}



/****i* prism2.device/GoOnline *********************************************
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
   unit->LEWordOut(unit->card, P2_REG_INTMASK, INT_MASK);

   /* Enable the transceiver */

   P2DoCmd(unit, P2_CMD_ENABLE, 0, base);

   /* Record start time and report Online event */

   GetSysTime(&unit->stats.LastStart);
   ReportEvents(unit, S2EVENT_ONLINE, base);

   return;
}



/****i* prism2.device/GoOffline ********************************************
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
   if((unit->flags & UNITF_HAVEADAPTER) != 0)
   {
      /* Stop interrupts */

      unit->LEWordOut(unit->card, P2_REG_INTMASK, 0);

      /* Update statistics */

      UpdateStats(unit, base);

      /* Stop transmission and reception */

      P2DoCmd(unit, P2_CMD_DISABLE, 0, base);
   }

   /* Flush pending read and write requests */

   FlushUnit(unit, WRITE_QUEUE, S2ERR_OUTOFSERVICE, base);

   /* Report Offline event and return */

   ReportEvents(unit, S2EVENT_OFFLINE, base);
   return;
}



/****i* prism2.device/GetDefaults *******************************************
*
*   NAME
*	GetDefaults
*
*   SYNOPSIS
*	GetDefaults(unit)
*
*	VOID GetDefaults(struct DevUnit *);
*
*   NOTES
*	This function only exists for use in conjunction with the
*	SetPrism2Defaults utility.
*
****************************************************************************
*
*/

static VOID GetDefaults(struct DevUnit *unit, struct DevBase *base)
{
   const struct NamedObject *options;
   struct TagItem *tag_list, *tag_item;
   const TEXT *id;
   UPINT length;
   UWORD encryption;
   const struct WEPKey *key;

   options = FindNamedObject(NULL, options_name, NULL);
   if(options != NULL)
   {
      tag_list = (APTR)options->no_Object;

      while((tag_item = NextTagItem(&tag_list)) != NULL)
      {
         switch(tag_item->ti_Tag)
         {
         case P2OPT_SSID:
            id = (const TEXT *)tag_item->ti_Data;
            length = StrLen(id);
            CopyMem(id, unit->ssid, length);
            unit->ssid_length = length;
            break;

         case P2OPT_Encryption:
            encryption = tag_item->ti_Data;
            if((unit->flags & UNITF_HASWEP) != 0)
               unit->encryption = encryption;
            break;

         case P2OPT_WEPKey:
            key = (APTR)tag_item->ti_Data;
            if((unit->flags & UNITF_HASWEP) != 0)
            {
               unit->keys[0].length = key->length;
               CopyMem(key->key, &unit->keys[0].key, key->length);
            }
            break;

         case P2OPT_PortType:
            unit->mode = tag_item->ti_Data;
            break;

         case P2OPT_Channel:
            if(tag_item->ti_Data != 0)
               unit->channel = tag_item->ti_Data;
            break;
         }
      }
   }

   return;
}



/****i* prism2.device/AddMulticastRange ************************************
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



/****i* prism2.device/RemMulticastRange ************************************
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



/****i* prism2.device/FindMulticastRange ***********************************
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



/****i* prism2.device/SetMulticast *****************************************
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
   ULONG address_left;
   UWORD address_right, i = 0;
   struct AddressRange *range, *tail;
   BOOL range_ended;

   /* Fill in multicast list */

   P2Seek(unit, 1, P2_REC_MCASTLIST, 4, base);

   range = (APTR)unit->multicast_ranges.mlh_Head;
   tail = (APTR)&unit->multicast_ranges.mlh_Tail;

   while(range != tail && i < P2_MAXMCASTENTRIES)
   {
      address_left = range->lower_bound_left;
      address_right = range->lower_bound_right;
      range_ended = FALSE;

      while(!range_ended && i++ < P2_MAXMCASTENTRIES)
      {
         unit->BEWordOut(unit->card, P2_REG_DATA1,
            (UWORD)(address_left >> 16));
         unit->BEWordOut(unit->card, P2_REG_DATA1, (UWORD)address_left);
         unit->BEWordOut(unit->card, P2_REG_DATA1, (UWORD)address_right);

         if(address_left == range->upper_bound_left &&
            address_right == range->upper_bound_right)
            range_ended = TRUE;
         if(++address_right == 0)
            address_left++;
      }

      if(range_ended)
         range = (APTR)range->node.mln_Succ;
   }

   /* Turn promiscuous mode on or off depending on the previous state and
      whether we've overflowed the multicast list */

   if((unit->flags & UNITF_PROM) == 0)
   {
      if(range != tail)
      {
         if((unit->flags & UNITF_ALLMCAST) == 0)
         {
            P2SetWord(unit, P2_REC_PROMISC, TRUE, base);
            unit->flags |= UNITF_ALLMCAST;
         }
      }
      else
      {
         if((unit->flags & UNITF_ALLMCAST) != 0)
         {
            P2SetWord(unit, P2_REC_PROMISC, FALSE, base);
            unit->flags &= ~UNITF_ALLMCAST;
         }

         /* Only commit multicast list if promiscuity is off */

         P2Seek(unit, 1, P2_REC_MCASTLIST, 0, base);
         unit->LEWordOut(unit->card, P2_REG_DATA1,
            1 + ETH_ADDRESSSIZE / 2 * i);
         unit->LEWordOut(unit->card, P2_REG_DATA1, P2_REC_MCASTLIST);
         P2DoCmd(unit, P2_CMD_ACCESS | P2_CMDF_WRITE, P2_REC_MCASTLIST,
            base);
      }
   }

   return;
}



/****i* prism2.device/FindTypeStats ****************************************
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



/****i* prism2.device/FlushUnit ********************************************
*
*   NAME
*	FlushUnit
*
*   SYNOPSIS
*	FlushUnit(unit)
*
*	VOID FlushUnit(struct DevUnit *);
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



/****i* prism2.device/CopyPacket *******************************************
*
*   NAME
*	CopyPacket -- Copy packet to client's buffer.
*
*   SYNOPSIS
*	CopyPacket(unit, request, packet_size, packet_type,
*	    buffer, all_read, frame_id)
*
*	VOID CopyPacket(struct DevUnit *, struct IOSana2Req *, UWORD, UWORD,
*	    UBYTE *, BOOL, UWORD);
*
****************************************************************************
*
*/

static VOID CopyPacket(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD packet_size, UWORD packet_type, UBYTE *buffer, BOOL all_read,
   UWORD frame_id, struct DevBase *base)
{
   struct Opener *opener;
   BOOL filtered = FALSE;
   UWORD size, *p;

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

   /* Read rest of packet and de-encapsulate SNAP frames */

   if(!all_read)
   {
      if(packet_type > ETH_MTU)
      {
         p = (UWORD *)(buffer + ETH_PACKET_TYPE);
         size = packet_size - ETH_HEADERSIZE;
         *p++ = MakeBEWord(packet_type);
      }
      else
      {
         p = (UWORD *)(buffer + ETH_SNAPHEADERSIZE);
         size = packet_size - ETH_SNAPHEADERSIZE;
      }
      unit->WordsIn(unit->card, P2_REG_DATA1, p, (size + 1) / 2);
   }

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



/****i* prism2.device/AddressFilter ****************************************
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



/****i* prism2.device/UpdateStats ******************************************
*
*   NAME
*	UpdateStats
*
*   SYNOPSIS
*	UpdateStats(unit)
*
*	VOID UpdateStats(struct DevUnit *);
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

VOID UpdateStats(struct DevUnit *unit, struct DevBase *base)
{
   /* Ask for and wait for stats */

   if((unit->flags & UNITF_ONLINE) != 0)
   {
      Disable();
      unit->LEWordOut(unit->card, P2_REG_INTMASK,
         unit->LEWordIn(unit->card, P2_REG_INTMASK) & ~P2_EVENTF_INFO);
      Enable();
      P2DoCmd(unit, P2_CMD_INQUIRE, P2_INFO_COUNTERS, base);
      while((unit->LEWordIn(unit->card, P2_REG_EVENTS) & P2_EVENTF_INFO)
         == 0);
      Cause(&unit->info_int);
   }

   return;
}

/****i* prism2.device/ReportEvents *****************************************
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



/****i* prism2.device/UpdateSignalQuality **********************************
*
*   NAME
*	UpdateSignalQuality -- Read signal quality from card.
*
*   SYNOPSIS
*	UpdateSignalQuality(unit)
*
*	VOID UpdateSignalQuality(struct DevUnit *);
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

VOID UpdateSignalQuality(struct DevUnit *unit, struct DevBase *base)
{
   UWORD signal_level, noise_level;

   P2DoCmd(unit, P2_CMD_ACCESS, P2_REC_LINKQUALITY, base);
   P2Seek(unit, 1, P2_REC_LINKQUALITY, 6, base);
   signal_level = unit->LEWordIn(unit->card, P2_REG_DATA1);
   noise_level = unit->LEWordIn(unit->card, P2_REG_DATA1);

   if(unit->firmware_type == LUCENT_FIRMWARE)
   {
      unit->signal_quality.SignalLevel = signal_level - LUCENT_DBM_OFFSET;
      unit->signal_quality.NoiseLevel = noise_level - LUCENT_DBM_OFFSET;
   }
   else
   {
      unit->signal_quality.SignalLevel =
         signal_level / 3 - INTERSIL_DBM_OFFSET;
      unit->signal_quality.NoiseLevel =
         noise_level / 3 - INTERSIL_DBM_OFFSET;
   }

   return;
}



/****i* prism2.device/LoadFirmware *****************************************
*
*   NAME
*	LoadFirmware
*
*   SYNOPSIS
*	success = LoadFirmware(unit)
*
*	BOOL LoadFirmware(struct DevUnit *);
*
*   FUNCTION
*
*   INPUTS
*	unit - A unit of this device.
*
*   RESULT
*	success - Success indicator.
*
****************************************************************************
*
*/

static BOOL LoadFirmware(struct DevUnit *unit, struct DevBase *base)
{
   BOOL success = TRUE;
   struct FileInfoBlock *info = NULL;
   UWORD control_reg;
   ULONG location, length, start_address;
   BPTR file;
   UBYTE *data = NULL;
   LONG ch;
   UBYTE *buffer = NULL, *p, *end, *q;
   UBYTE n = 0, type;
   UWORD i;

   /* Read firmware file */

   file = Open(firmware_file_name, MODE_OLDFILE);
   if(file == BNULL)
      success = FALSE;

   if(success)
   {
      info = AllocDosObject(DOS_FIB, NULL);
      if(info == NULL)
         success = FALSE;
   }

   if(success)
   {
      if(!ExamineFH(file, info))
         success = FALSE;
   }

   if(success)
   {
      buffer = AllocVec(info->fib_Size, MEMF_ANY);
      end = buffer + info->fib_Size;
      data = AllocVec(MAX_S_REC_SIZE, MEMF_ANY);
      if(buffer == NULL || data == NULL)
         success = FALSE;
   }

   if(success)
   {
      if(Read(file, buffer, info->fib_Size) == -1)
         success = FALSE;
   }

   if(success)
   {
      /* Enable auxiliary ports */

      unit->LEWordOut(unit->card, P2_REG_PARAM0, 0xfe01);
      unit->LEWordOut(unit->card, P2_REG_PARAM1, 0xdc23);
      unit->LEWordOut(unit->card, P2_REG_PARAM2, 0xba45);

      control_reg = unit->LEWordIn(unit->card, P2_REG_CONTROL);
      control_reg =
         control_reg & ~P2_REG_CONTROLF_AUX | P2_REG_CONTROL_ENABLEAUX;
      unit->LEWordOut(unit->card, P2_REG_CONTROL, control_reg);

      BusyMilliDelay(5, base);
      while(
         (unit->LEWordIn(unit->card, P2_REG_CONTROL) & P2_REG_CONTROLF_AUX)
         != P2_REG_CONTROL_AUXENABLED);

      /* Write firmware to card */

      unit->LEWordOut(unit->card, P2_REG_PARAM1, 0);
      P2DoCmd(unit, P2_CMD_PROGRAM | P2_CMDF_WRITE, 0, base);

      p = buffer;
      while(p < end)
      {
         /* Find start of next record */

         ch = *p++;
         if(ch == 'S')
         {
            /* Get record type */

            type = *p++;

            /* Skip length field to keep alignment easy */

            p += 2;

            /* Parse hexadecimal portion of record */

            q = data;
            i = 0;
            while((ch = *p++) >= '0')
            {
               n <<= 4;

               if(ch >= 'A')
                  n |= ch - 'A' + 10;
               else
                  n |= ch - '0';

               if((++i & 0x1) == 0)
               {
                  *q++ = n;
                  n = 0;
               }
            }
            length = i >> 1;

            switch(type)
            {
            case '3':

               /* Get location in card memory to store record's data */

               location = (data[0] << 24) + (data[1] << 16) + (data[2] << 8)
                  + data[3];
               length -= 5;

               /* Write data to card */

               if(unit->firmware_type != HERMES2_FIRMWARE)
               {
                  unit->LEWordOut(unit->card, P2_REG_PARAM1,
                     location >> 16);
                  P2DoCmd(unit, P2_CMD_PROGRAM | P2_CMDF_WRITE, location,
                     base);
               }

               unit->LEWordOut(unit->card, P2_REG_AUXPAGE, location >> 7);
               unit->LEWordOut(unit->card, P2_REG_AUXOFFSET,
                  location & (1 << 7) - 1);

               unit->WordsOut(unit->card, P2_REG_AUXDATA,
                  (UWORD *)(data + 4), length >> 1);
               break;

            case '7':

               /* Get location in card memory to begin execution of new
                  firmware at */

               start_address = (data[0] << 24) + (data[1] << 16)
                  + (data[2] << 8) + data[3];
            }
         }
      }

      /* Disable auxiliary ports */

      control_reg = unit->LEWordIn(unit->card, P2_REG_CONTROL);
      control_reg =
         control_reg & ~P2_REG_CONTROLF_AUX | P2_REG_CONTROL_DISABLEAUX;
      unit->LEWordOut(unit->card, P2_REG_CONTROL, control_reg);

      /* Execute downloaded firmware */

      if(unit->firmware_type == HERMES2_FIRMWARE)
      {
         unit->LEWordOut(unit->card, P2_REG_PARAM1, start_address >> 16);
         P2DoCmd(unit, P2_CMD_EXECUTE, start_address, base);
      }
      else
      {
         unit->LEWordOut(unit->card, P2_REG_PARAM1, start_address >> 16);
         P2DoCmd(unit, P2_CMD_PROGRAM, start_address, base);
         P2DoCmd(unit, P2_CMD_INIT, 0, base);
      }
   }

   /* Free Resources */

   FreeVec(buffer);
   FreeVec(data);
   FreeDosObject(DOS_FIB, info);
   if(file != BNULL)
      Close(file);

   return success;
}



/****i* prism2.device/P2DoCmd **********************************************
*
*   NAME
*	P2DoCmd
*
****************************************************************************
*
* Commands can't fail without software/firmware bug?
*
*/

static VOID P2DoCmd(struct DevUnit *unit, UWORD command, UWORD param,
   struct DevBase *base)
{
   unit->LEWordOut(unit->card, P2_REG_PARAM0, param);
   unit->LEWordOut(unit->card, P2_REG_COMMAND, command);
   while((unit->LEWordIn(unit->card, P2_REG_EVENTS)
      & P2_EVENTF_CMD) == 0);
   unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, P2_EVENTF_CMD);
}



/****i* prism2.device/P2Seek ***********************************************
*
*   NAME
*	P2Seek
*
****************************************************************************
*
*/

static BOOL P2Seek(struct DevUnit *unit, UWORD path_no, UWORD rec_no,
   UWORD offset, struct DevBase *base)
{
   UPINT offset_reg;

   path_no <<= 1;
   offset_reg = P2_REG_OFFSET0 + path_no;
   unit->LEWordOut(unit->card, P2_REG_SELECT0 + path_no,
      rec_no);
   unit->LEWordOut(unit->card, offset_reg, offset);
   while((unit->LEWordIn(unit->card, offset_reg) & P2_REG_OFFSETF_BUSY)
      != 0);
   return (unit->LEWordIn(unit->card, offset_reg) & P2_REG_OFFSETF_ERROR)
      == 0;
}



/****i* prism2.device/P2SetID **********************************************
*
*   NAME
*	P2SetID
*
****************************************************************************
*
*/

static VOID P2SetID(struct DevUnit *unit, UWORD rec_no, const UBYTE *id,
   UWORD length, struct DevBase *base)
{
   P2Seek(unit, 1, rec_no, 0, base);

   unit->LEWordOut(unit->card, P2_REG_DATA1, length / 2 + 3);
   unit->LEWordOut(unit->card, P2_REG_DATA1, rec_no);
   unit->LEWordOut(unit->card, P2_REG_DATA1, length);   /* ??? */
   unit->WordsOut(unit->card, P2_REG_DATA1, (UWORD *)id, (length + 1) / 2);

   P2DoCmd(unit, P2_CMD_ACCESS | P2_CMDF_WRITE, rec_no, base);

   return;
}



/****i* prism2.device/P2SetWord ********************************************
*
*   NAME
*	P2SetWord
*
****************************************************************************
*
*/

static VOID P2SetWord(struct DevUnit *unit, UWORD rec_no, UWORD value,
   struct DevBase *base)
{
   P2Seek(unit, 1, rec_no, 0, base);

   unit->LEWordOut(unit->card, P2_REG_DATA1, 2);
   unit->LEWordOut(unit->card, P2_REG_DATA1, rec_no);
   unit->LEWordOut(unit->card, P2_REG_DATA1, value);

   P2DoCmd(unit, P2_CMD_ACCESS | P2_CMDF_WRITE, rec_no, base);

   return;
}



/****i* prism2.device/P2GetWord ********************************************
*
*   NAME
*	P2GetWord
*
****************************************************************************
*
*/

static UWORD P2GetWord(struct DevUnit *unit, UWORD rec_no,
   struct DevBase *base)
{
   P2DoCmd(unit, P2_CMD_ACCESS, rec_no, base);

   P2Seek(unit, 1, rec_no, 4, base);

   return unit->LEWordIn(unit->card, P2_REG_DATA1);
}



/****i* prism2.device/P2AllocMem *******************************************
*
*   NAME
*	P2AllocMem
*
****************************************************************************
*
*/

static UWORD P2AllocMem(struct DevUnit *unit, UWORD size,
   struct DevBase *base)
{
   UWORD id;
   P2DoCmd(unit, P2_CMD_ALLOCMEM, size, base);
   while((unit->LEWordIn(unit->card, P2_REG_EVENTS) & P2_EVENTF_ALLOCMEM)
      == 0);
   id = unit->LEWordIn(unit->card, P2_REG_ALLOCFID);
   unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, P2_EVENTF_ALLOCMEM);
   return id;
}



/****i* prism2.device/P2SetData ********************************************
*
*   NAME
*	P2SetData
*
****************************************************************************
*
*/

static VOID P2SetData(struct DevUnit *unit, UWORD rec_no, const UBYTE *data,
   UWORD length, struct DevBase *base)
{
   length = (length + 1) / 2;
   P2Seek(unit, 1, rec_no, 0, base);

   unit->LEWordOut(unit->card, P2_REG_DATA1, 1 + length);
   unit->LEWordOut(unit->card, P2_REG_DATA1, rec_no);
   unit->WordsOut(unit->card, P2_REG_DATA1, (const UWORD *)data, length);

   P2DoCmd(unit, P2_CMD_ACCESS | P2_CMDF_WRITE, rec_no, base);

   return;
}



/****i* prism2.device/UnitTask *********************************************
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



/****i* prism2.device/BusyMilliDelay ***************************************
*
*   NAME
*	BusyMilliDelay - Busy-wait for specified number of milliseconds.
*
*   SYNOPSIS
*	BusyMilliDelay(millis)
*
*	VOID BusyMilliDelay(ULONG);
*
****************************************************************************
*
*/

static VOID BusyMilliDelay(ULONG millis, struct DevBase *base)
{
   struct timeval time, end_time;

   GetSysTime(&end_time);
   time.tv_secs = 0;
   time.tv_micro = millis * 1000;
   AddTime(&end_time, &time);

   while(CmpTime(&end_time, &time) < 0)
      GetSysTime(&time);

   return;
}



/****i* prism2.device/StrLen ***********************************************
*
*   NAME
*	StrLen
*
*   SYNOPSIS
*	length = StrLen(s)
*
*	UPINT StrLen(TEXT *);
*
****************************************************************************
*
*/

static UPINT StrLen(const TEXT *s)
{
   const TEXT *p;

   for(p = s; *p != '\0'; p++);
   return p - s;
}

#undef SysBase

/****i* prism2.device/StatusInt ********************************************
*
*   NAME
*	StatusInt
*
*   SYNOPSIS
*	finished = StatusInt(unit)
*
*	BOOL StatusInt(struct DevUnit *);
*
*   FUNCTION
*
*   INPUTS
*	unit
*
*   RESULT
*	finished
*
****************************************************************************
*
* int_code is really in A5, but GCC 2.95.3 doesn't seem able to handle that.
* Since we don't use this parameter, we can lie.
*
*/

AROS_UFIH1(StatusInt, struct DevUnit *, unit)
{
   AROS_USERFUNC_INIT

   struct DevBase *base;
   UWORD events, int_mask;

   base = unit->device;
   events = unit->LEWordIn(unit->card, P2_REG_EVENTS);

   /* Turning off ints acknowledges the request? */

   int_mask = unit->LEWordIn(unit->card, P2_REG_INTMASK);
   unit->LEWordOut(unit->card, P2_REG_INTMASK, 0);

   /* Handle events */

   if((events & P2_EVENTF_INFO) != 0)
   {
      int_mask &= ~P2_EVENTF_INFO;
      Cause(&unit->info_int);
   }
   if((events & P2_EVENTF_RX) != 0)
   {
      int_mask &= ~P2_EVENTF_RX;
      Cause(&unit->rx_int);
   }
   if((events & P2_EVENTF_ALLOCMEM) != 0)
   {
      unit->tx_frame_id = unit->LEWordIn(unit->card, P2_REG_ALLOCFID);
      unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, P2_EVENTF_ALLOCMEM);
      Cause(&unit->tx_int);
   }
   if((events & P2_EVENTF_TXFAIL) != 0)
   {
      ReportEvents(unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_TX,
         base);
      unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, P2_EVENTF_TXFAIL);
   }

#ifdef __MORPHOS__
   int_mask = INT_MASK;
#endif
   unit->LEWordOut(unit->card, P2_REG_INTMASK, int_mask);

   return FALSE;

   AROS_USERFUNC_EXIT
}



/****i* prism2.device/RXInt ************************************************
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

static AROS_UFIH1(RXInt, struct DevUnit *, unit)
{
   AROS_USERFUNC_INIT

   UWORD frame_status, offset, packet_size, frame_id, message_type;
   struct DevBase *base;
   BOOL is_orphan, accepted, is_snap;
   ULONG packet_type;
   UBYTE *buffer;
   struct IOSana2Req *request, *request_tail;
   struct Opener *opener, *opener_tail;
   struct TypeStats *tracker;

   base = unit->device;
   buffer = unit->rx_buffer;

   while((unit->LEWordIn(unit->card, P2_REG_EVENTS) & P2_EVENTF_RX) != 0)
   {
      unit->stats.PacketsReceived++;
      frame_id = unit->LEWordIn(unit->card, P2_REG_RXFID);
      P2Seek(unit, 1, frame_id, P2_FRM_STATUS, base);
      frame_status = unit->LEWordIn(unit->card, P2_REG_DATA1);

      if((frame_status & (P2_FRM_STATUSF_BADCRYPT | P2_FRM_STATUSF_BADCRC))
         == 0)
      {
         /* Read packet header */

         is_orphan = TRUE;
         if(unit->firmware_type == HERMES2_FIRMWARE)
            offset = P2_H2FRM_ETHFRAME;
         else
            offset = P2_FRM_ETHFRAME;
         P2Seek(unit, 1, frame_id, offset, base);
         unit->WordsIn(unit->card, P2_REG_DATA1, (UWORD *)buffer,
            ETH_SNAPHEADERSIZE / 2);

         if(AddressFilter(unit, buffer + ETH_PACKET_DEST, base))
         {
            packet_size = BEWord(*((UWORD *)(buffer + ETH_PACKET_IEEELEN)))
               + ETH_HEADERSIZE;
            message_type = frame_status >> P2_FRM_STATUSB_MSGTYPE;
            is_snap = message_type == P2_MSGTYPE_RFC1042 ||
               message_type == P2_MSGTYPE_TUNNEL;
            if(is_snap)
            {
               packet_type =
                  BEWord(*((UWORD *)(buffer + ETH_PACKET_SNAPTYPE)));
               packet_size -= ETH_SNAPHEADERSIZE - ETH_HEADERSIZE;
            }
            else
               packet_type =
                  BEWord(*((UWORD *)(buffer + ETH_PACKET_IEEELEN)));

            opener = (APTR)unit->openers.mlh_Head;
            opener_tail = (APTR)&unit->openers.mlh_Tail;

            /* Offer packet to every opener */

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
                        buffer, !is_orphan, frame_id, base);
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
                     FALSE, frame_id, base);
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
         }
      }
      else
      {
         ReportEvents(unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX,
            base);
      }

      /* Discard packet */

      unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, P2_EVENTF_RX);
   }

   /* Re-enable RX interrupts */

   Disable();
   unit->LEWordOut(unit->card, P2_REG_INTMASK,
      unit->LEWordIn(unit->card, P2_REG_INTMASK) | P2_EVENTF_RX);
   Enable();
   return FALSE;

   AROS_USERFUNC_EXIT
}

/****i* prism2.device/TXInt ************************************************
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

static AROS_UFIH1(TXInt, struct DevUnit *, unit)
{
   AROS_USERFUNC_INIT

   UWORD i, packet_size, data_size, packet_type, ieee_length,
      frame_id;
   UBYTE *buffer;
   struct DevBase *base;
   struct IOSana2Req *request;
   BOOL is_ieee;
   struct Opener *opener;
   ULONG wire_error;
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   BYTE error = 0;
   struct MsgPort *port;
   struct TypeStats *tracker;

   base = unit->device;
   port = unit->request_ports[WRITE_QUEUE];

   if(unit->tx_frame_id != 0 && !IsMsgPortEmpty(port))
   {
      request = (APTR)port->mp_MsgList.lh_Head;
      data_size = packet_size = request->ios2_DataLength;

      if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
         packet_size += ETH_PACKET_DATA;

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
         if(!opener->tx_function(buffer, request->ios2_Data, data_size))
         {
            error = S2ERR_NO_RESOURCES;
            wire_error = S2WERR_BUFF_ERROR;
            ReportEvents(unit,
               S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_TX,
               base);
         }
      }

      if(error == 0)
      {
         /* Get packet type or length */

         if((request->ios2_Req.io_Flags & SANA2IOF_RAW) != 0)
            packet_type = BEWord(*(UWORD *)(buffer + ETH_PACKET_TYPE));
         else
            packet_type = request->ios2_PacketType;
         is_ieee = packet_type <= ETH_MTU;

         /* Write packet descriptor */

         if(unit->firmware_type == HERMES2_FIRMWARE)
         {
            P2Seek(unit, 0, unit->tx_frame_id, 0, base);
            for(i = 0; i < P2_H2FRM_ETHFRAME / 2; i++)
               unit->LEWordOut(unit->card, P2_REG_DATA0, 0);
         }
         else
         {
            P2Seek(unit, 0, unit->tx_frame_id, 0, base);
            for(i = 0; i < P2_FRM_HEADER / 2; i++)
               unit->LEWordOut(unit->card, P2_REG_DATA0, 0);
            unit->LEWordOut(unit->card, P2_REG_DATA0,
               IEEE802_11_FRMTYPE_DATA << IEEE802_11_FRM_CONTROLB_TYPE);
            for(i++; i < P2_FRM_ETHFRAME / 2; i++)
               unit->LEWordOut(unit->card, P2_REG_DATA0, 0);
         }

         /* Write packet header */

         if((request->ios2_Req.io_Flags & SANA2IOF_RAW) != 0)
         {
            unit->WordsOut(unit->card, P2_REG_DATA0, (UWORD *)buffer,
               ETH_ADDRESSSIZE * 2 / 2);
            buffer += ETH_HEADERSIZE;
         }
         else
         {
            unit->WordsOut(unit->card, P2_REG_DATA0,
               (UWORD *)request->ios2_DstAddr, ETH_ADDRESSSIZE / 2);
            unit->WordsOut(unit->card, P2_REG_DATA0, (UWORD *)unit->address,
               ETH_ADDRESSSIZE / 2);
         }

         if(is_ieee)
         {
            ieee_length = packet_type;
         }
         else
         {
            ieee_length = request->ios2_DataLength + ETH_SNAPHEADERSIZE
               - ETH_HEADERSIZE;
            if((request->ios2_Req.io_Flags & SANA2IOF_RAW) != 0)
               ieee_length -= ETH_HEADERSIZE;
         }

         unit->BEWordOut(unit->card, P2_REG_DATA0, ieee_length);

         if(!is_ieee)
         {
            unit->WordsOut(unit->card, P2_REG_DATA0, (UWORD *)snap_stuff,
               (ETH_PACKET_SNAPTYPE - ETH_HEADERSIZE) / 2);
            unit->BEWordOut(unit->card, P2_REG_DATA0, packet_type);
         }

         /* Write packet data and send */

         unit->WordsOut(unit->card, P2_REG_DATA0, (UWORD *)buffer,
            (packet_size - ETH_HEADERSIZE + 1) / 2);
         frame_id = unit->tx_frame_id;
         unit->tx_frame_id = 0;
         P2DoCmd(unit, P2_CMD_TX | P2_CMDF_RECLAIM, frame_id, base);
      }

      /* Reply request */

      request->ios2_Req.io_Error = error;
      request->ios2_WireError = wire_error;
      Remove((APTR)request);
      ReplyMsg((APTR)request);

      /* Update statistics */

      if(error == 0)
      {
         unit->stats.PacketsSent++;

         tracker = FindTypeStats(unit, &unit->type_trackers,
            request->ios2_PacketType, base);
         if(tracker != NULL)
         {
            tracker->stats.PacketsSent++;
            tracker->stats.BytesSent += packet_size;
         }
      }
   }

   /* Don't try to keep sending packets if there's no space left */

   if(unit->tx_frame_id != 0)
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
   else
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;

   return FALSE;

   AROS_USERFUNC_EXIT
}


/****i* prism2.device/InfoInt *********************************************
*
*   NAME
*	InfoInt
*
*   SYNOPSIS
*	InfoInt(unit)
*
*	VOID InfoInt(struct DevUnit *);
*
*   FUNCTION
*
*   INPUTS
*	unit - A unit of this device.
*
*   RESULT
*	None.
*
*   NOTES
*	The only reason this is a (soft) interrupt is so that it won't
*	interfere with RXInt() by interrupting it. This would be dangerous
*	because they use the same data channel.
*
****************************************************************************
*
*/

static AROS_UFIH1(InfoInt, struct DevUnit *, unit)
{
   AROS_USERFUNC_INIT

   struct DevBase *base;
   UWORD id;

   base = unit->device;
   id = unit->LEWordIn(unit->card, P2_REG_INFOFID);

   /* Read useful stats and skip others */

   P2Seek(unit, 1, id, 2, base);

   if(unit->LEWordIn(unit->card, P2_REG_DATA1) == P2_INFO_COUNTERS)
   {
      unit->LEWordIn(unit->card, P2_REG_DATA1);
      unit->LEWordIn(unit->card, P2_REG_DATA1);

      unit->LEWordIn(unit->card, P2_REG_DATA1);
      unit->LEWordIn(unit->card, P2_REG_DATA1);
      unit->LEWordIn(unit->card, P2_REG_DATA1);
      unit->LEWordIn(unit->card, P2_REG_DATA1);

      unit->special_stats[S2SS_ETHERNET_RETRIES & 0xffff] +=
         unit->LEWordIn(unit->card, P2_REG_DATA1) +
         unit->LEWordIn(unit->card, P2_REG_DATA1) +
         unit->LEWordIn(unit->card, P2_REG_DATA1);

      unit->LEWordIn(unit->card, P2_REG_DATA1);

      unit->LEWordIn(unit->card, P2_REG_DATA1);
      unit->LEWordIn(unit->card, P2_REG_DATA1);

      unit->LEWordIn(unit->card, P2_REG_DATA1);
      unit->LEWordIn(unit->card, P2_REG_DATA1);
      unit->LEWordIn(unit->card, P2_REG_DATA1);

      unit->stats.BadData += unit->LEWordIn(unit->card, P2_REG_DATA1);
      unit->stats.Overruns += unit->LEWordIn(unit->card, P2_REG_DATA1);

      unit->LEWordIn(unit->card, P2_REG_DATA1);

      unit->stats.BadData += unit->LEWordIn(unit->card, P2_REG_DATA1);
   }

   /* Acknowledge event and re-enable info interrupts */

   unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, P2_EVENTF_INFO);
   Disable();
   if((unit->flags & UNITF_ONLINE) != 0)
      unit->LEWordOut(unit->card, P2_REG_INTMASK,
         unit->LEWordIn(unit->card, P2_REG_INTMASK) | P2_EVENTF_INFO);
   Enable();

   return FALSE;

   AROS_USERFUNC_EXIT
}




