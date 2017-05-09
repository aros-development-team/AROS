/*

Copyright (C) 2001-2017 Neil Cafferkey

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
#include <exec/tasks.h>

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
#include "encryption_protos.h"
#include "timer_protos.h"


#define TASK_PRIORITY 0
#define STACK_SIZE 4096
#define INT_MASK \
   (P2_EVENTF_INFO | P2_EVENTF_ALLOCMEM | P2_EVENTF_TXFAIL | P2_EVENTF_RX)
#define MAX_S_REC_SIZE 50
#define LUCENT_DBM_OFFSET   149
#define INTERSIL_DBM_OFFSET 100
#define SCAN_BUFFER_SIZE 2000
#define BEACON_BUFFER_SIZE 8000
#define SCAN_TAG_COUNT 8          +10
#define INFO_TAG_COUNT 4          +10
#define LUCENT_PDA_ADDRESS 0x390000
#define LUCENT_PDA_SIZE 1000
#define FRAME_BUFFER_SIZE (P2_H2FRM_ETHFRAME + ETH_HEADERSIZE \
   + SNAP_HEADERSIZE + ETH_MTU + EIV_SIZE + ICV_SIZE + MIC_SIZE)


static struct AddressRange *FindMulticastRange(struct DevUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
   UWORD upper_bound_right, struct DevBase *base);
static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base);
static VOID RXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
static UBYTE *GetRXBuffer(struct DevUnit *unit, const UBYTE *address,
   UWORD frag_no, UWORD *buffer_no, struct DevBase *base);
static VOID DistributeRXPacket(struct DevUnit *unit, UBYTE *frame,
   struct DevBase *base);
static VOID CopyPacket(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD packet_size, UWORD packet_type, UBYTE *buffer,
   struct DevBase *base);
static BOOL AddressFilter(struct DevUnit *unit, UBYTE *address,
   struct DevBase *base);
static VOID SaveBeacon(struct DevUnit *unit, const UBYTE *frame,
   struct DevBase *base);
static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
static VOID InfoInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
static VOID ResetHandler(REG(a1, struct DevUnit *unit),
   REG(a6, APTR int_code));
static VOID ReportEvents(struct DevUnit *unit, ULONG events,
   struct DevBase *base);
static BOOL LoadFirmware(struct DevUnit *unit, struct DevBase *base);
static const TEXT *ParseNextSRecord(const TEXT *s, UBYTE *type, UBYTE *data,
   UWORD *data_length, ULONG *location, struct DevBase *base);
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
static BOOL P2ReadRec(struct DevUnit *unit, UWORD rec_no, APTR buffer,
   UWORD max_length, struct DevBase *base);
static LONG ConvertLevel(struct DevUnit *unit, UWORD raw_level,
   struct DevBase *base);
static LONG ConvertScanLevel(struct DevUnit *unit, UWORD raw_level,
   struct DevBase *base);
static UBYTE *GetIE(UBYTE id, UBYTE *ies, UWORD ies_length,
   struct DevBase *base);
static BOOL CompareMACAddresses(APTR mac1, APTR mac2);
static VOID UnitTask(struct DevUnit *unit);
static UPINT StrLen(const TEXT *s);


static const UBYTE snap_template[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
static const UBYTE scan_params[] = {0xff, 0x3f, 0x01, 0x00, 0x00, 0x00};
static const TEXT h1_firmware_file_name[] = "DEVS:Firmware/HermesI";
static const TEXT h2_firmware_file_name[] = "DEVS:Firmware/HermesII";
static const TEXT h25_firmware_file_name[] = "DEVS:Firmware/HermesII.5";
static const UBYTE h2_wpa_ie[] =
{
   0xdd, 0x18, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00,
   0x00, 0x50, 0xf2, 0x02, 0x01, 0x00, 0x00, 0x50,
   0xf2, 0x02, 0x01, 0x00, 0x00, 0x50, 0xf2, 0x02,
   0x00, 0x00
};


#ifdef __mc68000
#define AddUnitTask(task, initial_pc, unit) \
   ({ \
      task->tc_SPReg -= sizeof(APTR); \
      *((APTR *)task->tc_SPReg) = unit; \
      AddTask(task, initial_pc, NULL); \
   })
#endif
#ifdef __amigaos4__
#define AddUnitTask(task, initial_pc, unit) \
   ({ \
      struct TagItem _task_tags[] = \
         {{AT_Param1, (UPINT)unit}, {TAG_END, 0}}; \
      AddTask(task, initial_pc, NULL, _task_tags); \
   })
#endif
#ifdef __MORPHOS__
#define AddUnitTask(task, initial_pc, unit) \
   ({ \
      struct TagItem _task_tags[] = \
      { \
         {TASKTAG_CODETYPE, CODETYPE_PPC}, \
         {TASKTAG_PC, (UPINT)initial_pc}, \
         {TASKTAG_PPC_ARG1, (UPINT)unit}, \
         {TAG_END, 1} \
      }; \
      struct TaskInitExtension _task_init = {0xfff0, 0, _task_tags}; \
      AddTask(task, &_task_init, NULL); \
   })
#endif
#ifdef __AROS__
#define AddUnitTask(task, initial_pc, unit) \
   ({ \
      struct TagItem _task_tags[] = \
         {{TASKTAG_ARG1, (UPINT)unit}, {TAG_END, 0}}; \
      NewAddTask(task, initial_pc, NULL, _task_tags); \
   })
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

      /* Allocate buffers */

      unit->rx_buffer = AllocVec(FRAME_BUFFER_SIZE, MEMF_PUBLIC);
      unit->rx_buffers = AllocVec(FRAME_BUFFER_SIZE * RX_BUFFER_COUNT,
         MEMF_PUBLIC);
      for(i = 0; i < RX_BUFFER_COUNT; i++)
         unit->rx_fragment_nos[i] = -1;
      unit->tx_buffer = AllocVec(ETH_MAXPACKETSIZE, MEMF_PUBLIC);
      unit->rx_descriptor = AllocVec(FRAME_BUFFER_SIZE,
         MEMF_PUBLIC | MEMF_CLEAR);
      unit->tx_descriptor = AllocVec(FRAME_BUFFER_SIZE,
         MEMF_PUBLIC | MEMF_CLEAR);
      unit->scan_results_rec = AllocVec(SCAN_BUFFER_SIZE, MEMF_PUBLIC);
      unit->next_beacon = unit->beacons =
         AllocVec(BEACON_BUFFER_SIZE, MEMF_PUBLIC);
      if(unit->rx_buffer == NULL || unit->rx_buffers == NULL
         || unit->tx_buffer == NULL || unit->rx_descriptor == NULL
         || unit->tx_descriptor == NULL || unit->scan_results_rec == NULL
         || unit->beacons == NULL)
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

      unit->reset_handler.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->reset_handler.is_Code = (APTR)ResetHandler;
      unit->reset_handler.is_Data = unit;

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
      task->tc_SPReg = task->tc_SPUpper;
      NewList(&task->tc_MemEntry);

      if(AddUnitTask(task, UnitTask, unit) != NULL)
         unit->flags |= UNITF_TASKADDED;
      else
         success = FALSE;
   }

   if(success)
   {
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
      /* Remove task */

      task = unit->task;
      if(task != NULL)
      {
         if((unit->flags & UNITF_TASKADDED) != 0)
         {
            RemTask(task);
            FreeMem(task->tc_SPLower, STACK_SIZE);
         }
         FreeMem(task, sizeof(struct Task));
      }

      /* Free request queues */

      for(i = 0; i < REQUEST_QUEUE_COUNT; i++)
      {
         if(unit->request_ports[i] != NULL)
            FreeMem(unit->request_ports[i], sizeof(struct MsgPort));
      }

      /* Go offline */

      if((unit->flags & UNITF_ONLINE) != 0)   /* Needed! */
         GoOffline(unit, base);

      /* Clear target SSID */

      P2SetID(unit, P2_REC_DESIREDSSID, unit->ssid, 0, base);

      /* Free buffers and unit structure */

      FreeVec(unit->beacons);
      FreeVec(unit->scan_results_rec);
      FreeVec(unit->tx_descriptor);
      FreeVec(unit->rx_descriptor);
      FreeVec(unit->tx_buffer);
      FreeVec(unit->rx_buffers);
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
   UWORD id, version, revision, i;
   BOOL success = TRUE, loaded;
   UBYTE address[ETH_ADDRESSSIZE];
   WORD length;

   /* Wait for card to be ready following bus-specific reset, then start
      it */

   while((unit->LEWordIn(unit->card, P2_REG_COMMAND)
      & P2_REG_COMMANDF_BUSY) != 0);

   P2DoCmd(unit, P2_CMD_INIT, 0, base);

   /* Determine firmware type */

   P2DoCmd(unit, P2_CMD_ACCESS, P2_REC_NICIDENTITY, base);
   P2Seek(unit, 1, P2_REC_NICIDENTITY, 4, base);
   id = unit->LEWordIn(unit->card, P2_REG_DATA1);
   unit->LEWordIn(unit->card, P2_REG_DATA1);
   version = unit->LEWordIn(unit->card, P2_REG_DATA1);
   revision = unit->LEWordIn(unit->card, P2_REG_DATA1);

   if((id & 0x8000) != 0)
   {
      if(version == 0)
         unit->firmware_type = SYMBOL_FIRMWARE;
      else
         unit->firmware_type = INTERSIL_FIRMWARE;
   }
   else
   {
      P2DoCmd(unit, P2_CMD_ACCESS, P2_REC_PRIIDENTITY, base);
      P2Seek(unit, 1, P2_REC_PRIIDENTITY, 0, base);
      length = unit->LEWordIn(unit->card, P2_REG_DATA1);
      if(length == 5)
         unit->firmware_type = LUCENT_FIRMWARE;
      else
      {
         P2DoCmd(unit, P2_CMD_ACCESS, P2_REC_STAIDENTITY, base);
         P2Seek(unit, 1, P2_REC_STAIDENTITY, 4, base);
         id = unit->LEWordIn(unit->card, P2_REG_DATA1);
         unit->LEWordIn(unit->card, P2_REG_DATA1);
         version = unit->LEWordIn(unit->card, P2_REG_DATA1);
         revision = unit->LEWordIn(unit->card, P2_REG_DATA1);
         if(version > 1)
            unit->firmware_type = HERMES2G_FIRMWARE;
         else
            unit->firmware_type = HERMES2_FIRMWARE;
      }
   }

   /* Download firmware if necessary or available */

   loaded = LoadFirmware(unit, base);
   if(!loaded && unit->firmware_type >= HERMES2_FIRMWARE)
      success = FALSE;

   if(success)
   {
      /* Determine features, and get offsets of certain fields within frame
         descriptors */

      P2DoCmd(unit, P2_CMD_ACCESS, P2_REC_STAIDENTITY, base);
      P2Seek(unit, 1, P2_REC_STAIDENTITY, 4, base);
      unit->LEWordIn(unit->card, P2_REG_DATA1);
      unit->LEWordIn(unit->card, P2_REG_DATA1);
      version = unit->LEWordIn(unit->card, P2_REG_DATA1);
      revision = unit->LEWordIn(unit->card, P2_REG_DATA1);

      if(P2GetWord(unit, P2_REC_HASWEP, base) != 0)
         unit->flags |= UNITF_HASWEP | UNITF_HARDWEP;

      unit->ethernet_offset = P2_FRM_ETHFRAME;
      unit->data_offset = P2_FRM_DATA;
      unit->txcontrol_offset = P2_FRM_TXCONTROL;
      unit->datalen_offset = P2_FRM_DATALEN;

      if(unit->firmware_type == LUCENT_FIRMWARE)
      {
         if(version > 6 || version == 6 && revision >= 6)
            unit->flags |= UNITF_HASADHOC;
         if(version >= 9)
         {
            unit->flags |= UNITF_HASTKIP | UNITF_HARDTKIP;
            unit->txcontrol_offset = P2_FRM_ALTTXCONTROL;
         }
      }
      else if(unit->firmware_type >= HERMES2_FIRMWARE)
      {
         unit->flags |= UNITF_HASADHOC | UNITF_HASTKIP | UNITF_HARDTKIP;
         unit->ethernet_offset = P2_H2FRM_ETHFRAME;
         unit->data_offset = P2_H2FRM_DATA;
         unit->txcontrol_offset = P2_H2FRM_TXCONTROL;
         unit->datalen_offset = P2_H2FRM_DATALEN;
      }
      else if(unit->firmware_type == INTERSIL_FIRMWARE)
      {
         unit->flags |= UNITF_HASADHOC | UNITF_HASWEP;
         if(version == 1 && revision >= 7)
            unit->flags |= UNITF_HASTKIP | UNITF_HASCCMP;
      }

      /* Get default channel and MAC address */

      unit->channel = P2GetWord(unit, P2_REC_OWNCHNL, base);
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

      unit->tx_frame_id = P2AllocMem(unit, FRAME_BUFFER_SIZE, base);

      /* Set IV sizes */

      if((unit->flags & UNITF_HARDWEP) == 0)
         unit->iv_sizes[S2ENC_WEP] = IV_SIZE;
      if((unit->flags & UNITF_HARDTKIP) == 0)
         unit->iv_sizes[S2ENC_TKIP] = EIV_SIZE;
      unit->iv_sizes[S2ENC_CCMP] = EIV_SIZE;

      /* Set encryption functions */

      unit->fragment_encrypt_functions[S2ENC_NONE] = WriteClearFragment;

      if((unit->flags & UNITF_HARDWEP) != 0)
         unit->fragment_encrypt_functions[S2ENC_WEP] = WriteClearFragment;
      else
         unit->fragment_encrypt_functions[S2ENC_WEP] = EncryptWEPFragment;

      if((unit->flags & UNITF_HARDTKIP) != 0)
         unit->fragment_encrypt_functions[S2ENC_TKIP] = WriteClearFragment;
      else
         unit->fragment_encrypt_functions[S2ENC_TKIP] = EncryptTKIPFragment;

      unit->fragment_encrypt_functions[S2ENC_CCMP] = EncryptCCMPFragment;

      /* Set decryption functions */

      unit->fragment_decrypt_functions[S2ENC_NONE] = ReadClearFragment;

      if((unit->flags & UNITF_HARDWEP) != 0)
         unit->fragment_decrypt_functions[S2ENC_WEP] = ReadClearFragment;
      else
         unit->fragment_decrypt_functions[S2ENC_WEP] = DecryptWEPFragment;

      if((unit->flags & UNITF_HARDTKIP) != 0)
         unit->fragment_decrypt_functions[S2ENC_TKIP] = ReadClearFragment;
      else
         unit->fragment_decrypt_functions[S2ENC_TKIP] = DecryptTKIPFragment;

      unit->fragment_decrypt_functions[S2ENC_CCMP] = DecryptCCMPFragment;
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
   UWORD i, key_length, port_type, lowest_enc = S2ENC_CCMP,
      highest_enc = S2ENC_NONE, enc_flags, size, value;
   const struct KeyUnion *keys;

   /* Set MAC address */

   P2SetData(unit, P2_REC_ADDRESS, unit->address, ETH_ADDRESSSIZE,
      base);

   /* Decide on promiscuous mode */

   P2SetWord(unit, P2_REC_PROMISC, FALSE, base);
   SetMulticast(unit, base);

   /* Set wireless parameters */

   if(unit->mode == S2PORT_ADHOC)
   {
      P2SetWord(unit, P2_REC_OWNCHNL, unit->channel, base);
      P2SetID(unit, P2_REC_OWNSSID, unit->ssid, unit->ssid_length, base);
   }
   if(unit->mode == S2PORT_MANAGED)
      port_type = 1;
   else
      port_type = 0;
   if((unit->flags & UNITF_ONLINE) == 0)
      P2SetWord(unit, P2_REC_PORTTYPE, port_type, base);
   P2SetWord(unit, P2_REC_CREATEIBSS, unit->mode == S2PORT_ADHOC, base);
   P2SetID(unit, P2_REC_DESIREDSSID, unit->ssid, unit->ssid_length, base);

   /* Determine highest encryption type in use */

   for(i = 0; i < WIFI_KEYCOUNT; i++)
   {
      if(unit->keys[i].type > highest_enc)
         highest_enc = unit->keys[i].type;
      if(unit->keys[i].type < lowest_enc)
         lowest_enc = unit->keys[i].type;
   }

   if(unit->wpa_ie[1] != 0)
      highest_enc = S2ENC_TKIP;

   /* Allow reception of beacon/probe-response frames */

   if(unit->firmware_type == INTERSIL_FIRMWARE)
      P2SetWord(unit, P2_REC_RXMGMTFRAMES, 1, base);

   /* Transmit at 11Mbps, with fallback */

   if(unit->firmware_type == INTERSIL_FIRMWARE
      || unit->firmware_type == SYMBOL_FIRMWARE)
      P2SetWord(unit, P2_REC_TXRATE, 0xf, base);

   /* Configure authentication and encryption */

   if(unit->firmware_type >= LUCENT_FIRMWARE)
   {
      /* Set authentication and encryption modes */

      P2SetWord(unit, P2_REC_ALTAUTHTYPE, unit->auth_types, base);

      P2SetWord(unit, P2_REC_ALTENCRYPTION, highest_enc, base);

      /* Set up firmware-based WEP encryption if appropriate */

      if(highest_enc == S2ENC_WEP && (unit->flags & UNITF_HARDWEP) != 0)
      {
         P2SetWord(unit, P2_REC_ALTTXCRYPTKEY, unit->tx_key_no, base);
         P2Seek(unit, 1, P2_REC_DEFLTCRYPTKEYS, 0, base);
         unit->LEWordOut(unit->card, P2_REG_DATA1, P2_ALTWEPRECLEN);
         unit->LEWordOut(unit->card, P2_REG_DATA1, P2_REC_DEFLTCRYPTKEYS);
         keys = unit->keys;
         for(i = 0; i < WIFI_KEYCOUNT; i++)
         {
            key_length = keys[i].u.wep.length;
            if(key_length == 0)
               key_length = WIFI_WEP128LEN;
            unit->LEWordOut(unit->card, P2_REG_DATA1, key_length);
            unit->WordsOut(unit->card, P2_REG_DATA1,
               (UWORD *)keys[i].u.wep.key, (key_length + 1) / 2);
         }

         P2DoCmd(unit, P2_CMD_ACCESS | P2_CMDF_WRITE,
            P2_REC_DEFLTCRYPTKEYS, base);
      }

      /* Set key management suite to PSK */

      if(highest_enc > S2ENC_WEP)
      {
         value = (unit->firmware_type >= HERMES2_FIRMWARE) ? 4 : 2;
         P2SetWord(unit, P2_REC_KEYMGMTSUITE, value, base);
      }
   }
   else
   {
      /* Set authentication mode */

      P2SetWord(unit, P2_REC_AUTHTYPE, unit->auth_types, base);

      /* Set encryption flags */

      if(highest_enc > S2ENC_NONE)
         enc_flags = P2_REC_ENCRYPTIONF_ENABLE;
      else
         enc_flags = 0;

      if(highest_enc > S2ENC_WEP
         || highest_enc == S2ENC_WEP && (unit->flags & UNITF_HARDWEP) == 0)
         enc_flags |= P2_REC_ENCRYPTIONF_HOSTDECRYPT
            | P2_REC_ENCRYPTIONF_HOSTENCRYPT;
      P2SetWord(unit, P2_REC_ENCRYPTION, enc_flags, base);

      /* Set up firmware-based WEP encryption if appropriate */

      if(highest_enc == S2ENC_WEP && (unit->flags & UNITF_HARDWEP) != 0)
      {
         P2SetWord(unit, P2_REC_TXCRYPTKEY, unit->tx_key_no, base);

         keys = unit->keys;
         for(i = 0; i < WIFI_KEYCOUNT; i++)
         {
            key_length = keys[i].u.wep.length;
            if(key_length == 0)
               key_length = keys[unit->tx_key_no].u.wep.length;
            P2SetData(unit, P2_REC_CRYPTKEY0 + i, keys[i].u.wep.key,
               key_length, base);
         }
      }

      /* Set or clear WPA IE */

      if(highest_enc > S2ENC_WEP)
         size = unit->wpa_ie[1] + 2;
      else
         size = 0;
      P2SetID(unit, P2_REC_WPAIE, unit->wpa_ie, size, base);

      /* Let supplicant handle association and roaming */

      P2SetWord(unit, P2_REC_ROAMINGMODE, 3, base);
   }

   /* Restart the transceiver if we're already online */

   if((unit->flags & UNITF_ONLINE) != 0)
   {
      P2DoCmd(unit, P2_CMD_DISABLE, 0, base);
      P2DoCmd(unit, P2_CMD_ENABLE, 0, base);

      /* Attempt to join specified network */

      if(unit->firmware_type == INTERSIL_FIRMWARE)
      {
         unit->bssid[ETH_ADDRESSSIZE] = unit->channel;
         P2SetData(unit, P2_REC_JOIN, unit->bssid, ETH_ADDRESSSIZE + 2,
            base);
      }
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
      unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, 0xffff);

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



/****i* prism2.device/SetOptions *******************************************
*
*   NAME
*	SetOptions -- Set and use interface options.
*
*   SYNOPSIS
*	SetOptions(unit, tag_list)
*
*	VOID SetOptions(struct DevUnit *, struct TagItem *);
*
****************************************************************************
*
*/

VOID SetOptions(struct DevUnit *unit, const struct TagItem *tag_list,
   struct DevBase *base)
{
   struct TagItem *tag_item, *tlist = (struct TagItem *)tag_list;
   const TEXT *id;
   UPINT length;
   const UBYTE *ie;

   while((tag_item = NextTagItem(&tlist)) != NULL)
   {
      switch(tag_item->ti_Tag)
      {
      case S2INFO_SSID:
         id = (const TEXT *)tag_item->ti_Data;
         length = StrLen(id);
         CopyMem(id, unit->ssid, length);
         unit->ssid_length = length;
         break;

      case S2INFO_BSSID:
         CopyMem((APTR)tag_item->ti_Data, unit->bssid, ETH_ADDRESSSIZE);
         break;

      case S2INFO_DefaultKeyNo:
         unit->tx_key_no = tag_item->ti_Data;
         break;

      case S2INFO_PortType:
         unit->mode = tag_item->ti_Data;
         break;

      case S2INFO_Channel:
         unit->channel = tag_item->ti_Data;
         break;

      case S2INFO_WPAInfo:
         if(tag_item->ti_Data != (UPINT)NULL)
         {
            /* Hermes-II uses an "unusual" WPA IE in its association
               request. So we use a matching IE everywhere else too */

            if(unit->firmware_type >= HERMES2_FIRMWARE)
               ie = h2_wpa_ie;
            else
               ie = (const UBYTE *)tag_item->ti_Data;
            CopyMem(ie, unit->wpa_ie, ie[1] + 2);
         }
         else
         {
            unit->wpa_ie[0] = 0;
            unit->wpa_ie[1] = 0;
         }
         break;

      case S2INFO_AuthTypes:
         unit->auth_types = tag_item->ti_Data;
         break;
      }
   }

   return;
}



/****i* prism2.device/SetKey ***********************************************
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
   const UBYTE tx_counter[8] = {0, 0, 0, 0, 0x10, 0, 0, 0};
   UWORD i;
   struct EClockVal eclock;

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
            /* For Hermes, load parameters for hardware encryption. The
               pairwise key is treated differently from group keys on
               Hermes-II, but not on Hermes-I */

            if(unit->firmware_type >= HERMES2_FIRMWARE && index == 0)
            {
               P2Seek(unit, 1, P2_REC_ADDMAPPEDTKIPKEY, 0, base);
               unit->LEWordOut(unit->card, P2_REG_DATA1, 28);
               unit->LEWordOut(unit->card, P2_REG_DATA1,
                  P2_REC_ADDMAPPEDTKIPKEY);
               unit->WordsOut(unit->card, P2_REG_DATA1,
                  (UWORD *)unit->bssid, ETH_ADDRESSSIZE / 2);
               unit->WordsOut(unit->card, P2_REG_DATA1, (UWORD *)key,
                  16 / 2);
               unit->WordsOut(unit->card, P2_REG_DATA1, (UWORD *)tx_counter,
                  8 / 2);
               unit->WordsOut(unit->card, P2_REG_DATA1, (UWORD *)rx_counter,
                  6 / 2);
               unit->LEWordOut(unit->card, P2_REG_DATA1, 0);
               unit->WordsOut(unit->card, P2_REG_DATA1, (UWORD *)key + 16,
                  16 / 2);
               P2DoCmd(unit, P2_CMD_ACCESS | P2_CMDF_WRITE,
                  P2_REC_ADDMAPPEDTKIPKEY, base);
            }
            else
            {
               P2Seek(unit, 1, P2_REC_ADDDEFAULTTKIPKEY, 0, base);
               unit->LEWordOut(unit->card, P2_REG_DATA1, 26);
               unit->LEWordOut(unit->card, P2_REG_DATA1,
                  P2_REC_ADDDEFAULTTKIPKEY);
               if(index == unit->tx_key_no)
                  index |= 0x8000;
               unit->LEWordOut(unit->card, P2_REG_DATA1, index);
               unit->WordsOut(unit->card, P2_REG_DATA1, (UWORD *)rx_counter,
                  6 / 2);
               unit->LEWordOut(unit->card, P2_REG_DATA1, 0);
               unit->WordsOut(unit->card, P2_REG_DATA1, (UWORD *)key,
                  32 / 2);
               unit->WordsOut(unit->card, P2_REG_DATA1, (UWORD *)tx_counter,
                  8 / 2);
               P2DoCmd(unit, P2_CMD_ACCESS | P2_CMDF_WRITE,
                  P2_REC_ADDDEFAULTTKIPKEY, base);
            }
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

   /* Clear TKIP key if necessary */

   if(slot->type == S2ENC_TKIP && type != S2ENC_TKIP)
   {
      if(unit->firmware_type >= HERMES2_FIRMWARE && index == 0)
         P2SetData(unit, P2_REC_REMMAPPEDTKIPKEY, unit->bssid,
            ETH_ADDRESSSIZE, base);
      else if(unit->firmware_type >= LUCENT_FIRMWARE)
         P2SetWord(unit, P2_REC_REMDEFAULTTKIPKEY, index, base);
   }

   /* Update type of key in selected slot */

   slot->type = type;
   Enable();

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
         request->io_Error = error;
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
      request->io_Error = error;
      ReplyMsg((APTR)request);
   }
#endif

   /* Return */

   return;
}



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

BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code))
{
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

static VOID RXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code))
{
   UWORD frame_status, frame_id, ieee_length, frame_control, frame_type,
      frame_subtype, encryption, key_no, buffer_no, old_length;
   struct DevBase *base;
   BOOL is_good;
   LONG frag_no;
   UBYTE *buffer, *p, *frame, *data;

   base = unit->device;

   while((unit->LEWordIn(unit->card, P2_REG_EVENTS) & P2_EVENTF_RX) != 0)
   {
      is_good = TRUE;
      frame_id = unit->LEWordIn(unit->card, P2_REG_RXFID);
      P2Seek(unit, 1, frame_id, P2_FRM_STATUS, base);
      frame_status = unit->LEWordIn(unit->card, P2_REG_DATA1);

      if((frame_status & (P2_FRM_STATUSF_BADCRYPT | P2_FRM_STATUSF_BADCRC))
         == 0)
      {
         /* Read frame descriptor from card */

         P2Seek(unit, 1, frame_id, 0, base);
         unit->WordsIn(unit->card, P2_REG_DATA1,
            (UWORD *)unit->rx_descriptor,
            (unit->ethernet_offset + ETH_HEADERSIZE) / 2);
         frame = unit->rx_descriptor + unit->ethernet_offset;
         ieee_length = BEWord(*(UWORD *)(frame + ETH_PACKET_IEEELEN));
         data = frame + ETH_PACKET_DATA;
         unit->WordsIn(unit->card, P2_REG_DATA1, (UWORD *)data,
            (ieee_length + MIC_SIZE + 1) / 2);
         frame_control =
            LEWord(*(UWORD *)(unit->rx_descriptor + P2_FRM_HEADER));

         /* Get buffer to store fragment in */

         frag_no = LEWord(*(UWORD *)(unit->rx_descriptor + P2_FRM_HEADER
            + WIFI_FRM_SEQCONTROL));
         buffer = GetRXBuffer(unit, frame + ETH_PACKET_SOURCE, frag_no,
            &buffer_no, base);

         /* Get location to put new data */

         if(buffer != NULL)
         {
            if((frag_no & 0xf ) > 0)
               old_length = BEWord(*(UWORD *)(buffer + ETH_PACKET_IEEELEN));
            else
            {
               /* Copy header to new frame */

               CopyMem(frame, buffer, ETH_HEADERSIZE);
               old_length = 0;
            }
            p = buffer + ETH_HEADERSIZE + old_length;

            /* Get encryption type and key index */

            if((frame_control & WIFI_FRM_CONTROLF_WEP) != 0)
            {
               key_no = data[3] >> 6 & 0x3;
               encryption = unit->keys[key_no].type;
            }
            else
               encryption = S2ENC_NONE;

            /* Append fragment to frame, decrypting/checking fragment if
               necessary */

            is_good = unit->fragment_decrypt_functions[encryption](unit,
               unit->rx_descriptor + P2_FRM_HEADER, data, &ieee_length, p,
               base);

            /* Update length in frame being built with current fragment, or
               increment bad frame counter if fragment is bad */

            if(is_good)
            {
               ieee_length += old_length;
               *(UWORD *)(buffer + ETH_PACKET_IEEELEN) =
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

                  data = buffer + ETH_HEADERSIZE;
                  if(encryption == S2ENC_TKIP)
                  {
                     /* Hermes cards don't include MIC in frame length, so
                        we need to grab the MIC from the original RX
                        descriptor here */

                     if(unit->firmware_type >= LUCENT_FIRMWARE)
                     {
                        CopyMem(frame + ETH_PACKET_DATA + ieee_length,
                           data + ieee_length, MIC_SIZE);
                        ieee_length += MIC_SIZE;
                     }

                     /* Check Michael MIC */

#ifndef __mc68000
                     is_good = TKIPDecryptFrame(unit, buffer, data,
                        ieee_length, data, key_no, base);
#endif
                     ieee_length -= MIC_SIZE;
                     *(UWORD *)(buffer + ETH_PACKET_IEEELEN) =
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

                  /* If it's a management frame, process it internally;
                     otherwise distribute it to clients after filtering */

                  if(frame_type == WIFI_FRMTYPE_MGMT)
                  {
                     if(frame_subtype == 0x5)
                     {
                        CopyMem(unit->rx_descriptor + P2_FRM_HEADER
                           + WIFI_FRM_ADDRESS3, buffer + ETH_PACKET_SOURCE,
                           ETH_ADDRESSSIZE);
                        SaveBeacon(unit, buffer, base);
                     }
                  }
                  else if(AddressFilter(unit, buffer + ETH_PACKET_DEST,
                     base))
                  {
                     unit->stats.PacketsReceived++;
                        DistributeRXPacket(unit, buffer, base);
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

      /* Discard packet */

      unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, P2_EVENTF_RX);
   }

   /* Re-enable RX interrupts */

   Disable();
   unit->LEWordOut(unit->card, P2_REG_INTMASK,
      unit->LEWordIn(unit->card, P2_REG_INTMASK) | P2_EVENTF_RX);
   Enable();

   return;
}



/****i* prism2.device/GetRXBuffer ******************************************
*
*   NAME
*	GetRXBuffer -- Find an appropriate RX buffer to use.
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

   buffer = unit->rx_buffers;
   for(i = 0, found = FALSE; i < RX_BUFFER_COUNT * 2 && !found; i++)
   {
      /* Throw away old buffer contents if we didn't find a free slot the
         first time around */

      if(i >= RX_BUFFER_COUNT)
         unit->rx_fragment_nos[i % RX_BUFFER_COUNT] = -1;

      /* For a frame's first fragment, find an empty slot; for subsequent
         fragments, find a slot with matching source address */

      n = unit->rx_fragment_nos[i % RX_BUFFER_COUNT];
      if(n == -1 && (frag_no & 0xf) == 0
         || *((ULONG *)(buffer + ETH_PACKET_SOURCE))
         == *((ULONG *)(address))
         && *((UWORD *)(buffer + ETH_PACKET_SOURCE + 4))
         == *((UWORD *)(address + 4)))
      {
         found = TRUE;
         if(n == -1)
            unit->rx_fragment_nos[i % RX_BUFFER_COUNT] = frag_no;
         *buffer_no = i;
      }
      else
         buffer += FRAME_BUFFER_SIZE;
   }

   if(!found)
      buffer = NULL;

   return buffer;
}



/****i* prism2.device/DistributeRXPacket ***********************************
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

static VOID DistributeRXPacket(struct DevUnit *unit, UBYTE *frame,
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

   ieee_length = BEWord(*(UWORD *)(frame + ETH_PACKET_IEEELEN));
   packet_size = ETH_HEADERSIZE + ieee_length;
   if(ieee_length >= SNAP_HEADERSIZE)
      is_snap = *(const ULONG *)(frame + ETH_PACKET_DATA)
         == *(const ULONG *)template;

   /* De-encapsulate SNAP packets and get packet type */

   if(is_snap)
   {
      buffer = unit->rx_buffer;
      packet_size -= SNAP_HEADERSIZE;
      CopyMem(frame, buffer, ETH_PACKET_TYPE);
      CopyMem(frame + ETH_HEADERSIZE + SNAP_FRM_TYPE,
         buffer + ETH_PACKET_TYPE, packet_size - ETH_PACKET_TYPE);
   }
   else
      buffer = frame;

   packet_type = BEWord(*((UWORD *)(buffer + ETH_PACKET_TYPE)));

   if(packet_size <= ETH_MAXPACKETSIZE)
   {
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
   }
   else
      unit->stats.BadData++;

   return;
}



/****i* prism2.device/CopyPacket *******************************************
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
      packet_size += 4;   /* Needed for Shapeshifter & Fusion */
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



/****i* prism2.device/SaveBeacon *******************************************
*
*   NAME
*	SaveBeacon -- Save beacon frame for later examination.
*
*   SYNOPSIS
*	SaveBeacon(unit, frame)
*
*	VOID SaveBeacon(struct DevUnit *, UBYTE *);
*
****************************************************************************
*
*/

static VOID SaveBeacon(struct DevUnit *unit, const UBYTE *frame,
   struct DevBase *base)
{
   UWORD size;

   /* Store frame for later matching with scan results */

   size = ETH_HEADERSIZE + BEWord(*(UWORD *)(frame + ETH_PACKET_IEEELEN));
   if(unit->next_beacon + size < unit->beacons + BEACON_BUFFER_SIZE)
   {
      CopyMem(frame, unit->next_beacon, size);
      unit->beacon_count++;
      unit->next_beacon += size + sizeof(ULONG) & ~3;
   }

   return;
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

static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code))
{
   UWORD i, packet_size, send_size, packet_type, data_size, body_size = 0,
      frame_id, encryption, control_value, subtype;
   UBYTE *buffer, *desc = unit->tx_descriptor, *plaintext, *ciphertext,
      *header, mic_header[ETH_ADDRESSSIZE * 2], *q;
   const UBYTE *p, *dest, *source;
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
      /* Get next request and full packet size */

      request = (APTR)port->mp_MsgList.lh_Head;
      packet_size = request->ios2_DataLength;
      if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
         packet_size += ETH_PACKET_DATA;

      /* Determine encryption type and frame subtype */

      if(packet_size > ETH_HEADERSIZE)
      {
         encryption = unit->keys[unit->tx_key_no].type;
         subtype = 0;
      }
      else
      {
         encryption = S2ENC_NONE;
         subtype = 4;
      }

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
               S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_TX,
               base);
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

         /* Clear frame descriptor as far as start of 802.11 header */

         q = desc;
         for(i = 0; i < P2_FRM_HEADER; i++)
            *q++ = 0;
         header = q;

         /* Set TX control field */

         control_value = P2_FRM_TXCONTROLF_NATIVE;
         if(encryption == S2ENC_TKIP && (unit->flags & UNITF_HARDTKIP) != 0)
            control_value |= unit->tx_key_no << P2_FRM_TXCONTROLB_MICKEYID
               | P2_FRM_TXCONTROLF_MIC;
         else if(encryption == S2ENC_NONE
            && unit->firmware_type < LUCENT_FIRMWARE)
            control_value |= P2_FRM_TXCONTROLF_NOENC;

         *(UWORD *)(desc + unit->txcontrol_offset) =
            MakeLEWord(control_value);

         /* Write 802.11 header */

         *(UWORD *)q = MakeLEWord(
            (encryption == S2ENC_NONE ? 0 : WIFI_FRM_CONTROLF_WEP)
            | (unit->mode == S2PORT_ADHOC ? 0 : WIFI_FRM_CONTROLF_TODS)
            | subtype << WIFI_FRM_CONTROLB_SUBTYPE
            | WIFI_FRMTYPE_DATA << WIFI_FRM_CONTROLB_TYPE);
         q += 2;

         *(UWORD *)q = 0;
         q += 2;

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
         *(UWORD *)q = 0;

         /* Clear 802.3 header */

         q = desc + unit->ethernet_offset;
         for(i = 0; i < ETH_HEADERSIZE; i++)
            *q++ = 0;

         /* Leave room for encryption overhead */

         q = desc + unit->data_offset;
         ciphertext = q;
         q += unit->iv_sizes[encryption];
         plaintext = q;

         /* Write SNAP header */

         if(!is_ieee)
         {
            for(i = 0, p = snap_template; i < SNAP_FRM_TYPE; i++)
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

         unit->fragment_encrypt_functions[encryption](unit, header,
            plaintext, &body_size, ciphertext, base);

         /* Calculate total length of data to send to adapter */

         send_size = unit->data_offset + body_size;

         /* Fill in length field, adjusting for Hermes peculiarities */

         if(unit->firmware_type >= LUCENT_FIRMWARE
            && encryption == S2ENC_TKIP)
            body_size -= MIC_SIZE;

         if(unit->firmware_type == LUCENT_FIRMWARE
            && (unit->flags & UNITF_HARDTKIP) != 0)
            *(UWORD *)(desc + unit->ethernet_offset + ETH_PACKET_IEEELEN) =
               MakeBEWord(body_size);
         else
            *(UWORD *)(desc + unit->datalen_offset) = MakeLEWord(body_size);

         /* Write packet to adapter and send */

         frame_id = unit->tx_frame_id;
         P2Seek(unit, 0, frame_id, 0, base);
         unit->WordsOut(unit->card, P2_REG_DATA0, (UWORD *)desc,
            (send_size + 1) / 2);
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

   return;
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



/****i* prism2.device/InfoInt **********************************************
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

static VOID InfoInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code))
{
   struct DevBase *base;
   UWORD id, length, rec_length, status, ies_length, data_length,
      ssid_length, *ap_rec, *rec, count, i;
   UBYTE *ie, *ssid, *descriptor, *frame, *data, *bssid = unit->bssid;
   BOOL associated, is_duplicate = FALSE;

   base = unit->device;
   id = unit->LEWordIn(unit->card, P2_REG_INFOFID);

   P2Seek(unit, 1, id, 0, base);
   length = (unit->LEWordIn(unit->card, P2_REG_DATA1) + 1) * 2;

   switch(unit->LEWordIn(unit->card, P2_REG_DATA1))
   {
   case P2_INFO_COUNTERS:

      /* Read useful stats and skip others */

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

      break;

   case P2_INFO_SCANRESULTS:
   case P2_INFO_HOSTSCANRESULTS:

      P2ReadRec(unit, id, unit->scan_results_rec, SCAN_BUFFER_SIZE, base);
      Signal(unit->task, unit->scan_complete_signal);
      break;

   case P2_INFO_SCANRESULT:

      descriptor = unit->rx_descriptor;
      P2ReadRec(unit, id, descriptor, FRAME_BUFFER_SIZE, base);
      if(length > 4)
      {
         /* Save IEEE 802.3 portion of scan result */

         frame = descriptor + unit->ethernet_offset;
         data = frame + ETH_PACKET_DATA;
         CopyMem(descriptor + P2_FRM_HEADER + WIFI_FRM_ADDRESS3,
            frame + ETH_PACKET_SOURCE, ETH_ADDRESSSIZE);
         data_length =
            LEWord(*(UWORD *)(descriptor + unit->datalen_offset));
         ies_length = data_length - WIFI_BEACON_IES;
         *(UWORD *)(frame + ETH_PACKET_IEEELEN) = MakeBEWord(data_length);

         /* Append a fake old-style scan record on to fake record list */

         rec_length = LEWord(unit->scan_results_rec[0]);
         ap_rec = unit->scan_results_rec + 1 + rec_length;

         /* Check for duplicate scan results */

         rec = unit->scan_results_rec + 2;
         count = (rec_length - 1) * 2 / P2_APRECLEN;
         for(i = 0; i < count && !is_duplicate; i++, rec += P2_APRECLEN / 2)
            if(CompareMACAddresses(frame + ETH_PACKET_SOURCE,
               rec + P2_APREC_BSSID / 2))
               is_duplicate = TRUE;

         /* Only add new record if there's space and its BSSID hasn't
            already been seen */

         if(2 + rec_length * 2 + P2_APRECLEN < SCAN_BUFFER_SIZE
            && !is_duplicate)
         {
            SaveBeacon(unit, frame, base);

            CopyMem(frame + ETH_PACKET_SOURCE, ap_rec + P2_APREC_BSSID / 2,
               ETH_ADDRESSSIZE);

            ap_rec[P2_APREC_SIGNAL / 2] =
               MakeLEWord(descriptor[P2_FRM_SIGNAL]);

            ap_rec[P2_APREC_NOISE / 2] =
               MakeLEWord(descriptor[P2_FRM_NOISE]);

            ap_rec[P2_APREC_CHANNEL / 2] = MakeLEWord(GetIE(WIFI_IE_CHANNEL,
               data + WIFI_BEACON_IES, ies_length, base)[2]);

            ap_rec[P2_APREC_INTERVAL / 2] =
               *(UWORD *)(data + WIFI_BEACON_INTERVAL);

            ap_rec[P2_APREC_CAPABILITIES / 2] =
               *(UWORD *)(data + WIFI_BEACON_CAPABILITIES);

            ie = GetIE(WIFI_IE_SSID, data + WIFI_BEACON_IES, ies_length,
               base);
            ssid_length = ie[1];
            ssid = ie + 2;
            ap_rec[P2_APREC_NAMELEN / 2] = MakeLEWord(ssid_length);
            CopyMem(ssid, ap_rec + P2_APREC_NAME / 2, ssid_length);

            unit->scan_results_rec[0] =
               MakeLEWord(rec_length + P2_APRECLEN / 2);
         }
      }
      else
         Signal(unit->task, unit->scan_complete_signal);
      break;

   case P2_INFO_LINKSTATUS:

      /* Only report an event if association status has really changed */

      status = unit->LEWordIn(unit->card, P2_REG_DATA1);

      if(status == 1 || unit->firmware_type < LUCENT_FIRMWARE
         && status == 3)
         associated = TRUE;
      else
         associated = FALSE;

      if(!(*(ULONG *)bssid == 0 && *(UWORD *)(bssid + 4) == 0))
      {
         if(associated && (unit->flags & UNITF_ASSOCIATED) == 0)
         {
            unit->flags |= UNITF_ASSOCIATED;
            ReportEvents(unit, S2EVENT_CONNECT, base);
         }
         else if(!associated && (unit->flags & UNITF_ASSOCIATED) != 0)
         {
            unit->flags &= ~UNITF_ASSOCIATED;
            ReportEvents(unit, S2EVENT_DISCONNECT, base);
         }
      }
   }

   /* Acknowledge event and re-enable info interrupts */

   unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, P2_EVENTF_INFO);
   Disable();
   if((unit->flags & UNITF_ONLINE) != 0)
      unit->LEWordOut(unit->card, P2_REG_INTMASK,
         unit->LEWordIn(unit->card, P2_REG_INTMASK) | P2_EVENTF_INFO);
   Enable();

   return;
}



/****i* prism2.device/ResetHandler *****************************************
*
*   NAME
*	ResetHandler -- Disable hardware before a reboot.
*
*   SYNOPSIS
*	ResetHandler(unit, int_code)
*
*	VOID ResetHandler(struct DevUnit *, APTR);
*
****************************************************************************
*
*/

static VOID ResetHandler(REG(a1, struct DevUnit *unit),
   REG(a6, APTR int_code))
{
   if((unit->flags & UNITF_HAVEADAPTER) != 0)
   {
      /* Stop interrupts */

      unit->LEWordOut(unit->card, P2_REG_INTMASK, 0);

      /* Stop transmission and reception */

      unit->LEWordOut(unit->card, P2_REG_PARAM0, 0);
      unit->LEWordOut(unit->card, P2_REG_COMMAND, P2_CMD_DISABLE);
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



/****i* prism2.device/SendScanResults **************************************
*
*   NAME
*	SendScanResults -- Reply to all outstanding scan requests.
*
*   SYNOPSIS
*	SendScanResults(unit)
*
*	VOID SendScanResults(struct DevUnit *);
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

static VOID SendScanResults(struct DevUnit *unit, struct DevBase *base)
{
   BYTE error = 0;
   struct IOSana2Req *request, *tail, *next_request;
   struct List *list;
   APTR pool;
   UWORD count, i, j, ssid_length, length, entry_length, *ap_rec,
      data_length, frame_length, ies_length;
   struct TagItem **tag_lists, *tag;
   UBYTE *bssid, *ies;
   const UBYTE *beacon, *ie_bssid;
   TEXT *ssid;

   list = &unit->request_ports[SCAN_QUEUE]->mp_MsgList;
   next_request = (APTR)list->lh_Head;
   tail = (APTR)&list->lh_Tail;

   while(next_request != tail)
   {
      request = next_request;
      next_request = (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

      pool = request->ios2_Data;

      length = LEWord(unit->scan_results_rec[0]);
      ap_rec = unit->scan_results_rec + 2;

      if(unit->firmware_type == INTERSIL_FIRMWARE)
      {
         entry_length = LEWord(unit->scan_results_rec[2]);
         ap_rec += 2;
         count = (length - 3) * 2 / entry_length;
      }
      else
      {
         entry_length = P2_APRECLEN;
         count = (length - 1) * 2 / entry_length;
      }

      /* Allocate array of tag lists, one for each AP */

      if(count > 0)
      {
         tag_lists = AllocPooled(pool, count * sizeof(APTR));
         if(tag_lists == NULL)
            error = S2ERR_NO_RESOURCES;
      }
      else
         tag_lists = NULL;

      for(i = 0; i < count && error == 0; i++, ap_rec += entry_length / 2)
      {
         tag_lists[i] =
            AllocPooled(pool, SCAN_TAG_COUNT * sizeof(struct TagItem));
         if(tag_lists[i] == NULL)
            error = S2ERR_NO_RESOURCES;

         if(error == 0)
         {
            tag = tag_lists[i];

            tag->ti_Tag = S2INFO_BSSID;
            tag->ti_Data = (UPINT)(bssid =
               AllocPooled(pool, ETH_ADDRESSSIZE));
            if(bssid != NULL)
               CopyMem(ap_rec + P2_APREC_BSSID / 2, bssid,
                  ETH_ADDRESSSIZE);
            else
               error = S2ERR_NO_RESOURCES;
            tag++;

            tag->ti_Tag = TAG_IGNORE;
            tag++;

            tag->ti_Tag = S2INFO_Channel;
            tag->ti_Data = LEWord(ap_rec[P2_APREC_CHANNEL / 2]);
            tag++;

            tag->ti_Tag = S2INFO_BeaconInterval;
            tag->ti_Data = LEWord(ap_rec[P2_APREC_INTERVAL / 2]);
            tag++;

            tag->ti_Tag = S2INFO_Capabilities;
            tag->ti_Data = LEWord(ap_rec[P2_APREC_CAPABILITIES / 2]);
            tag++;

            tag->ti_Tag = S2INFO_Signal;
            tag->ti_Data = ConvertScanLevel(unit,
               LEWord(ap_rec[P2_APREC_SIGNAL / 2]), base);
            tag++;

            tag->ti_Tag = S2INFO_Noise;
            tag->ti_Data = ConvertScanLevel(unit,
               LEWord(ap_rec[P2_APREC_NOISE / 2]), base);
            tag++;


            ssid_length = LEWord(ap_rec[P2_APREC_NAMELEN / 2]);
            tag->ti_Tag = S2INFO_SSID;
            tag->ti_Data = (UPINT)(ssid =
               AllocPooled(pool, 31 + 1));
            if(ssid != NULL)
            {
               CopyMem(ap_rec + P2_APREC_NAME / 2, ssid, ssid_length);
               ssid[ssid_length] = '\0';
            }
            else
               error = S2ERR_NO_RESOURCES;
            tag++;

            tag->ti_Tag = TAG_END;
         }
      }

      /* Find IEs for each BSS and insert them into the BSS's tag list */

      for(beacon = unit->beacons, i = 0; i < unit->beacon_count; i++)
      {
         /* Extract IEs from beacon descriptor */

         data_length = BEWord(*(UWORD *)(beacon + ETH_PACKET_IEEELEN));
         ies_length = data_length - 12;
         frame_length = ETH_HEADERSIZE + data_length;
         ies = AllocPooled(pool, sizeof(UWORD) + ies_length);
         if(ies != NULL)
         {
            *(UWORD *)ies = ies_length;
            CopyMem(beacon + ETH_PACKET_DATA + WIFI_BEACON_IES,
               ies + sizeof(UWORD), ies_length);
         }
         else
            error = S2ERR_NO_RESOURCES;

         /* Find matching tag list and add IEs to it */

         ie_bssid = beacon + ETH_PACKET_SOURCE;
         for(j = 0; j < count; j++)
         {
            tag = tag_lists[j];
            bssid = (UBYTE *)tag->ti_Data;
            if(*(ULONG *)bssid == *(ULONG *)ie_bssid
               && *(UWORD *)(bssid + 4) == *(UWORD *)(ie_bssid + 4))
            {
               tag++;
               tag->ti_Tag = S2INFO_InfoElements;
               tag->ti_Data = (PINT)ies;
            }
         }

         beacon += frame_length + sizeof(ULONG) & ~3;
      }

      /* Return results */

      if(error == 0)
      {
         request->ios2_StatData = tag_lists;
         request->ios2_DataLength = count;
      }
      else
      {
         request->ios2_Req.io_Error = error;
         request->ios2_WireError = S2WERR_GENERIC_ERROR;
      }
      Remove((APTR)request);
      ReplyMsg((APTR)request);
   }

   /* Discard collected beacon frames */

   Disable();
   unit->next_beacon = unit->beacons;
   unit->beacon_count = 0;
   Enable();

   return;
}



/****i* prism2.device/GetNetworkInfo ***************************************
*
*   NAME
*	GetNetworkInfo -- Get information on current network.
*
*   SYNOPSIS
*	tag_list = GetNetworkInfo(unit, pool)
*
*	struct TagItem *GetNetworkInfo(struct DevUnit *, APTR);
*
*   FUNCTION
*
*   INPUTS
*	unit - A unit of this device.
*	pool - A memory pool.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

struct TagItem *GetNetworkInfo(struct DevUnit *unit, APTR pool,
   struct DevBase *base)
{
   BYTE error = 0;
   struct TagItem *tag_list, *tag;
   UBYTE *bssid, *ie;

   tag_list =
      AllocPooled(pool, INFO_TAG_COUNT * sizeof(struct TagItem));
   if(tag_list == NULL)
      error = S2ERR_NO_RESOURCES;

   if(error == 0)
   {
      tag = tag_list;

      tag->ti_Tag = S2INFO_BSSID;
      tag->ti_Data = (UPINT)(bssid =
         AllocPooled(pool, ETH_ADDRESSSIZE));
      if(bssid != NULL)
         CopyMem(unit->bssid, bssid, ETH_ADDRESSSIZE);
      else
         error = S2ERR_NO_RESOURCES;
      tag++;

      tag->ti_Tag = TAG_IGNORE;
      tag++;

      tag->ti_Tag = S2INFO_WPAInfo;
      tag->ti_Data = (UPINT)(ie =
         AllocPooled(pool, unit->wpa_ie[1] + 2));
      if(ie != NULL)
         CopyMem(unit->wpa_ie, ie, unit->wpa_ie[1] + 2);
      else
         error = S2ERR_NO_RESOURCES;
      tag++;

      tag->ti_Tag = TAG_END;
   }

   if(error != 0)
      tag_list = NULL;

   return tag_list;
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
   P2DoCmd(unit, P2_CMD_ACCESS, P2_REC_LINKQUALITY, base);
   P2Seek(unit, 1, P2_REC_LINKQUALITY, 6, base);

   unit->signal_quality.SignalLevel =
      ConvertLevel(unit, unit->LEWordIn(unit->card, P2_REG_DATA1), base);
   unit->signal_quality.NoiseLevel =
      ConvertLevel(unit, unit->LEWordIn(unit->card, P2_REG_DATA1), base);

   return;
}



/****i* prism2.device/StartScan ********************************************
*
*   NAME
*	StartScan -- Start a scan for available networks.
*
*   SYNOPSIS
*	StartScan(unit)
*
*	VOID StartScan(struct DevUnit *);
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

VOID StartScan(struct DevUnit *unit, const TEXT *ssid, struct DevBase *base)
{
   UBYTE *params;
   UWORD ssid_length = 0;

   /* Ask for a scan */

   if((unit->flags & UNITF_ONLINE) != 0)
   {
      if(ssid != NULL)
         ssid_length = StrLen(ssid);
      if(unit->firmware_type == INTERSIL_FIRMWARE)
      {
         params = AllocVec(sizeof(scan_params) + ssid_length, MEMF_PUBLIC);
         if(params != NULL)
         {
            CopyMem(scan_params, params, sizeof(scan_params));
            if(ssid != NULL)
               CopyMem(ssid, params + sizeof(scan_params), ssid_length);
            params[4] = ssid_length;
            P2SetData(unit, P2_REC_HOSTSCAN, params,
               sizeof(scan_params) + ssid_length, base);
            FreeVec(params);
         }
      }
      else if(unit->firmware_type == SYMBOL_FIRMWARE)
         P2SetWord(unit, P2_REC_ALTHOSTSCAN, 0x82, base);
      else if(unit->firmware_type >= LUCENT_FIRMWARE
         && (unit->flags & UNITF_HARDTKIP) != 0)
      {
         /* Initialise fake scan results and ask for a series of raw beacon
            descriptors */

         unit->scan_results_rec[0] = MakeLEWord(1);

         P2SetID(unit, P2_REC_SCANSSID, ssid, ssid_length, base);
         P2SetWord(unit, P2_REC_SCANCHANNELS, 0x7fff, base);
         unit->LEWordOut(unit->card, P2_REG_PARAM1, 0x3fff);
         P2DoCmd(unit, P2_CMD_INQUIRE, P2_INFO_SCANRESULT, base);
      }
      else
      {
         P2SetID(unit, P2_REC_SCANSSID, ssid, ssid_length, base);
         P2DoCmd(unit, P2_CMD_INQUIRE, P2_INFO_SCANRESULTS, base);
      }
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
   const TEXT *file_name;
   struct FileInfoBlock *info = NULL;
   UWORD control_reg, pdr_no, *pda = NULL, *pdr, *prod_data, length;
   ULONG location, start_address;
   BPTR file = (BPTR)NULL;
   UBYTE *data = NULL;
   TEXT *buffer = NULL;
   const TEXT *p;
   UBYTE type;

   /* Read firmware file */

   switch(unit->firmware_type)
   {
   case LUCENT_FIRMWARE:
      file_name = h1_firmware_file_name;
      break;
   case HERMES2_FIRMWARE:
      file_name = h2_firmware_file_name;
      break;
   case HERMES2G_FIRMWARE:
      file_name = h25_firmware_file_name;
      break;
   default:
      file_name = NULL;
   }

   if(file_name == NULL)
      success = FALSE;

   if(success)
   {
      file = Open(file_name, MODE_OLDFILE);
      if(file == (BPTR)NULL)
         success = FALSE;
   }

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
      buffer = AllocVec(info->fib_Size + 1, MEMF_ANY);
      data = AllocVec(MAX_S_REC_SIZE, MEMF_ANY);
      pda = AllocVec(LUCENT_PDA_SIZE, MEMF_ANY);
      if(buffer == NULL || data == NULL || pda == NULL)
         success = FALSE;
   }

   if(success)
   {
      if(Read(file, buffer, info->fib_Size) == -1)
         success = FALSE;
      buffer[info->fib_Size] = '\0';
   }

   if(success)
   {
      unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, 0xffff);
      P2DoCmd(unit, P2_CMD_INIT | 0x100, 0, base);
      unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, 0xffff);
      P2DoCmd(unit, P2_CMD_INIT | 0x0, 0, base);
      unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, 0xffff);

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

      /* Read Production Data Area from card */

      if(unit->firmware_type < HERMES2_FIRMWARE)
      {
         location = LUCENT_PDA_ADDRESS;
         unit->LEWordOut(unit->card, P2_REG_AUXPAGE, location >> 7);
         unit->LEWordOut(unit->card, P2_REG_AUXOFFSET,
            location & (1 << 7) - 1);

         unit->WordsIn(unit->card, P2_REG_AUXDATA,
            (UWORD *)pda, LUCENT_PDA_SIZE >> 1);
      }

      /* Allow writing to card's RAM */

      BusyMilliDelay(100, base);
      start_address = 0xf8000;
      unit->LEWordOut(unit->card, P2_REG_PARAM1, start_address >> 16);
      P2DoCmd(unit, P2_CMD_PROGRAM | P2_CMDF_WRITE, start_address, base);

      /* Parse firmware image data and write it to card */

      p = buffer;
      while(p != NULL)
      {
         p = ParseNextSRecord(p, &type, data, &length, &location, base);
         if(p != NULL)
         {
            switch(type)
            {
            case '3':

               /* Check that this is not a "special" record */

               if(location < 0xff000000)
               {
                  /* Write data to card */

                  if(unit->firmware_type < LUCENT_FIRMWARE)
                  {
                     unit->LEWordOut(unit->card, P2_REG_PARAM1,
                        location >> 16);
                     P2DoCmd(unit, P2_CMD_PROGRAM | P2_CMDF_WRITE, location,
                        base);
                  }

                  unit->LEWordOut(unit->card, P2_REG_AUXPAGE,
                     location >> 7);
                  unit->LEWordOut(unit->card, P2_REG_AUXOFFSET,
                     location & (1 << 7) - 1);

                  unit->WordsOut(unit->card, P2_REG_AUXDATA,
                     (UWORD *)data, length >> 1);
                  }
               break;

            case '7':

               /* Get location in card memory to begin execution of new
                  firmware at */

               start_address = location;
            }
         }
      }

      /* Parse PDA plug records and patch firmware */

      p = buffer;
      while(p != NULL)
      {
         p = ParseNextSRecord(p, &type, data, &length, &location, base);

         if(p != NULL && type == '3' && location == 0xff000000)
         {
            /* Get PDR number and the location where it should be patched
               into firmware */

            pdr_no = LELong(*(ULONG *)data);
            location = LELong(*(ULONG *)(data + 4));
            length = LELong(*(ULONG *)(data + 8));

            /* Find PDR to copy data from */

            prod_data = NULL;
            for(pdr = pda; pdr[1] != 0; pdr += LEWord(pdr[0]) + 1)
            {
               if(LEWord(pdr[1]) == pdr_no)
                  prod_data = pdr + 2;
            }

            /* Write production data to card if it was found */

            if(prod_data != NULL)
            {
               if(unit->firmware_type < LUCENT_FIRMWARE)
               {
                  unit->LEWordOut(unit->card, P2_REG_PARAM1,
                     location >> 16);
                  P2DoCmd(unit, P2_CMD_PROGRAM | P2_CMDF_WRITE, location,
                     base);
               }

               unit->LEWordOut(unit->card, P2_REG_AUXPAGE, location >> 7);
               unit->LEWordOut(unit->card, P2_REG_AUXOFFSET,
                  location & (1 << 7) - 1);

               unit->WordsOut(unit->card, P2_REG_AUXDATA, prod_data,
                  length >> 1);
            }
         }
      }

      /* Disable auxiliary ports */

      control_reg = unit->LEWordIn(unit->card, P2_REG_CONTROL);
      control_reg =
         control_reg & ~P2_REG_CONTROLF_AUX | P2_REG_CONTROL_DISABLEAUX;
      unit->LEWordOut(unit->card, P2_REG_CONTROL, control_reg);

      /* Execute downloaded firmware */

      if(unit->firmware_type >= HERMES2_FIRMWARE)
      {
         unit->LEWordOut(unit->card, P2_REG_PARAM1, start_address >> 16);
         P2DoCmd(unit, P2_CMD_EXECUTE, start_address, base);
      }
      else if(unit->firmware_type >= LUCENT_FIRMWARE)
      {
         P2DoCmd(unit, P2_CMD_PROGRAM, 0, base);
         unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, 0xffff);
         P2DoCmd(unit, P2_CMD_INIT, 0, base);
         unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, 0xffff);
      }
      else
      {
         unit->LEWordOut(unit->card, P2_REG_PARAM1, start_address >> 16);
         P2DoCmd(unit, P2_CMD_PROGRAM, start_address, base);
         P2DoCmd(unit, P2_CMD_INIT, 0, base);
      }
   }

   /* Free Resources */

   FreeVec(pda);
   FreeVec(buffer);
   FreeVec(data);
   FreeDosObject(DOS_FIB, info);
   if(file != (BPTR)NULL)
      Close(file);

   return success;
}



/****i* prism2.device/ParseNextSRecord *************************************
*
*   NAME
*	ParseNextSRecord
*
****************************************************************************
*
*/

static const TEXT *ParseNextSRecord(const TEXT *s, UBYTE *type, UBYTE *data,
   UWORD *data_length, ULONG *location, struct DevBase *base)
{
   LONG ch;
   ULONG n = 0;
   UWORD i;
   BOOL found = FALSE;

   /* Find start of next record, if any */

   while(!found)
   {
      ch = *s++;
      if(ch == 'S' || ch == '\0')
         found = TRUE;
   }

   if(ch == 'S')
   {
      /* Get record type */

      *type = *s++;

      /* Skip length field to keep alignment easy */

      s += 2;

      /* Parse hexadecimal portion of record */

      for(i = 0; (ch = *s++) >= '0'; i++)
      {
         n <<= 4;

         if(ch >= 'A')
            n |= ch - 'A' + 10;
         else
            n |= ch - '0';

         if(i >= 8 && (i & 0x1) != 0)
         {
            *data++ = n;
            n = 0;
         }
         else if(i == 7)
         {
            *location = n;
            n = 0;
         }
      }
      *data_length = (i >> 1) - 5;
   }
   else
      s = NULL;

   /* Return updated text pointer */

   return s;
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
   if(unit->firmware_type < LUCENT_FIRMWARE && command == P2_CMD_INIT)
      unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, 0xffff);

   unit->LEWordOut(unit->card, P2_REG_PARAM0, param);
   unit->LEWordOut(unit->card, P2_REG_COMMAND, command);
   while((unit->LEWordIn(unit->card, P2_REG_EVENTS)
      & P2_EVENTF_CMD) == 0);
   unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, P2_EVENTF_CMD);

   if(unit->firmware_type < LUCENT_FIRMWARE && command == P2_CMD_INIT)
      unit->LEWordOut(unit->card, P2_REG_ACKEVENTS, 0xffff);
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
   while((unit->LEWordIn(unit->card, offset_reg) & P2_REG_OFFSETF_BUSY)
      != 0);
   unit->LEWordOut(unit->card, P2_REG_SELECT0 + path_no, rec_no);
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
*   NOTES
*	id may be NULL as long as length is zero.
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
   unit->LEWordOut(unit->card, P2_REG_DATA1, length);
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



/****i* prism2.device/P2ReadRec ********************************************
*
*   NAME
*	P2ReadRec -- Load and read an entire record.
*
*   SYNOPSIS
*	success = P2ReadRec(unit, rec_no, buffer, max_length)
*
*	BOOL P2ReadRec(struct DevUnit *, UWORD, APTR, UWORD);
*
*   FUNCTION
*
*   INPUTS
*	unit - A unit of this device.
*	rec_no - Record number to read.
*	buffer - Buffer to store data in.
*	max_length - Maximum number of bytes to store in buffer.
*
*   RESULT
*	success - Success indicator.
*
****************************************************************************
*
*/

static BOOL P2ReadRec(struct DevUnit *unit, UWORD rec_no, APTR buffer,
   UWORD max_length, struct DevBase *base)
{
   BOOL success = TRUE;
   WORD length;

   P2DoCmd(unit, P2_CMD_ACCESS, rec_no, base);
   P2Seek(unit, 1, rec_no, 0, base);

   length = (unit->LEWordIn(unit->card, P2_REG_DATA1) + 1) * 2;
   P2Seek(unit, 1, rec_no, 0, base);
   if(length <= max_length)
      unit->WordsIn(unit->card, P2_REG_DATA1, (UWORD *)buffer,
         length / 2);
   else
      success = FALSE;
   return success;
}



/****i* prism2.device/ConvertLevel *****************************************
*
*   NAME
*	ConvertLevel -- Convert a signal or noise level to dBm.
*
*   SYNOPSIS
*	dbm_level = ConvertLevel(unit, raw_level)
*
*	LONG ConvertLevel(struct DevUnit *, UWORD);
*
*   FUNCTION
*
*   INPUTS
*	unit - A unit of this device.
*	raw_level - The value returned from the hardware.
*
*   RESULT
*	dbm_level - The value in dBm.
*
****************************************************************************
*
*/

static LONG ConvertLevel(struct DevUnit *unit, UWORD raw_level,
   struct DevBase *base)
{
   LONG dbm_level;

   if(unit->firmware_type >= LUCENT_FIRMWARE)
      dbm_level = raw_level - LUCENT_DBM_OFFSET;
   else
      dbm_level = raw_level / 3 - INTERSIL_DBM_OFFSET;

   return dbm_level;
}



/****i* prism2.device/ConvertScanLevel *************************************
*
*   NAME
*	ConvertScanLevel -- Convert a signal or noise level to dBm.
*
*   SYNOPSIS
*	dbm_level = ConvertScanLevel(unit, raw_level)
*
*	LONG ConvertScanLevel(struct DevUnit *, UWORD);
*
*   FUNCTION
*
*   INPUTS
*	unit - A unit of this device.
*	raw_level - The value returned from the hardware.
*
*   RESULT
*	dbm_level - The value in dBm.
*
****************************************************************************
*
*/

static LONG ConvertScanLevel(struct DevUnit *unit, UWORD raw_level,
   struct DevBase *base)
{
   LONG dbm_level;

   if(unit->firmware_type >= LUCENT_FIRMWARE)
      dbm_level = raw_level - LUCENT_DBM_OFFSET;
   else
      dbm_level = (WORD)raw_level;

   return dbm_level;
}



/****i* prism2.device/GetIE ************************************************
*
*   NAME
*	GetIE
*
*   SYNOPSIS
*	ie = GetIE(id, ies, ies_length)
*
*	UBYTE *GetIE(UBYTE, UBYTE *, UWORD);
*
*   FUNCTION
*
*   INPUTS
*	id - ID of IE to find.
*	ies - A series of IEs.
*	ies_length - Length of IE block.
*
*   RESULT
*	ie - Pointer to start of IE within block, or NULL if not found.
*
****************************************************************************
*
*/

static UBYTE *GetIE(UBYTE id, UBYTE *ies, UWORD ies_length,
   struct DevBase *base)
{
   UBYTE *ie;

   for(ie = ies; ie < ies + ies_length && ie[0] != id; ie += ie[1] + 2);
   if(ie >= ies + ies_length)
      ie = NULL;

   return ie;
}



/****i* prism2.device/CompareMACAddresses **********************************
*
*   NAME
*	CompareMACAddresses -- Compare two MAC addresses for equality.
*
*   SYNOPSIS
*	same = CompareMACAddresses(mac1, mac2)
*
*	UBYTE *CompareMACAddresses(UBYTE *, UBYTE *);
*
*   INPUTS
*	mac1 - first MAC address.
*	mac2 - second MAC address.
*
*   RESULT
*	same - TRUE if MAC addresses are equal.
*
****************************************************************************
*
*/

static BOOL CompareMACAddresses(APTR mac1, APTR mac2)
{
   return *(ULONG *)mac1 == *(ULONG *)mac2
      && *((UWORD *)mac1 + 2) == *((UWORD *)mac2 + 2);
}



/****i* prism2.device/UnitTask *********************************************
*
*   NAME
*	UnitTask
*
*   SYNOPSIS
*	UnitTask(unit)
*
*	VOID UnitTask(struct DevUnit *);
*
*   FUNCTION
*	Completes deferred requests, and handles card insertion and removal
*	in conjunction with the relevant interrupts.
*
****************************************************************************
*
*/

static VOID UnitTask(struct DevUnit *unit)
{
   struct DevBase *base;
   struct IORequest *request;
   struct MsgPort *general_port;
   ULONG signals = 0, wait_signals, card_removed_signal,
      card_inserted_signal, scan_complete_signal, general_port_signal;

   base = unit->device;

   /* Activate general request port */

   general_port = unit->request_ports[GENERAL_QUEUE];
   general_port->mp_SigTask = unit->task;
   general_port->mp_SigBit = AllocSignal(-1);
   general_port_signal = 1 << general_port->mp_SigBit;
   general_port->mp_Flags = PA_SIGNAL;

   /* Allocate signals for notification of card removal and insertion */

   card_removed_signal = unit->card_removed_signal = 1 << AllocSignal(-1);
   card_inserted_signal = unit->card_inserted_signal = 1 << AllocSignal(-1);
   scan_complete_signal = unit->scan_complete_signal = 1 << AllocSignal(-1);
   wait_signals = (1 << general_port->mp_SigBit) | card_removed_signal
      | card_inserted_signal | scan_complete_signal | SIGBREAKF_CTRL_C;

   /* Tell ourselves to check port for old messages */

   Signal(unit->task, general_port_signal);

   /* Infinite loop to service requests and signals */

   while((signals & SIGBREAKF_CTRL_C) == 0)
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

      if((signals & scan_complete_signal) != 0)
         SendScanResults(unit, base);

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



