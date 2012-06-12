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
#ifndef __amigaos4__
#include <proto/alib.h>
#else
#include <clib/alib_protos.h>
#endif
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/timer.h>

#include "device.h"

#include "unit_protos.h"
#include "request_protos.h"
#include "encryption_protos.h"
#include "hal/ah.h"
#include "hal/ah_desc.h"


#define TASK_PRIORITY 0
#define STACK_SIZE 4096
#define INT_MASK \
   (HAL_INT_GLOBAL | HAL_INT_TX | HAL_INT_TXDESC | HAL_INT_RX \
   | HAL_INT_RXEOL)
#define TX_POWER (20 << 1)
#define TX_TRIES 11
#define G_MGMT_RATE 2000
#define B_MGMT_RATE 1000
#define FRAME_BUFFER_SIZE (WIFI_FRM_DATA + SNAP_HEADERSIZE \
   + 2 * ETH_MTU + EIV_SIZE + ICV_SIZE + MIC_SIZE + FCS_SIZE + 4)
#define MAX_CHANNEL_COUNT 100

#ifndef AbsExecBase
#define AbsExecBase sys_base
#endif

static struct AddressRange *FindMulticastRange(struct DevUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
   UWORD upper_bound_right, struct DevBase *base);
static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base);
static BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
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
static VOID DistributeMgmtFrame(struct DevUnit *unit, UBYTE *frame,
   UWORD frame_size, struct DevBase *base);
static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
static VOID TXEndInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
static VOID MgmtTXInt(REG(a1, struct DevUnit *unit),
   REG(a6, APTR int_code));
static VOID MgmtTXEndInt(REG(a1, struct DevUnit *unit),
   REG(a6, APTR int_code));
static VOID ResetHandler(REG(a1, struct DevUnit *unit),
   REG(a6, APTR int_code));
static VOID ReportEvents(struct DevUnit *unit, ULONG events,
   struct DevBase *base);
static VOID UnitTask(struct ExecBase *sys_base);


static const UBYTE snap_template[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
static const UBYTE broadcast_address[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static const ULONG g_retry_rates[] = {54000, 36000, 18000, 2000};
static const ULONG b_retry_rates[] = {11000, 5500, 2000, 1000};


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
#ifdef __AROS__
#undef AddTask
#define AddTask(task, initial_pc, final_pc) \
   ({ \
      struct TagItem _task_tags[] = \
         {{TASKTAG_ARG1, (IPTR)SysBase}, {TAG_END, 0}}; \
      NewAddTask(task, initial_pc, final_pc, _task_tags); \
   })
#endif



/****i* atheros5000.device/CreateUnit **************************************
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

struct DevUnit *CreateUnit(ULONG index, APTR io_base, UWORD id, APTR card,
   const struct TagItem *io_tags, UWORD bus, struct DevBase *base)
{
   BOOL success = TRUE;
   struct DevUnit *unit;
   struct Task *task;
   struct MsgPort *port;
   UWORD i;
   struct ath_desc *tx_desc, *rx_desc;
   ULONG dma_size;
   APTR stack;
   HAL_STATUS hal_status;

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

      /* Store DMA memory hooks */

      unit->AllocDMAMem =
         (APTR)GetTagData(IOTAG_AllocDMAMem, (UPINT)NULL, io_tags);
      unit->FreeDMAMem =
         (APTR)GetTagData(IOTAG_FreeDMAMem, (UPINT)NULL, io_tags);
      if(unit->AllocDMAMem == NULL || unit->FreeDMAMem == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Initialise HAL */

      unit->hal = ath_hal_attach(id, unit, NULL, io_base, &hal_status);

      if(unit->hal == NULL)
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
         unit->tx_buffers[i] =
            unit->AllocDMAMem(unit->card, FRAME_BUFFER_SIZE, 4);
         if(unit->tx_buffers[i] == NULL)
            success = FALSE;
      }
      for(i = 0; i < MGMT_SLOT_COUNT; i++)
      {
         unit->mgmt_buffers[i] =
            unit->AllocDMAMem(unit->card, FRAME_BUFFER_SIZE, 4);
         if(unit->mgmt_buffers[i] == NULL)
            success = FALSE;
      }
      unit->rx_buffer = AllocVec(FRAME_BUFFER_SIZE, MEMF_PUBLIC);
      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         unit->rx_buffers[i] =
            unit->AllocDMAMem(unit->card, FRAME_BUFFER_SIZE, 4);
         if(unit->rx_buffers[i] == NULL)
            success = FALSE;
      }
      unit->rx_frames =
         AllocVec(FRAME_BUFFER_SIZE * FRAME_BUFFER_COUNT, MEMF_PUBLIC);
      for(i = 0; i < FRAME_BUFFER_COUNT; i++)
         unit->rx_fragment_nos[i] = -1;
      unit->tx_descs = unit->AllocDMAMem(unit->card,
         sizeof(struct ath_desc) * TX_SLOT_COUNT, 4);
      unit->mgmt_descs = unit->AllocDMAMem(unit->card,
         sizeof(struct ath_desc) * MGMT_SLOT_COUNT, 4);
      unit->rx_descs = unit->AllocDMAMem(unit->card,
         sizeof(struct ath_desc) * RX_SLOT_COUNT, 4);
      unit->tx_requests = AllocVec(sizeof(APTR) * TX_SLOT_COUNT,
         MEMF_PUBLIC);
      unit->mgmt_requests = AllocVec(sizeof(APTR) * MGMT_SLOT_COUNT,
         MEMF_PUBLIC);
      unit->channels = AllocVec(sizeof(HAL_CHANNEL) * MAX_CHANNEL_COUNT,
         MEMF_PUBLIC | MEMF_CLEAR);
      if(unit->tx_buffer == NULL
         || unit->rx_buffer == NULL
         || unit->rx_frames == NULL
         || unit->tx_descs == NULL
         || unit->mgmt_descs == NULL
         || unit->rx_descs == NULL
         || unit->tx_requests == NULL
         || unit->mgmt_requests == NULL
         || unit->channels == NULL)
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
      /* Get physical addresses of DMA structures */

      dma_size = sizeof(struct ath_desc) * TX_SLOT_COUNT;
      unit->tx_descs_p =
         (ULONG)(UPINT)CachePreDMA(unit->tx_descs, &dma_size, 0);
      if(dma_size != sizeof(struct ath_desc) * TX_SLOT_COUNT)
         success = FALSE;
      CachePostDMA(unit->tx_descs, &dma_size, 0);

      for(i = 0; i < TX_SLOT_COUNT; i++)
      {
         dma_size = FRAME_BUFFER_SIZE;
         unit->tx_buffers_p[i] =
            (ULONG)(UPINT)CachePreDMA(unit->tx_buffers[i], &dma_size, 0);
         if(dma_size != FRAME_BUFFER_SIZE)
            success = FALSE;
         CachePostDMA(unit->tx_buffers[i], &dma_size, 0);
      }

      dma_size = sizeof(struct ath_desc) * MGMT_SLOT_COUNT;
      unit->mgmt_descs_p =
         (ULONG)(UPINT)CachePreDMA(unit->mgmt_descs, &dma_size, 0);
      if(dma_size != sizeof(struct ath_desc) * MGMT_SLOT_COUNT)
         success = FALSE;
      CachePostDMA(unit->mgmt_descs, &dma_size, 0);

      for(i = 0; i < MGMT_SLOT_COUNT; i++)
      {
         dma_size = FRAME_BUFFER_SIZE;
         unit->mgmt_buffers_p[i] =
            (ULONG)(UPINT)CachePreDMA(unit->mgmt_buffers[i], &dma_size, 0);
         if(dma_size != FRAME_BUFFER_SIZE)
            success = FALSE;
         CachePostDMA(unit->mgmt_buffers[i], &dma_size, 0);
      }

      dma_size = sizeof(struct ath_desc) * RX_SLOT_COUNT;
      unit->rx_descs_p =
         (ULONG)(UPINT)CachePreDMA(unit->rx_descs, &dma_size, 0);
      if(dma_size != sizeof(struct ath_desc) * RX_SLOT_COUNT)
         success = FALSE;
      CachePostDMA(unit->rx_descs, &dma_size, 0);

      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         dma_size = FRAME_BUFFER_SIZE;
         unit->rx_buffers_p[i] =
            (ULONG)(UPINT)CachePreDMA(unit->rx_buffers[i], &dma_size, 0);
         if(dma_size != FRAME_BUFFER_SIZE)
            success = FALSE;
         CachePostDMA(unit->rx_buffers[i], &dma_size, 0);
      }

      /* Construct TX ring */

      for(tx_desc = unit->tx_descs, i = 0; i < TX_SLOT_COUNT; i++)
      {
         tx_desc->ds_data = unit->tx_buffers_p[i];
         tx_desc++;
      }

      /* Construct management frame TX ring */

      for(tx_desc = unit->mgmt_descs, i = 0; i < MGMT_SLOT_COUNT; i++)
      {
         tx_desc->ds_data = unit->mgmt_buffers_p[i];
         tx_desc++;
      }

      /* Construct RX ring */

      for(rx_desc = unit->rx_descs, i = 0; i < RX_SLOT_COUNT; i++)
      {
         rx_desc->ds_link = unit->rx_descs_p + ((i + 1) % RX_SLOT_COUNT)
            * sizeof(struct ath_desc);
         rx_desc->ds_data = unit->rx_buffers_p[i];
         unit->hal->ah_setupRxDesc(unit->hal, rx_desc, FRAME_BUFFER_SIZE,
            HAL_RXDESC_INTREQ);
         rx_desc++;
      }

      dma_size = sizeof(struct ath_desc) * RX_SLOT_COUNT;
      CachePreDMA(unit->rx_descs, &dma_size, 0);

      /* Record maximum speed in BPS */

      unit->speed = 54000000;

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

      unit->tx_end_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->tx_end_int.is_Code = (APTR)TXEndInt;
      unit->tx_end_int.is_Data = unit;

      unit->mgmt_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->mgmt_int.is_Code = (APTR)MgmtTXInt;
      unit->mgmt_int.is_Data = unit;

      unit->mgmt_end_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->mgmt_end_int.is_Code = (APTR)MgmtTXEndInt;
      unit->mgmt_end_int.is_Data = unit;

      unit->reset_handler.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->reset_handler.is_Code = (APTR)ResetHandler;
      unit->reset_handler.is_Data = unit;

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



/****i* atheros5000.device/DeleteUnit **************************************
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
            Signal(unit->task, SIGBREAKF_CTRL_C);
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

      for(i = 0; i < TX_SLOT_COUNT; i++)
         unit->FreeDMAMem(unit->card, unit->tx_buffers[i]);
      for(i = 0; i < MGMT_SLOT_COUNT; i++)
         unit->FreeDMAMem(unit->card, unit->mgmt_buffers[i]);
      for(i = 0; i < RX_SLOT_COUNT; i++)
         unit->FreeDMAMem(unit->card, unit->rx_buffers[i]);

      FreeVec(unit->tx_buffer);
      FreeVec(unit->rx_frames);
      FreeVec(unit->tx_requests);
      FreeVec(unit->mgmt_requests);
      FreeVec(unit->channels);
      FreeVec(unit->rx_buffer);

      if(unit->hal != NULL)
         unit->hal->ah_detach(unit->hal);

      FreeMem(unit, sizeof(struct DevUnit));
   }

   return;
}



/****i* atheros5000.device/InitialiseAdapter *******************************
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
   UBYTE i, reg_class_id;
   BOOL success = TRUE;
   unsigned int channel_count, reg_class_count;
   HAL_TXQ_INFO queue_info = {0};

   /* Get default MAC address */

   unit->hal->ah_getMacAddress(unit->hal, unit->default_address);

   /* Get default antenna */

   unit->antenna = unit->hal->ah_getDefAntenna(unit->hal);

   /* Reset key cache */

   for(i = 0; i < WIFI_KEYCOUNT; i++)
      unit->hal->ah_resetKeyCacheEntry(unit->hal, i);

   /* Initialise channels and rates */

   ath_hal_init_channels(unit->hal, unit->channels,
      MAX_CHANNEL_COUNT, &channel_count,
      &reg_class_id, 1, &reg_class_count, CTRY_DEFAULT,
      HAL_MODE_11B | HAL_MODE_PUREG, TRUE,
      FALSE);
   unit->channel_count = channel_count;

   unit->channel = 1;
   unit->band = S2BAND_B;

   /* Check if multi-rate retries are supported */

   if(unit->hal->ah_setupXTxDesc(unit->hal, NULL, 0, 0, 0, 0, 0, 0))
      unit->flags |= UNITF_SLOWRETRIES;

   /* Set up TX queue */

   queue_info.tqi_aifs = HAL_TXQ_USEDEFAULT;
   queue_info.tqi_cwmin = HAL_TXQ_USEDEFAULT;
   queue_info.tqi_cwmax = HAL_TXQ_USEDEFAULT;
   queue_info.tqi_qflags =
      HAL_TXQ_TXEOLINT_ENABLE | HAL_TXQ_TXDESCINT_ENABLE;

   unit->tx_queue_no =
      unit->hal->ah_setupTxQueue(unit->hal, HAL_TX_QUEUE_DATA, &queue_info);
   unit->hal->ah_resetTxQueue(unit->hal, unit->tx_queue_no);

   /* Set up management frame TX queue */

   queue_info.tqi_aifs = HAL_TXQ_USEDEFAULT;
   queue_info.tqi_cwmin = HAL_TXQ_USEDEFAULT;
   queue_info.tqi_cwmax = HAL_TXQ_USEDEFAULT;
   queue_info.tqi_qflags =
      HAL_TXQ_TXEOLINT_ENABLE | HAL_TXQ_TXDESCINT_ENABLE;

   unit->mgmt_queue_no =
      unit->hal->ah_setupTxQueue(unit->hal, HAL_TX_QUEUE_DATA, &queue_info);
   unit->hal->ah_resetTxQueue(unit->hal, unit->mgmt_queue_no);

   /* Find out hardware encryption capabilities */

   if(unit->hal->ah_getCapability(unit->hal, HAL_CAP_CIPHER,
      HAL_CIPHER_WEP, NULL) == HAL_OK)
      unit->flags |= UNITF_HARDWEP;
   if(unit->hal->ah_getCapability(unit->hal, HAL_CAP_CIPHER,
      HAL_CIPHER_TKIP, NULL) == HAL_OK)
      unit->flags |= UNITF_HARDTKIP;
#if 1
   if(unit->hal->ah_getCapability(unit->hal, HAL_CAP_CIPHER,
      HAL_CIPHER_MIC, NULL) == HAL_OK)
   {
      if(unit->hal->ah_setCapability(unit->hal, HAL_CAP_TKIP_MIC,
//         0, TRUE, NULL))
         0, 0, NULL))
         unit->flags |= UNITF_HARDMIC;
   }
#else
   if(unit->hal->ah_setCapability(unit->hal, HAL_CAP_TKIP_MIC,
      0, FALSE, NULL))
#endif
   if(unit->hal->ah_getCapability(unit->hal, HAL_CAP_TKIP_SPLIT, 0,
      NULL) == HAL_OK)
      unit->flags |= UNITF_SPLITMIC;
   if(unit->hal->ah_getCapability(unit->hal, HAL_CAP_CIPHER,
      HAL_CIPHER_AES_OCB, NULL) == HAL_OK)
      unit->flags |= UNITF_HARDAESOCB;
   if(unit->hal->ah_getCapability(unit->hal, HAL_CAP_CIPHER,
      HAL_CIPHER_AES_CCM, NULL) == HAL_OK)
      unit->flags |= UNITF_HARDCCMP;

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



/****i* atheros5000.device/ConfigureAdapter ********************************
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
   UWORD i, j, key_length, hal_band, key_type;
   const struct KeyUnion *keys;
   HAL_CHANNEL *channel = unit->channels, *ch;
   ULONG freq = 2407 + unit->channel * 5;
   HAL_STATUS status;
   HAL_KEYVAL hal_key;
   HAL_BOOL iq_done;
   const HAL_RATE_TABLE *rate_table;

   /* Get band-specific parameters */

   if(unit->band == S2BAND_G)
   {
      hal_band = HAL_MODE_11G;
      unit->band_mask = CHANNEL_G;
      unit->tx_rates = g_retry_rates;
      unit->mgmt_rate = G_MGMT_RATE;
   }
   else if(unit->band == S2BAND_B)
   {
      hal_band = HAL_MODE_11B;
      unit->band_mask = CHANNEL_B;
      unit->tx_rates = b_retry_rates;
      unit->mgmt_rate = B_MGMT_RATE;
   }
   unit->rate_table = unit->hal->ah_getRateTable(unit->hal, hal_band);

   /* Find rate codes to match our optimal and retry rates */

   rate_table = unit->rate_table;
   for(i = 0; i < rate_table->rateCount; i++)
   {
      for(j = 0; j < 4; j++)
      {
         if(rate_table->info[i].rateKbps == unit->tx_rates[j])
         {
            unit->tx_rate_codes[j] = rate_table->info[i].rateCode;
            if((unit->flags & UNITF_SHORTPREAMBLE) != 0)
               unit->tx_rate_codes[j] |= rate_table->info[i].shortPreamble;
         }
      }
      if(rate_table->info[i].rateKbps == unit->mgmt_rate)
      {
         unit->mgmt_rate_code = rate_table->info[i].rateCode;
         if((unit->flags & UNITF_SHORTPREAMBLE) != 0)
            unit->mgmt_rate_code |= rate_table->info[i].shortPreamble;
      }
   }

   /* Find a channel that matches requirements */

   for(i = 0, ch = unit->channels; i < unit->channel_count; i++, ch++)
   {
      if(ch->channel == freq
         && (ch->channelFlags & unit->band_mask) == unit->band_mask)
         channel = ch;
   }

   /* Stop the transceiver if we're already online */

   if((unit->flags & UNITF_ONLINE) != 0)
   {
      /* Disable frame transmission */

      unit->hal->ah_stopTxDma(unit->hal, unit->tx_queue_no);
      unit->hal->ah_stopTxDma(unit->hal, unit->mgmt_queue_no);
      unit->hal->ah_setRxFilter(unit->hal, 0);
ath_hal_delay(3000);

      /* Disable frame reception */

      unit->hal->ah_stopPcuReceive(unit->hal);
      unit->hal->ah_stopDmaReceive(unit->hal);

      /* Disable interrupts */

      unit->hal->ah_setInterrupts(unit->hal, 0);
   }

   /* Calculate RX filter mask */

   unit->filter_mask = HAL_RX_FILTER_UCAST | HAL_RX_FILTER_MCAST
      | HAL_RX_FILTER_BCAST;
   if((unit->flags & UNITF_PROM) != 0)
      unit->filter_mask |= HAL_RX_FILTER_PROM;
   if(unit->mode != S2PORT_MANAGED)
      unit->filter_mask |= HAL_RX_FILTER_PROBEREQ;

   /* Reset card */

   unit->hal->ah_reset(unit->hal, HAL_M_STA, channel, FALSE, &status);

   /* Set MAC address and miscellaneous wireless parameters */

   unit->hal->ah_setMacAddress(unit->hal, unit->address);
   unit->hal->ah_setPCUConfig(unit->hal);
   SetMulticast(unit, base);
   unit->hal->ah_setDefAntenna(unit->hal, unit->antenna);
   unit->hal->ah_perCalibration(unit->hal, channel, &iq_done);
   unit->hal->ah_setTxPowerLimit(unit->hal, TX_POWER);

   /* Set association ID */

   unit->hal->ah_writeAssocid(unit->hal, unit->bssid, unit->assoc_id);

   /* Put on a reassuring light */

   unit->hal->ah_setLedState(unit->hal, HAL_LED_RUN);

   /* Set or clear default encryption keys where appropriate */

   keys = unit->keys;
   for(i = 0; i < WIFI_KEYCOUNT; i++)
   {
      key_type = unit->keys[i].type;
      if(key_type <= S2ENC_WEP
         || key_type == S2ENC_TKIP && (unit->flags & UNITF_HARDTKIP) == 0
         || key_type == S2ENC_CCMP && (unit->flags & UNITF_HARDCCMP) == 0)
      {
         if(key_type == S2ENC_WEP && (unit->flags & UNITF_HARDWEP) != 0)
         {
            hal_key.kv_type = HAL_CIPHER_WEP;
            key_length = keys[i].u.wep.length;
            CopyMem(keys[i].u.wep.key, hal_key.kv_val, key_length);
         }
         else
         {
            hal_key.kv_type = HAL_CIPHER_CLR;
            key_length = 0;
         }
         hal_key.kv_len = key_length;
         unit->hal->ah_setKeyCacheEntry(unit->hal, i, &hal_key,
            unit->bssid, FALSE);
      }
   }

   /* Set pointer to RX ring */

   unit->hal->ah_setRxDP(unit->hal,
      unit->rx_descs_p + unit->rx_slot * sizeof(struct ath_desc));

   /* Restart the transceiver if we're already online */

   if((unit->flags & UNITF_ONLINE) != 0)
   {
      /* Enable interrupts */

      unit->hal->ah_setInterrupts(unit->hal, INT_MASK);

      /* Enable frame reception */

      unit->hal->ah_enableReceive(unit->hal);
      unit->hal->ah_setRxFilter(unit->hal, unit->filter_mask);
      unit->hal->ah_startPcuReceive(unit->hal);

#if 0
      /* Reset TX queues */

      unit->hal->ah_resetTxQueue(unit->hal, unit->tx_queue_no);
      unit->hal->ah_resetTxQueue(unit->hal, unit->mgmt_queue_no);
#endif
   }

   /* Return */

   return;
}



/****i* atheros5000.device/GoOnline ****************************************
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
   unit->hal->ah_setInterrupts(unit->hal, INT_MASK);

   /* Enable frame reception */

   unit->hal->ah_enableReceive(unit->hal);
   unit->hal->ah_setRxFilter(unit->hal, unit->filter_mask);
   unit->hal->ah_startPcuReceive(unit->hal);

#if 0
   /* Reset TX queues */

   unit->hal->ah_resetTxQueue(unit->hal, unit->tx_queue_no);
   unit->hal->ah_resetTxQueue(unit->hal, unit->mgmt_queue_no);
#endif

   /* Record start time and report Online event */

   GetSysTime(&unit->stats.LastStart);
   ReportEvents(unit, S2EVENT_ONLINE, base);

   return;
}



/****i* atheros5000.device/GoOffline ***************************************
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
      /* Disable frame transmission */

      unit->hal->ah_stopTxDma(unit->hal, unit->tx_queue_no);
      unit->hal->ah_stopTxDma(unit->hal, unit->mgmt_queue_no);

      /* Disable frame reception */

      unit->hal->ah_stopPcuReceive(unit->hal);
      unit->hal->ah_stopDmaReceive(unit->hal);
      unit->hal->ah_setRxFilter(unit->hal, 0);
ath_hal_delay(3000);

      /* Stop interrupts */

      unit->hal->ah_setInterrupts(unit->hal, 0);

      /* Update statistics */

      UpdateStats(unit, base);
   }

   /* Flush pending read and write requests */

   FlushUnit(unit, MGMT_QUEUE, S2ERR_OUTOFSERVICE, base);

   /* Report Offline event and return */

   ReportEvents(unit, S2EVENT_OFFLINE, base);
   return;
}



/****i* atheros5000.device/SetOptions **************************************
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
         if(tag_item->ti_Data != unit->band)
         {
            unit->band = tag_item->ti_Data;
            reconfigure = TRUE;
         }
         break;
      }
   }

   return reconfigure;
}



/****i* atheros5000.device/SetKey ******************************************
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
   HAL_KEYVAL hal_key;
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
// TO DO: Wait for TX queue to empty
            /* Load parameters for hardware encryption */

            hal_key.kv_type = HAL_CIPHER_TKIP;
            hal_key.kv_len = 16;
            CopyMem(slot->u.tkip.key, hal_key.kv_val, 16);

            CopyMem(slot->u.tkip.tx_mic_key, hal_key.kv_mic, MIC_SIZE);
            unit->hal->ah_setKeyCacheEntry(unit->hal, index, &hal_key,
               unit->bssid, FALSE);
            CopyMem(slot->u.tkip.rx_mic_key, hal_key.kv_mic, MIC_SIZE);
            unit->hal->ah_setKeyCacheEntry(unit->hal, index + 32, &hal_key,
               unit->bssid, FALSE);
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

         if((unit->flags & UNITF_HARDCCMP) != 0)
         {
// TO DO: Wait for TX queue to empty
            /* Load parameters for hardware encryption */

            hal_key.kv_type = HAL_CIPHER_AES_CCM;
            hal_key.kv_len = 16;
            CopyMem(slot->u.ccmp.key, hal_key.kv_val, 16);
            unit->hal->ah_setKeyCacheEntry(unit->hal, index, &hal_key,
               unit->bssid, FALSE);
         }
   }

   /* Update type of key in selected slot */

   slot->type = type;
   Enable();

   return;
}



/****i* atheros5000.device/AddMulticastRange *******************************
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



/****i* atheros5000.device/RemMulticastRange *******************************
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



/****i* atheros5000.device/FindMulticastRange ******************************
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



/****i* atheros5000.device/SetMulticast ************************************
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
   unit->hal->ah_setMulticastFilter(unit->hal, 0xffffffff, 0xffffffff);

   return;
}



/****i* atheros5000.device/FindTypeStats ***********************************
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



/****i* atheros5000.device/FlushUnit ***************************************
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



/****i* atheros5000.device/StatusInt ***************************************
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

static BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code))
{
   struct DevBase *base;
   uint32_t queue_mask;
   HAL_INT ints, int_mask;

   base = unit->device;
   int_mask = unit->hal->ah_getInterrupts(unit->hal);

   if(!unit->hal->ah_isInterruptPending(unit->hal)) return FALSE;

   /* Handle ints */

   if(unit->hal->ah_getPendingInterrupts(unit->hal, &ints))
   {
      if((ints & HAL_INT_TX) != 0)
      {
         int_mask &= ~(HAL_INT_TX | HAL_INT_TXDESC);
         queue_mask = 1 << unit->tx_queue_no | 1 << unit->mgmt_queue_no;
         unit->hal->ah_getTxIntrQueue(unit->hal, &queue_mask);
         if((queue_mask & 1 << unit->tx_queue_no) != 0)
            Cause(&unit->tx_end_int);
         if((queue_mask & 1 << unit->mgmt_queue_no) != 0)
            Cause(&unit->mgmt_end_int);
      }
      if((ints & HAL_INT_RX) != 0)
      {
         int_mask &= ~(HAL_INT_RX | HAL_INT_RXEOL);
         Cause(&unit->rx_int);
      }
   }

#ifdef __MORPHOS__
   int_mask = INT_MASK;
#endif
//   unit->hal->ah_setInterrupts(unit->hal, int_mask))

   return FALSE;
}



/****i* atheros5000.device/RXInt *******************************************
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
   UWORD ieee_length, frame_control, frame_type, slot, next_slot,
      encryption, key_no, buffer_no, old_length;
   struct DevBase *base;
   BOOL is_good;
   LONG frag_no;
   struct ath_desc *rx_desc, *next_desc;
   ULONG dma_size, rx_desc_p;
   UBYTE *buffer, *p, *frame, *data, *snap_frame, *source;
   struct ath_rx_status status;

   base = unit->device;
   slot = unit->rx_slot;
   rx_desc = unit->rx_descs + slot;
   rx_desc_p = unit->rx_descs_p + slot * sizeof(struct ath_desc);
   next_slot = (slot + 1) % RX_SLOT_COUNT;
   next_desc = unit->rx_descs + next_slot;

   dma_size = sizeof(struct ath_desc) * RX_SLOT_COUNT;
   CachePostDMA(unit->rx_descs, &dma_size, 0);

   while(unit->hal->ah_procRxDesc(unit->hal, rx_desc, rx_desc_p, next_desc,
      unit->hal->ah_getTsf64(unit->hal), &status) != HAL_EINPROGRESS)
   {
      is_good = TRUE;
      buffer = unit->rx_buffers[slot];

      dma_size = FRAME_BUFFER_SIZE;
      CachePostDMA(buffer, &dma_size, 0);

      if(status.rs_status == 0
         && status.rs_datalen >= WIFI_FRM_DATA + FCS_SIZE)
      {
         /* Get fragment info */

         frame = buffer;
         ieee_length = status.rs_datalen - FCS_SIZE - WIFI_FRM_DATA;
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
                  if(encryption == S2ENC_TKIP)
//                     && (unit->flags & UNITF_HARDTKIP) == 0)
//                     && (unit->flags & UNITF_HARDMIC) == 0)
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

                  /* If it's a management frame, process it separately;
                     otherwise distribute it to clients after filtering */

                  if(frame_type == WIFI_FRMTYPE_MGMT)
                  {
                     DistributeMgmtFrame(unit, frame, status.rs_datalen - 4,
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

      unit->hal->ah_setupRxDesc(unit->hal, rx_desc, FRAME_BUFFER_SIZE,
         HAL_RXDESC_INTREQ);

      dma_size = FRAME_BUFFER_SIZE;
      CachePreDMA(buffer, &dma_size, 0);

      /* Get next descriptor */

      slot = next_slot;
      rx_desc = next_desc;
      rx_desc_p = unit->rx_descs_p + slot * sizeof(struct ath_desc);
      next_slot = (slot + 1) % RX_SLOT_COUNT;
      next_desc = unit->rx_descs + next_slot;
   }

   dma_size = sizeof(struct ath_desc) * RX_SLOT_COUNT;
   CachePreDMA(unit->rx_descs, &dma_size, 0);

   unit->rx_slot = slot;
// TO DO: unstall reception?

   /* Re-enable RX interrupts */

#if 0
   Disable();
   unit->hal->ah_setInterrupts(unit->hal,
      unit->hal->ah_getInterrupts(unit->hal) | HAL_INT_RX | HAL_INT_RXEOL);
   Enable();
#endif

   return;
}



/****i* atheros5000.device/GetRXBuffer *************************************
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



/****i* atheros5000.device/DistributeRXPacket ******************************
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



/****i* atheros5000.device/CopyPacket **************************************
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



/****i* atheros5000.device/AddressFilter ***********************************
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



/****i* atheros5000.device/DistributeMgmtFrame *****************************
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



/****i* atheros5000.device/TXInt *******************************************
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
      last_slot, encryption, subtype, duration;
   UBYTE *buffer, *q, *plaintext, *ciphertext, *frame,
      mic_header[ETH_ADDRESSSIZE * 2];
   const UBYTE *p, *dest, *source;
   struct IOSana2Req *request;
   BOOL proceed = TRUE, is_ieee;
   struct Opener *opener;
   ULONG wire_error, dma_size;
   struct ath_desc *tx_desc, *last_desc;
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   BYTE error;
   struct MsgPort *port;
   struct TypeStats *tracker;
   const HAL_RATE_TABLE *rate_table;

   base = unit->device;
   port = unit->request_ports[WRITE_QUEUE];
   rate_table = unit->rate_table;

   while(proceed && (!IsMsgPortEmpty(port)))
   {
      slot = unit->tx_in_slot;
      new_slot = (slot + 1) % TX_SLOT_COUNT;

//      if(new_slot != unit->tx_out_slot)
      if(slot == unit->tx_out_slot)   // one packet at a time
      {
         error = 0;
         body_size = 0;

         /* Get request and DMA frame descriptor */

         request = (APTR)port->mp_MsgList.lh_Head;

         Remove((APTR)request);
         unit->tx_requests[slot] = request;
         tx_desc = unit->tx_descs + slot;
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
            q += 2;

            i = rate_table->rateCodeToIndex[unit->tx_rate_codes[0]];
            if((unit->flags & UNITF_SHORTPREAMBLE) != 0)
               duration = rate_table->info[i].spAckDuration;
            else
               duration = rate_table->info[i].lpAckDuration;
            *(UWORD *)q = MakeLEWord(duration);
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
            q += 2;

// TO DO: need to pad header to 4-byte boundary?

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
//               if((unit->flags & UNITF_HARDMIC) == 0)
               {
                  q = mic_header;
                  for(i = 0, p = dest; i < ETH_ADDRESSSIZE; i++)
                     *q++ = *p++;
                  for(i = 0, p = source; i < ETH_ADDRESSSIZE; i++)
                     *q++ = *p++;
                  TKIPEncryptFrame(unit, mic_header, plaintext, body_size,
                     plaintext, base);
               }
               body_size += MIC_SIZE;
            }

            /* Encrypt fragment if applicable */

            unit->fragment_encrypt_functions[encryption](unit, frame,
               plaintext, &body_size, ciphertext, base);

            /* Fill in DMA descriptor for packet transmission */

            frame_size = WIFI_FRM_DATA + body_size;
            tx_desc->ds_link = (ULONG)(UPINT)NULL;
            unit->hal->ah_setupTxDesc(unit->hal, tx_desc, frame_size + 4, // + CRC?
               WIFI_FRM_DATA, HAL_PKT_TYPE_NORMAL, TX_POWER,
               ((unit->flags & UNITF_SLOWRETRIES) != 0) ?
                  unit->tx_rate_codes[0] : unit->tx_rate_codes[1],
               ((unit->flags & UNITF_SLOWRETRIES) != 0) ? 1 : TX_TRIES,
               HAL_TXKEYIX_INVALID,
               HAL_ANTENNA_MIN_MODE,
               HAL_TXDESC_INTREQ | HAL_TXDESC_CLRDMASK,
               0, 0, 0, 0, 3);
            if((unit->flags & UNITF_SLOWRETRIES) != 0)
               unit->hal->ah_setupXTxDesc(unit->hal, tx_desc,
                  unit->tx_rate_codes[1], 1,
                  unit->tx_rate_codes[2], 1,
                  unit->tx_rate_codes[3], TX_TRIES - 3);
            unit->hal->ah_fillTxDesc(unit->hal, tx_desc, frame_size, TRUE, TRUE,
               tx_desc);

            dma_size = frame_size;
            CachePreDMA(frame, &dma_size, DMA_ReadFromRAM);

            /* Pass packet to adapter */

            unit->hal->ah_stopTxDma(unit->hal, unit->tx_queue_no);
            last_slot = (slot + TX_SLOT_COUNT - 1) % TX_SLOT_COUNT;
            if(unit->tx_out_slot != slot)
            //if(unit->hal->ah_numTxPending(unit->hal, unit->tx_queue_no) > 0)
            {
               last_desc =
                  unit->tx_descs + last_slot;
               last_desc->ds_link = unit->tx_descs_p + slot * sizeof(struct ath_desc);
               dma_size = sizeof(struct ath_desc) * TX_SLOT_COUNT;
               CachePreDMA(unit->tx_descs, &dma_size, 0);
            }
            else
            {
               dma_size = sizeof(struct ath_desc) * TX_SLOT_COUNT;
               CachePreDMA(unit->tx_descs, &dma_size, 0);
               unit->hal->ah_setTxDP(unit->hal, unit->tx_queue_no,
                  unit->tx_descs_p + slot * sizeof(struct ath_desc));
            }

            unit->hal->ah_startTxDma(unit->hal, unit->tx_queue_no);
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
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
   else
      unit->request_ports[WRITE_QUEUE]->mp_Flags = PA_IGNORE;

   return;
}



/****i* atheros5000.device/TXEndInt ****************************************
*
*   NAME
*	TXEndInt -- Clean up after a data frame has been sent.
*
*   SYNOPSIS
*	TXEndInt(unit, int_code)
*
*	VOID TXEndInt(struct DevUnit *, APTR);
*
*   NOTES
*	I think it's safe to assume that there will always be at least one
*	completed packet whenever this interrupt is called.
*
****************************************************************************
*
*/

static VOID TXEndInt(REG(a1, struct DevUnit *unit),
   REG(a6, APTR int_code))
{
   UWORD frame_size, new_out_slot, i;
   UBYTE *frame;
   struct DevBase *base;
   struct IOSana2Req *request;
   ULONG dma_size;
   struct TypeStats *tracker;

   /* Find out which packets have completed */

   base = unit->device;
   new_out_slot = (unit->tx_in_slot + TX_SLOT_COUNT
      - unit->hal->ah_numTxPending(unit->hal, unit->tx_queue_no))
      % TX_SLOT_COUNT;

   dma_size = sizeof(struct ath_desc) * TX_SLOT_COUNT;
   CachePostDMA(unit->tx_descs, &dma_size, 0);

   /* Retire sent packets */

   for(i = unit->tx_out_slot; i != new_out_slot;
      i = (i + 1) % TX_SLOT_COUNT)
   {
      frame = unit->tx_buffers[i];
      dma_size = FRAME_BUFFER_SIZE;
      CachePostDMA(frame, &dma_size, DMA_ReadFromRAM);

      /* Update statistics */

      request = unit->tx_requests[i];
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

      /* Reply request */

      request->ios2_Req.io_Error = 0;
      ReplyMsg((APTR)request);
   }

   unit->tx_out_slot = i;

   dma_size = sizeof(struct ath_desc) * TX_SLOT_COUNT;
   CachePreDMA(unit->tx_descs, &dma_size, 0);

   /* Restart downloads if they had stopped */

   if(unit->request_ports[WRITE_QUEUE]->mp_Flags == PA_IGNORE)
      Cause(&unit->tx_int);

   return;
}



/****i* atheros5000.device/MgmtTXInt ***************************************
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
   UWORD frame_size, slot, new_slot, last_slot, i, duration;
   UBYTE *frame;
   struct IOSana2Req *request;
   BOOL proceed = TRUE, has_bssid;
   ULONG dma_size;
   struct ath_desc *desc, *last_desc;
   struct MsgPort *port;
   const HAL_RATE_TABLE *rate_table;

   base = unit->device;
   port = unit->request_ports[MGMT_QUEUE];
   rate_table = unit->rate_table;

   while(proceed && (!IsMsgPortEmpty(port)))
   {
      slot = unit->mgmt_in_slot;
      new_slot = (slot + 1) % MGMT_SLOT_COUNT;

//      if(new_slot != unit->mgmt_out_slot)
      if(slot == unit->mgmt_out_slot)   // one packet at a time
      {
         /* Get request and DMA frame descriptor */

         request = (APTR)port->mp_MsgList.lh_Head;

         Remove((APTR)request);
         unit->mgmt_requests[slot] = request;
         desc = unit->mgmt_descs + slot;
         frame = unit->mgmt_buffers[slot];

         /* Get packet length */

         frame_size = request->ios2_DataLength;

         /* Copy frame into DMA buffer */

         CopyMem(request->ios2_Data, frame, frame_size);

         /* Set duration */

         has_bssid = ((frame + WIFI_FRM_ADDRESS1)[0] & 0x1) == 0;
         i = rate_table->rateCodeToIndex[unit->mgmt_rate_code];
         if(has_bssid)
         {
            if((unit->flags & UNITF_SHORTPREAMBLE) != 0)
               duration = rate_table->info[i].spAckDuration;
            else
               duration = rate_table->info[i].lpAckDuration;
         }
         else
            duration = 0;
         *(UWORD *)(frame + WIFI_FRM_DURATION) = MakeLEWord(duration);

         /* Fill in DMA descriptor for packet transmission */

         desc->ds_link = (ULONG)(UPINT)NULL;
         unit->hal->ah_setupTxDesc(unit->hal, desc, frame_size + 4, // + CRC?
            WIFI_FRM_DATA, HAL_PKT_TYPE_NORMAL, TX_POWER,
            unit->mgmt_rate_code, TX_TRIES, HAL_TXKEYIX_INVALID,
            HAL_ANTENNA_MIN_MODE,
            HAL_TXDESC_INTREQ | HAL_TXDESC_CLRDMASK
            | (has_bssid ? 0 : HAL_TXDESC_NOACK),
            0, 0, 0, 0, 3);
         unit->hal->ah_fillTxDesc(unit->hal, desc, frame_size, TRUE, TRUE,
            desc);

         dma_size = frame_size;
         CachePreDMA(frame, &dma_size, DMA_ReadFromRAM);

         /* Pass packet to adapter */

         unit->hal->ah_stopTxDma(unit->hal, unit->mgmt_queue_no);
         last_slot = (slot + MGMT_SLOT_COUNT - 1) % MGMT_SLOT_COUNT;
         if(unit->mgmt_out_slot != slot)
         //if(unit->hal->ah_numTxPending(unit->hal, unit->mgmt_queue_no) > 0)
         {
            last_desc = unit->mgmt_descs + last_slot;
            last_desc->ds_link =
               unit->mgmt_descs_p + slot * sizeof(struct ath_desc);
            dma_size = sizeof(struct ath_desc) * MGMT_SLOT_COUNT;
            CachePreDMA(unit->mgmt_descs, &dma_size, 0);
         }
         else
         {
            dma_size = sizeof(struct ath_desc) * MGMT_SLOT_COUNT;
            CachePreDMA(unit->mgmt_descs, &dma_size, 0);
            unit->hal->ah_setTxDP(unit->hal, unit->mgmt_queue_no,
               unit->mgmt_descs_p + slot * sizeof(struct ath_desc));
         }

         unit->hal->ah_startTxDma(unit->hal, unit->mgmt_queue_no);
         unit->mgmt_in_slot = new_slot;
      }
      else
         proceed = FALSE;
   }

   /* Don't try to keep sending packets if there's no space left */

   if(proceed)
      unit->request_ports[MGMT_QUEUE]->mp_Flags = PA_SOFTINT;
   else
      unit->request_ports[MGMT_QUEUE]->mp_Flags = PA_IGNORE;

   return;
}



/****i* atheros5000.device/MgmtTXEndInt ************************************
*
*   NAME
*	MgmtTXEndInt -- Clean up after a management frame has been sent.
*
*   SYNOPSIS
*	MgmtTXEndInt(unit, int_code)
*
*	VOID MgmtTXEndInt(struct DevUnit *, APTR);
*
*   NOTES
*	I think it's safe to assume that there will always be at least one
*	completed packet whenever this interrupt is called.
*
****************************************************************************
*
*/

static VOID MgmtTXEndInt(REG(a1, struct DevUnit *unit),
   REG(a6, APTR int_code))
{
   UWORD new_out_slot, i;
   UBYTE *frame;
   struct DevBase *base;
   struct IOSana2Req *request;
   ULONG dma_size;

   /* Find out which packets have completed */

   base = unit->device;
   new_out_slot = (unit->mgmt_in_slot + MGMT_SLOT_COUNT
      - unit->hal->ah_numTxPending(unit->hal, unit->mgmt_queue_no))
      % MGMT_SLOT_COUNT;

   dma_size = sizeof(struct ath_desc) * MGMT_SLOT_COUNT;
   CachePostDMA(unit->mgmt_descs, &dma_size, 0);

   /* Retire sent frames */

   for(i = unit->mgmt_out_slot; i != new_out_slot;
      i = (i + 1) % MGMT_SLOT_COUNT)
   {
      frame = unit->mgmt_buffers[i];
      dma_size = FRAME_BUFFER_SIZE;
      CachePostDMA(frame, &dma_size, DMA_ReadFromRAM);

      /* Reply request */

      request = unit->mgmt_requests[i];
      request->ios2_Req.io_Error = 0;
      ReplyMsg((APTR)request);
   }

   unit->mgmt_out_slot = i;

   dma_size = sizeof(struct ath_desc) * MGMT_SLOT_COUNT;
   CachePreDMA(unit->mgmt_descs, &dma_size, 0);

   /* Restart downloads if they had stopped */

   if(unit->request_ports[MGMT_QUEUE]->mp_Flags == PA_IGNORE)
      Cause(&unit->mgmt_int);

   return;
}



/****i* atheros5000.device/ResetHandler ************************************
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
      /* Disable frame transmission */

      unit->hal->ah_stopTxDma(unit->hal, unit->tx_queue_no);
      unit->hal->ah_stopTxDma(unit->hal, unit->mgmt_queue_no);

      /* Disable frame reception */

      unit->hal->ah_stopPcuReceive(unit->hal);
      unit->hal->ah_stopDmaReceive(unit->hal);

      /* Stop interrupts */

      unit->hal->ah_setInterrupts(unit->hal, 0);
   }

   return;
}



/****i* atheros5000.device/UpdateStats *************************************
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

   return;
}



/****i* atheros5000.device/ReportEvents ************************************
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



/****i* atheros5000.device/GetRadioBands ***********************************
*
*   NAME
*	GetRadioBands -- Get information on current network.
*
*   SYNOPSIS
*	tag_list = GetRadioBands(unit, pool)
*
*	struct TagItem *GetRadioBands(struct DevUnit *, APTR);
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

#if 0
struct TagItem *GetRadioBands(struct DevUnit *unit, APTR pool,
   struct DevBase *base)
{
   BYTE error = 0;
   struct Sana2RadioBand *bands;
   UWORD i;

   bands = AllocPooled(pool, sizeof(struct Sana2RadioBand) * 3);
   if(bands == NULL)
      error = S2ERR_NO_RESOURCES;

   if(error == 0)
   {
      for(i = 0; i < 1; i++)
      {
         bands[i] = AllocPooled(pool, sizeof(UWORD) * unit->channel_counts[S2BAND_B]);
      }
   }

//   if(error != 0)
//      bands = NULL;

   return bands;
}
#endif



/****i* atheros5000.device/UnitTask ****************************************
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

   FreeMem(task->tc_SPLower, STACK_SIZE);
}



