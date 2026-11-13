/*

Copyright (C) 2001-2025 Neil Cafferkey

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
#include <exec/errors.h>

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include "device.h"
#include "task.h"
#include "nvidianet.h"
#include "mii.h"

#include "unit_protos.h"
#include "request_protos.h"
#include "timer_protos.h"


#define TASK_PRIORITY 0
#define STACK_SIZE 4096
#define INT_MASK \
   (NV_INTF_LINK | NV_INTF_TXDONE | NV_INTF_RXDONE | NV_INTF_TXERR)
#define MII_MASK NV_MIIINTF_LINK
#define TX_DESC_TABLE_SIZE (NV_DESCSIZE * 2 * TX_SLOT_COUNT)
#define RX_DESC_TABLE_SIZE (NV_DESCSIZE * RX_SLOT_COUNT)
#define MAX_FRAME_SIZE (ETH_HEADERSIZE + ETH_MTU + ETH_CRCSIZE)
#define FRAME_BUFFER_SIZE (MAX_FRAME_SIZE + 0)
#define HEADER_BUFFER_SIZE (ETH_HEADERSIZE + 2)
#define PHY_NO 3
#define POLL_TIME 970

static BOOL InitialiseAdapter(struct DevUnit *unit, struct DevBase *base);
static VOID DeinitialiseAdapter(struct DevUnit *unit, struct DevBase *base);
static VOID StopAdapter(struct DevUnit *unit, struct DevBase *base);
static BOOL UpdateLinkSpeed(struct DevUnit *unit, struct DevBase *base);
static struct AddressRange *FindMulticastRange(struct DevUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
   UWORD upper_bound_right, struct DevBase *base);
static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base);
static BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID RXInt(REG(a1, struct DevUnit *unit), REG(a6, APTR int_code));
static VOID DistributeRXFrame(struct DevUnit *unit, const UBYTE *frame,
   UWORD frame_size, struct DevBase *base);
static VOID CopyFrame(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD frame_size, UWORD frame_type, const UBYTE *buffer,
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
static VOID UnitTask(struct DevUnit *unit);
static UWORD ReadMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   struct DevBase *base);


/****i* nvidianet.device/CreateUnit ****************************************
*
*   NAME
*	CreateUnit -- Create a unit.
*
*   SYNOPSIS
*	unit = CreateUnit(index, card, io_tags)
*
*	struct DevUnit *CreateUnit(ULONG, APTR, struct TagItem *);
*
*   FUNCTION
*	Creates a new unit.
*
****************************************************************************
*
*/

struct DevUnit *CreateUnit(ULONG index, APTR card,
   const struct TagItem *io_tags, struct DevBase *base)
{
   BOOL success = TRUE;
   struct DevUnit *unit;
   struct Task *task;
   struct MsgPort *port;
   UWORD i;
   ULONG *desc, *descs, dma_size;
   UQUAD buffer_p;
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

      /* Store I/O hooks */

      unit->LELongIn =
         (APTR)GetTagData(IOTAG_LELongIn, (UPINT)NULL, io_tags);
      unit->LELongOut =
         (APTR)GetTagData(IOTAG_LELongOut, (UPINT)NULL, io_tags);
      unit->AllocDMAMem =
         (APTR)GetTagData(IOTAG_AllocDMAMem, (UPINT)NULL, io_tags);
      unit->FreeDMAMem =
         (APTR)GetTagData(IOTAG_FreeDMAMem, (UPINT)NULL, io_tags);
      if(unit->LELongIn == NULL
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

      unit->tx_descs = descs =
         unit->AllocDMAMem(unit->card, TX_DESC_TABLE_SIZE, NV_DESCSIZE);
      if(descs == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Get physical address of TX descriptors */

      dma_size = TX_DESC_TABLE_SIZE;
      unit->tx_descs_p = (UPINT)CachePreDMA(descs, &dma_size, 0);
      if(dma_size != TX_DESC_TABLE_SIZE)
         success = FALSE;
      CachePostDMA(descs, &dma_size, 0);
   }

   if(success)
   {
      /* Allocate RX descriptors */

      unit->rx_descs = descs =
         unit->AllocDMAMem(unit->card, RX_DESC_TABLE_SIZE, NV_DESCSIZE);
      if(descs == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Get physical address of RX descriptors */

      dma_size = RX_DESC_TABLE_SIZE;
      unit->rx_descs_p = (UPINT)CachePreDMA(descs, &dma_size, 0);
      if(dma_size != RX_DESC_TABLE_SIZE)
         success = FALSE;
      CachePostDMA(descs, &dma_size, 0);

      /* Allocate frame buffers */

      unit->tx_buffer = unit->AllocDMAMem(unit->card, FRAME_BUFFER_SIZE, 1);
      unit->tx_headers = unit->AllocDMAMem(unit->card,
         HEADER_BUFFER_SIZE * TX_SLOT_COUNT, HEADER_BUFFER_SIZE);
      if(unit->tx_buffer == NULL || unit->tx_headers == NULL)
         success = FALSE;

      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         unit->rx_buffers[i] =
            unit->AllocDMAMem(unit->card, FRAME_BUFFER_SIZE, 1);
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
      /* Initialise TX ring */

      desc = unit->tx_descs;
      for(i = 0; i < 2 * TX_SLOT_COUNT; i++)
      {
         desc[NV_DESC_VLAN] = 0;
         desc[NV_DESC_CONTROL] = 0;
         desc += NV_DESCSIZE / sizeof(ULONG);
      }
      dma_size = TX_DESC_TABLE_SIZE;
      CachePreDMA(unit->tx_descs, &dma_size, 0);

      /* Initialise RX ring */

      desc = unit->rx_descs;
      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         dma_size = FRAME_BUFFER_SIZE;
         buffer_p =
            (UPINT)CachePreDMA(unit->rx_buffers[i], &dma_size, 0);
         if(dma_size != FRAME_BUFFER_SIZE)
            success = FALSE;
         desc[NV_DESC_BUFFERHIGH] = MakeLELong((ULONG)(buffer_p >> 32));
         desc[NV_DESC_BUFFERLOW] = MakeLELong((ULONG)buffer_p);
         desc[NV_DESC_VLAN] = 0;
         desc[NV_DESC_CONTROL] =
            MakeLELong(NV_DESC_CONTROLF_INUSE | MAX_FRAME_SIZE);
         desc += NV_DESCSIZE / sizeof(ULONG);
      }
      dma_size = RX_DESC_TABLE_SIZE;
      CachePreDMA(unit->rx_descs, &dma_size, 0);

      /* Set MII PHY number */

      unit->mii_phy_no = PHY_NO;
   }

   if(success)
   {
      /* Initialise network adapter hardware */

      success = InitialiseAdapter(unit, base);
   }

   if(success)
   {
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
      task->tc_SPLower = stack = AllocMem(STACK_SIZE, MEMF_PUBLIC);
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
      task->tc_SPReg = stack + STACK_SIZE;
      NewList(&task->tc_MemEntry);

      if(AddUnitTask(task, UnitTask, unit) != NULL)
         unit->flags |= UNITF_TASKADDED;
      else
         success = FALSE;
   }

   if(!success)
   {
      DeleteUnit(unit, base);
      unit = NULL;
   }

   return unit;
}



/****i* nvidianet.device/DeleteUnit ****************************************
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
         if(task->tc_SPLower != NULL)
         {
            if((unit->flags & UNITF_TASKADDED) != 0)
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

      DeinitialiseAdapter(unit, base);

      for(i = 0; i < RX_SLOT_COUNT; i++)
         unit->FreeDMAMem(unit->card, unit->rx_buffers[i]);
      unit->FreeDMAMem(unit->card, unit->tx_headers);
      unit->FreeDMAMem(unit->card, unit->tx_buffer);
      unit->FreeDMAMem(unit->card, unit->rx_descs);
      unit->FreeDMAMem(unit->card, unit->tx_descs);

      FreeVec(unit->tx_requests);

      FreeMem(unit, sizeof(struct DevUnit));
   }

   return;
}



/****i* nvidianet.device/InitialiseAdapter *********************************
*
*   NAME
*	InitialiseAdapter
*
*   SYNOPSIS
*	success = InitialiseAdapter(unit)
*
*	BOOL InitialiseAdapter(struct DevUnit *);
*
*   FUNCTION
*
*   INPUTS
*	unit
*
*   RESULT
*	success - Success indicator.
*
****************************************************************************
*
*/

static BOOL InitialiseAdapter(struct DevUnit *unit, struct DevBase *base)
{
   BOOL success = TRUE;
   UBYTE *p, seed;
   UWORD mii_value;
   ULONG value;
   struct EClockVal eclock;

   /* Get default MAC address */

   p = unit->default_address;
   value = unit->LELongIn(unit->card, NV_REG_ADDRESSLOW);
   p[0] = value & 0xff;
   value >>= 8;
   p[1] = value & 0xff;
   value >>= 8;
   p[2] = value & 0xff;
   value >>= 8;
   p[3] = value & 0xff;

   value = unit->LELongIn(unit->card, NV_REG_ADDRESSHIGH);
   p[4] = value & 0xff;
   value >>= 8;
   p[5] = value & 0xff;

   /* Reset transceiver */

   unit->LELongOut(unit->card, NV_REG_XCVRCTRL,
      0x2204 | NV_REG_XCVRCTRLF_RESET);
   BusyMicroDelay(4, base);
   unit->LELongOut(unit->card, NV_REG_XCVRCTRL, 0x2204);

   /* Set DMA rings */

   unit->LELongOut(unit->card, NV_REG_TXDMALISTLOW, (ULONG)unit->tx_descs_p);
   unit->LELongOut(unit->card, NV_REG_TXDMALISTHIGH,
      (ULONG)(unit->tx_descs_p >> 32));
   unit->LELongOut(unit->card, NV_REG_RXDMALISTLOW, (ULONG)unit->rx_descs_p);
   unit->LELongOut(unit->card, NV_REG_RXDMALISTHIGH,
      (ULONG)(unit->rx_descs_p >> 32));
   unit->LELongOut(unit->card, NV_REG_DMALISTSIZES,
      RX_SLOT_COUNT - 1 << NV_REG_DMALISTSIZESB_RX
      | TX_SLOT_COUNT * 2 - 1 << NV_REG_DMALISTSIZESB_TX);

   /* Set random seed */

   ReadEClock(&eclock);
   seed = (UBYTE)FastRand(eclock.ev_lo ^ eclock.ev_hi);
   unit->LELongOut(unit->card, NV_REG_RANDOMSEED, 0x7f00 | seed);

   /* Set default options and state */

   unit->LELongOut(unit->card, NV_REG_XCVRCTRL, 0x2200);
   unit->LELongOut(unit->card, NV_REG_VLANCTRL, 0);
   unit->LELongOut(unit->card, NV_REG_XCVRCTRL, 0x2202);
   while((unit->LELongIn(unit->card, 0x130) & 1 << 31) == 0);

   unit->LELongOut(unit->card, NV_REG_INTSTATUS, INT_MASK);
   unit->LELongOut(unit->card, NV_REG_INTMASK, 0);
   unit->LELongOut(unit->card, NV_REG_OFFLOADCONFIG, FRAME_BUFFER_SIZE);

   unit->LELongOut(unit->card, NV_REG_TXDEFER, 0x15050f);
   unit->LELongOut(unit->card, NV_REG_RXDEFER, 0x16);
   unit->LELongOut(unit->card, NV_REG_TXPAUSE, 0x1ff0080);

   /* Select MII PHY number */

   unit->LELongOut(unit->card, NV_REG_ADAPTERCTRL,
      unit->mii_phy_no << NV_REG_ADAPTERCTRLB_PHYNO
      | NV_REG_ADAPTERCTRLF_PHYVALID | NV_REG_ADAPTERCTRLF_RUNNING);
   unit->LELongOut(unit->card, NV_REG_MIISPEED, 0x105);

   /* Ensure adapter is powered on */

   value = unit->LELongIn(unit->card, NV_REG_POWERCTRL);
   if((value & NV_REG_POWERCTRLF_ON) == 0)
   {
      value |= NV_REG_POWERCTRLF_ON;
      unit->LELongOut(unit->card, NV_REG_POWERCTRL, value);
   }
   BusyMicroDelay(10, base);
   value = unit->LELongIn(unit->card, NV_REG_POWERCTRL);
   value |= NV_REG_POWERCTRLF_VALID;
   unit->LELongOut(unit->card, NV_REG_POWERCTRL, value);

   /* Choose frame types to receive */

   value = 0x7f0000 | NV_REG_RXCONFIGF_UCAST;
   if((unit->flags & UNITF_PROM) != 0)
      value |= NV_REG_RXCONFIGF_PROM;
   unit->LELongOut(unit->card, NV_REG_RXCONFIG, value);
   SetMulticast(unit, base);

   /* Record maximum speed in BPS */

   ReadMII(unit, unit->mii_phy_no, MII_STATUS, base);
   mii_value = ReadMII(unit, unit->mii_phy_no, MII_STATUS, base);
   if((mii_value & MII_STATUSF_EXTSTATUS) != 0)
      unit->speed = 1000000000;
   else
      unit->speed = 100000000;

   /* Return */

   return success;
}



/****i* nvidianet.device/DeinitialiseAdapter *******************************
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

static VOID DeinitialiseAdapter(struct DevUnit *unit, struct DevBase *base)
{
   const UBYTE *p;

   /* Restore default MAC address */

   p = unit->default_address;
   unit->LELongOut(unit->card, NV_REG_ADDRESSLOW,
      p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0]);
   unit->LELongOut(unit->card, NV_REG_ADDRESSHIGH, p[5] << 8 | p[4]);

   return;
}



/****i* nvidianet.device/ConfigureAdapter **********************************
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

   /* Set MAC address */

   p = unit->address;
   unit->LELongOut(unit->card, NV_REG_ADDRESSLOW,
      p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0]);
   unit->LELongOut(unit->card, NV_REG_ADDRESSHIGH, p[5] << 8 | p[4]);

   /* Return */

   return;
}



/****i* nvidianet.device/GoOnline ******************************************
*
*   NAME
*	GoOnline -- Put unit online.
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
   ULONG value;

   /* Enable interrupts */

   unit->flags |= UNITF_ONLINE;
   unit->LELongOut(unit->card, NV_REG_INTMASK, INT_MASK);
   unit->LELongOut(unit->card, NV_REG_MIIMASK, MII_MASK);

   /* Check link speed/duplex */

   UpdateLinkSpeed(unit, base);

   /* Enable frame transmission and reception */

   value = unit->LELongIn(unit->card, NV_REG_TXCTRL);
   value |= NV_REG_TXCTRLF_START;
   unit->LELongOut(unit->card, NV_REG_TXCTRL, value);

   value = unit->LELongIn(unit->card, NV_REG_RXCTRL);
   value |= NV_REG_RXCTRLF_START;
   unit->LELongOut(unit->card, NV_REG_RXCTRL, value);

   /* Record start time and report Online event */

   GetSysTime(&unit->stats.LastStart);
   ReportEvents(unit, S2EVENT_ONLINE, base);

   return;
}



/****i* nvidianet.device/GoOffline *****************************************
*
*   NAME
*	GoOffline -- Take unit offline.
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
   /* Stop adapter's activity */

   unit->flags &= ~UNITF_ONLINE;
   StopAdapter(unit, base);

   /* Flush pending read and write requests */

   FlushUnit(unit, WRITE_QUEUE, S2ERR_OUTOFSERVICE, base);

   /* Report Offline event and return */

   ReportEvents(unit, S2EVENT_OFFLINE, base);
   return;
}



/****i* nvidianet.device/StopAdapter ***************************************
*
*   NAME
*	StopAdapter -- Disable transmission/reception and interrupts.
*
*   SYNOPSIS
*	StopAdapter(unit)
*
*	VOID StopAdapter(struct DevUnit *);
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

static VOID StopAdapter(struct DevUnit *unit, struct DevBase *base)
{
   ULONG value;

   /* Stop transmission and reception */

   value = unit->LELongIn(unit->card, NV_REG_TXCTRL);
   value &= ~NV_REG_TXCTRLF_START;
   unit->LELongOut(unit->card, NV_REG_TXCTRL, value);

   value = unit->LELongIn(unit->card, NV_REG_RXCTRL);
   value &= ~NV_REG_RXCTRLF_START;
   unit->LELongOut(unit->card, NV_REG_RXCTRL, value);

   /* Stop interrupts */

   unit->LELongOut(unit->card, NV_REG_INTMASK, 0);
   unit->LELongOut(unit->card, NV_REG_MIIMASK, 0);

   return;
}



/****i* nvidianet.device/UpdateLinkSpeed ***********************************
*
*   NAME
*	UpdateLinkSpeed
*
*   SYNOPSIS
*	success = UpdateLinkSpeed(unit)
*
*	BOOL UpdateLinkSpeed(struct DevUnit *);
*
****************************************************************************
*
*/

static BOOL UpdateLinkSpeed(struct DevUnit *unit, struct DevBase *base)
{
   UWORD phy_no = unit->mii_phy_no, status, local_modes, remote_modes,
      common_modes, control_1g = 0, status_1g = 0, rate = NV_RATE_10;
   BOOL success = TRUE, full_duplex = FALSE;
   ULONG value;

   /* Find negotiated speed, or fall back to 10Mbps half duplex */

   ReadMII(unit, phy_no, MII_STATUS, base);
   status = ReadMII(unit, phy_no, MII_STATUS, base);
                           /* Yes, status reg must be read twice */
   if((status & MII_STATUSF_LINK) != 0)
   {
      if(((status & MII_STATUSF_AUTONEGDONE) != 0)
         && ((status & MII_STATUSF_EXTREGSET) != 0))
      {
         if((status & MII_STATUSF_EXTSTATUS) != 0)
         {
            control_1g = ReadMII(unit, phy_no, MII_1GCONTROL, base);
            status_1g = ReadMII(unit, phy_no, MII_1GSTATUS, base);
         }
         local_modes = ReadMII(unit, phy_no, MII_AUTONEGADVERT, base);
         remote_modes = ReadMII(unit, phy_no, MII_AUTONEGABILITY, base);
         common_modes = local_modes & remote_modes;

         if((control_1g & MII_1GCONTROLF_1000BASETFD) != 0
            && (status_1g & MII_1GSTATUSF_1000BASETFD) != 0)
         {
            rate = NV_RATE_1000;
            full_duplex = TRUE;
         }
         else if((control_1g & MII_1GCONTROLF_1000BASET) != 0
            && (status_1g & MII_1GSTATUSF_1000BASET) != 0)
         {
            rate = NV_RATE_1000;
         }
         else if((common_modes & MII_AUTONEGF_100BASETXFD) != 0)
         {
            rate = NV_RATE_100;
            full_duplex = TRUE;
         }
         else if((common_modes & MII_AUTONEGF_100BASETX) != 0)
            rate = NV_RATE_100;
         else if((common_modes & MII_AUTONEGF_10BASETFD) != 0)
            full_duplex = TRUE;
         else if((common_modes & MII_AUTONEGF_10BASET) == 0)
            success = FALSE;
      }
   }
   else
      success = FALSE;

   if(success)
   {
      /* Configure selected speed and duplex */

      unit->LELongOut(unit->card, NV_REG_LINKSPEED, rate | 0x10000);

      if(full_duplex)
         value = 0x3b0f3c;
      else
         value = 0x3b0f3e;
      unit->LELongOut(unit->card, 0x80, value);

      value = unit->LELongIn(unit->card, NV_REG_PHYCTRL);
      value &= ~(NV_REG_PHYCTRLF_1000MBPS | NV_REG_PHYCTRLF_100MBPS);
      if(rate == NV_RATE_1000)
         value |= NV_REG_PHYCTRLF_1000MBPS;
      else if(rate == NV_RATE_100)
         value |= NV_REG_PHYCTRLF_100MBPS;
      if(full_duplex)
         value &= ~NV_REG_PHYCTRLF_HDUPLEX;
      else
         value |= NV_REG_PHYCTRLF_HDUPLEX;
      unit->LELongOut(unit->card, NV_REG_PHYCTRL, value);

      /* Set TX deferral */

      if((value & NV_REG_PHYCTRLF_RGMII) != 0)
      {
         if(rate == NV_RATE_1000)
            value = 0x14050f;
         else
            value = 0x16070f;
      }
      else
         value = 0x15050f;
      unit->LELongOut(unit->card, NV_REG_TXDEFER, value);

      /* Set TX threshold */

      if(rate == NV_RATE_1000)
         value = 0xfe08000;
      else
         value = 0x1e08000;
      unit->LELongOut(unit->card, 0x13c, value);

      /* Adapt random seed to link speed on gigabit PHYs */

      if((status & MII_STATUSF_EXTSTATUS) != 0)
      {
         value = unit->LELongIn(unit->card, NV_REG_RANDOMSEED);
         value &= 0xff;
         if(rate == NV_RATE_1000)
            value |= 0x7f00;
         else if(rate == NV_RATE_100)
            value |= 0x2d00;
         else
            value |= 0x7400;
         unit->LELongOut(unit->card, NV_REG_RANDOMSEED, value);
      }
   }

   /* Record and report new connection status */

   if(success && (unit->flags & UNITF_CONNECTED) == 0)
   {
      unit->flags |= UNITF_CONNECTED;
      ReportEvents(unit, S2EVENT_CONNECT, base);
   }
   else if(!success && (unit->flags & UNITF_CONNECTED) != 0)
   {
      unit->flags &= ~UNITF_CONNECTED;
      ReportEvents(unit, S2EVENT_DISCONNECT, base);
   }

   return success;
}



/****i* nvidianet.device/AddMulticastRange *********************************
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



/****i* nvidianet.device/RemMulticastRange *********************************
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



/****i* nvidianet.device/FindMulticastRange ********************************
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



/****i* nvidianet.device/SetMulticast **************************************
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
* Note that the multicast filter could be configured to accept unicast
* addresses if it allows the multicast bit to be clear.
*
*/

static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base)
{
   if(IsListEmpty(&unit->multicast_ranges))
   {
      /* Reject all addresses (except the invalid empty address) */

      unit->LELongOut(unit->card, NV_REG_MCASTLOW, 0);
      unit->LELongOut(unit->card, NV_REG_MCASTHIGH, 0);
      unit->LELongOut(unit->card, NV_REG_MCASTMASKLOW, 0xffffffff);
      unit->LELongOut(unit->card, NV_REG_MCASTMASKHIGH, 0xffff);
   }
   else
   {
      /* Accept all addresses whose multicast bit is set */

      unit->LELongOut(unit->card, NV_REG_MCASTLOW, 1);
      unit->LELongOut(unit->card, NV_REG_MCASTHIGH, 0);
      unit->LELongOut(unit->card, NV_REG_MCASTMASKLOW, 1);
      unit->LELongOut(unit->card, NV_REG_MCASTMASKHIGH, 0);
   }

   return;
}



/****i* nvidianet.device/FindTypeStats *************************************
*
*   NAME
*	FindTypeStats
*
*   SYNOPSIS
*	stats = FindTypeStats(unit, list,
*	    frame_type)
*
*	struct TypeStats *FindTypeStats(struct DevUnit *, struct MinList *,
*	    ULONG);
*
****************************************************************************
*
*/

struct TypeStats *FindTypeStats(struct DevUnit *unit, struct MinList *list,
   ULONG frame_type, struct DevBase *base)
{
   struct TypeStats *stats, *tail;
   BOOL found = FALSE;

   stats = (APTR)list->mlh_Head;
   tail = (APTR)&list->mlh_Tail;

   while(stats != tail && !found)
   {
      if(stats->frame_type == frame_type)
         found = TRUE;
      else
         stats = (APTR)stats->node.mln_Succ;
   }

   if(!found)
      stats = NULL;

   return stats;
}



/****i* nvidianet.device/FlushUnit *****************************************
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



/****i* nvidianet.device/StatusInt *****************************************
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
* It seems that timer interrupts occur at least once a second, even when
* disabled in the interrupt mask. We therefore apply the mask manually again
* here.
*
*/

static BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   struct DevBase *base;
   ULONG ints;

   base = unit->device;
   ints = unit->LELongIn(unit->card, NV_REG_INTSTATUS);
   ints &= ~NV_INTF_TIMER;

   if(ints != 0)
   {
      /* Acknowledge interrupts */

      unit->LELongOut(unit->card, NV_REG_INTSTATUS, ints);

      /* Handle interrupts */

      if((ints & (NV_INTF_TXDONE | NV_INTF_TXERR)) != 0)
      {
         if((ints & NV_INTF_TXERR) != 0)
            ReportEvents(unit,
               S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_TX, base);
         Cause(&unit->tx_end_int);
      }
      if((ints & NV_INTF_RXDONE) != 0)
         Cause(&unit->rx_int);
      if((ints & NV_INTF_LINK) != 0
         && (unit->LELongIn(unit->card, NV_REG_MIISTATUS) & MII_MASK) != 0)
      {
         unit->LELongOut(unit->card, NV_REG_MIISTATUS, MII_MASK);
         UpdateLinkSpeed(unit, base);
      }
   }

   return FALSE;
}



/****i* nvidianet.device/RXInt *********************************************
*
*   NAME
*	RXInt -- Soft interrupt for frame reception.
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
   UWORD slot, desc_offset, frame_size;
   struct DevBase *base;
   ULONG rx_status, *desc, dma_size;
   UBYTE *buffer;

   base = unit->device;
   slot = unit->rx_slot;
   desc_offset = slot * NV_DESCSIZE;
   desc = (APTR)unit->rx_descs + desc_offset;

   dma_size = NV_DESCSIZE;
   CachePostDMA(unit->rx_descs, &dma_size, 0);
   while(((rx_status = LELong(desc[NV_DESC_CONTROL]))
      & NV_DESC_CONTROLF_INUSE) == 0)
   {
      /* Distribute frame to clients */

      if((rx_status & NV_DESC_CONTROLF_ERROR) == 0)
      {
         frame_size = ((rx_status & NV_DESC_CONTROLF_LENGTH)
            >> NV_DESC_CONTROLB_LENGTH);
         buffer = unit->rx_buffers[slot];
         dma_size = FRAME_BUFFER_SIZE;
         CachePostDMA(buffer, &dma_size, 0);

         if(AddressFilter(unit, buffer + ETH_FRAME_DEST, base))
         {
            unit->stats.PacketsReceived++;
            DistributeRXFrame(unit, buffer, frame_size, base);
         }
         dma_size = FRAME_BUFFER_SIZE;
         CachePreDMA(buffer, &dma_size, 0);
      }
      else
      {
         unit->stats.BadData++;
         ReportEvents(unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX,
            base);
      }

      /* Mark descriptor as free for next time */

      desc[NV_DESC_CONTROL] =
         MakeLELong(NV_DESC_CONTROLF_INUSE | MAX_FRAME_SIZE);

      /* Get next descriptor */

      slot = (slot + 1) % RX_SLOT_COUNT;
      desc_offset = slot * NV_DESCSIZE;
      desc = (APTR)unit->rx_descs + desc_offset;
   }

   /* Return */

   unit->rx_slot = slot;
   return;
}



/****i* nvidianet.device/DistributeRXFrame *********************************
*
*   NAME
*	DistributeRXFrame -- Send a frame to all appropriate destinations.
*
*   SYNOPSIS
*	DistributeRXFrame(unit, frame)
*
*	VOID DistributeRXFrame(struct DevUnit *, UBYTE *);
*
****************************************************************************
*
*/

static VOID DistributeRXFrame(struct DevUnit *unit, const UBYTE *frame,
   UWORD frame_size, struct DevBase *base)
{
   BOOL is_orphan = TRUE, accepted;
   ULONG frame_type;
   struct IOSana2Req *request, *request_tail;
   struct Opener *opener, *opener_tail;
   struct TypeStats *tracker;

   /* Offer frame to every opener */

   opener = (APTR)unit->openers.mlh_Head;
   opener_tail = (APTR)&unit->openers.mlh_Tail;
   frame_type = BEWord(*((UWORD *)(frame + ETH_FRAME_TYPE)));

   while(opener != opener_tail)
   {
      request = (APTR)opener->read_port.mp_MsgList.lh_Head;
      request_tail = (APTR)&opener->read_port.mp_MsgList.lh_Tail;
      accepted = FALSE;

      /* Offer frame to each request until it's accepted */

      while(request != request_tail && !accepted)
      {
         if(request->ios2_PacketType == frame_type)
         {
            CopyFrame(unit, request, frame_size, frame_type,
               frame, base);
            accepted = TRUE;
         }
         request =
            (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;
      }

      if(accepted)
         is_orphan = FALSE;
      opener = (APTR)opener->node.mln_Succ;
   }

   /* If frame was unwanted, give it to S2_READORPHAN request */

   if(is_orphan)
   {
      unit->stats.UnknownTypesReceived++;
      if(!IsMsgPortEmpty(unit->request_ports[ADOPT_QUEUE]))
      {
         CopyFrame(unit,
            (APTR)unit->request_ports[ADOPT_QUEUE]->mp_MsgList.lh_Head,
            frame_size, frame_type, frame, base);
      }
   }

   /* Update remaining statistics */

   if(frame_type <= ETH_MTU)
      frame_type = ETH_MTU;
   tracker =
      FindTypeStats(unit, &unit->type_trackers, frame_type, base);
   if(tracker != NULL)
   {
      tracker->stats.PacketsReceived++;
      tracker->stats.BytesReceived += frame_size;
   }

   return;
}



/****i* nvidianet.device/CopyFrame *****************************************
*
*   NAME
*	CopyFrame -- Copy frame to client's buffer.
*
*   SYNOPSIS
*	CopyFrame(unit, request, frame_size, frame_type,
*	    buffer)
*
*	VOID CopyFrame(struct DevUnit *, struct IOSana2Req *, UWORD, UWORD,
*	    UBYTE *);
*
****************************************************************************
*
*/

static VOID CopyFrame(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD frame_size, UWORD frame_type, const UBYTE *buffer,
   struct DevBase *base)
{
   struct Opener *opener;
   BOOL filtered = FALSE;

   /* Set multicast and broadcast flags */

   request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
   if((*((ULONG *)(buffer + ETH_FRAME_DEST)) == 0xffffffff) &&
      (*((UWORD *)(buffer + ETH_FRAME_DEST + 4)) == 0xffff))
      request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
   else if((buffer[ETH_FRAME_DEST] & 0x1) != 0)
      request->ios2_Req.io_Flags |= SANA2IOF_MCAST;

   /* Set source and destination addresses and frame type */

   CopyMem(buffer + ETH_FRAME_SOURCE, request->ios2_SrcAddr,
      ETH_ADDRESSSIZE);
   CopyMem(buffer + ETH_FRAME_DEST, request->ios2_DstAddr,
      ETH_ADDRESSSIZE);
   request->ios2_PacketType = frame_type;

   /* Adjust for cooked frame request */

   if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
   {
      frame_size -= ETH_FRAME_DATA;
      buffer += ETH_FRAME_DATA;
   }
#ifdef USE_HACKS
   else
      frame_size += 4;   /* Needed for Shapeshifter & Fusion */
#endif
   request->ios2_DataLength = frame_size;

   /* Filter frame */

   opener = request->ios2_BufferManagement;
   if(request->ios2_Req.io_Command == CMD_READ &&
      opener->filter_hook != NULL)
      if(!CallHookPkt(opener->filter_hook, request, (APTR)buffer))
         filtered = TRUE;

   if(!filtered)
   {
      /* Copy frame into opener's buffer and reply request */

      if(!opener->rx_function(request->ios2_Data, (APTR)buffer,
         frame_size))
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



/****i* nvidianet.device/AddressFilter *************************************
*
*   NAME
*	AddressFilter -- Determine if an RX frame should be accepted.
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



/****i* nvidianet.device/TXInt *********************************************
*
*   NAME
*	TXInt -- Soft interrupt for frame transmission.
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
* Each TX DMA "slot" has a pair of descriptors. Regardless of whether a
* frame is raw or not, the Ethernet header is contained in the first
* descriptor, and the frame body in the second descriptor. This is because
* we want to have a consistent number of descriptors per slot, but the
* hardware does not allow any descriptor to have zero length or be skipped.
*
*/

static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   struct DevBase *base;
   UWORD data_size, slot, next_slot, *p, *q, i, desc_offset;
   struct IOSana2Req *request;
   BOOL proceed = TRUE;
   struct Opener *opener;
   ULONG wire_error, *header_desc, *data_desc, dma_size, control_value;
   UQUAD buffer_p;
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   BYTE error;
   UBYTE *header, *data;
   struct MsgPort *port;

   base = unit->device;
   port = unit->request_ports[WRITE_QUEUE];

   while(proceed && !IsMsgPortEmpty(port))
   {
      slot = unit->tx_in_slot;
      next_slot = (slot + 1) % TX_SLOT_COUNT;

      if(next_slot != unit->tx_out_slot
         && (unit->flags & UNITF_TXBUFFERINUSE) == 0)
      {
         error = 0;

         /* Get request and DMA frame descriptor */

         request = (APTR)port->mp_MsgList.lh_Head;

         Remove((APTR)request);
         unit->tx_requests[slot] = request;
         desc_offset = slot * NV_DESCSIZE * 2;
         header_desc = (APTR)unit->tx_descs + desc_offset;
         data_desc = header_desc + NV_DESCSIZE / sizeof(ULONG);

         /* Get frame data */

         data_size = request->ios2_DataLength;
         opener = (APTR)request->ios2_BufferManagement;
         dma_tx_function = opener->dma_tx_function;
         if(dma_tx_function != NULL)
            data = dma_tx_function(request->ios2_Data);
         else
            data = NULL;

         if(data == NULL)
         {
            data = unit->tx_buffer;
            if(opener->tx_function(data, request->ios2_Data,
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
         unit->tx_buffers[slot] = data;

         if(error == 0)
         {
            /* Get Ethernet header */

            header = unit->tx_headers + HEADER_BUFFER_SIZE * slot;
            if((request->ios2_Req.io_Flags & SANA2IOF_RAW) != 0)
            {
               /* Copy header from raw frame */

               CopyMem(data, unit->tx_headers + HEADER_BUFFER_SIZE * slot,
                  ETH_HEADERSIZE);
               data += ETH_HEADERSIZE;
               data_size -= ETH_HEADERSIZE;
            }
            else
            {
               /* Generate header */

               p = (UWORD *)header;
               for(i = 0, q = (UWORD *)request->ios2_DstAddr;
                  i < ETH_ADDRESSSIZE / 2; i++)
                  *p++ = *q++;
               for(i = 0, q = (UWORD *)unit->address;
                  i < ETH_ADDRESSSIZE / 2; i++)
                  *p++ = *q++;
               *p++ = MakeBEWord(request->ios2_PacketType);
            }

            /* Set up frame body in second descriptor */

            control_value = NV_DESC_CONTROLF_LASTFRAG
               | NV_DESC_CONTROLF_TXVALID | NV_DESC_CONTROLF_INT
               | data_size - 1;
            data_desc[NV_DESC_CONTROL] = MakeLELong(control_value);

            dma_size = data_size;
            buffer_p = (UPINT)CachePreDMA(data, &dma_size,
               DMA_ReadFromRAM);
            data_desc[NV_DESC_BUFFERHIGH] =
               MakeLELong((ULONG)(buffer_p >> 32));
            data_desc[NV_DESC_BUFFERLOW] =
               MakeLELong((ULONG)buffer_p);
            data_desc[NV_DESC_VLAN] = 0;

            /* Set up frame header in first descriptor */

            dma_size = HEADER_BUFFER_SIZE;
            buffer_p = (UPINT)CachePreDMA(header, &dma_size,
               DMA_ReadFromRAM);

            header_desc[NV_DESC_BUFFERHIGH] =
               MakeLELong((ULONG)(buffer_p >> 32));
            header_desc[NV_DESC_BUFFERLOW] =
               MakeLELong((ULONG)buffer_p);
            header_desc[NV_DESC_VLAN] = 0;
            header_desc[NV_DESC_CONTROL] =
               MakeLELong(NV_DESC_CONTROLF_TXVALID | ETH_HEADERSIZE - 1);

            dma_size = NV_DESCSIZE * 2;
            CachePreDMA(header_desc, &dma_size, 0);

            /* Restart transmission if it had stopped */

            unit->LELongOut(unit->card, NV_REG_XCVRCTRL,
               unit->LELongIn(unit->card, NV_REG_XCVRCTRL)
               | NV_REG_XCVRCTRLF_TXPOLL);
            unit->tx_in_slot = next_slot;
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



/****i* nvidianet.device/TXEndInt ******************************************
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
*/

static VOID TXEndInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   UWORD slot, desc_offset;
   UBYTE *buffer;
   struct DevBase *base;
   ULONG *desc, dma_size;
   BOOL proceed = TRUE;

   /* Retire sent frames */

   base = unit->device;
   slot = unit->tx_out_slot;
   while(proceed)
   {
      /* Get descriptor pair */

      desc_offset = slot * NV_DESCSIZE * 2;
      desc = (APTR)unit->tx_descs + desc_offset;
      dma_size = NV_DESCSIZE * 2;
      CachePostDMA(desc, &dma_size, 0);

      /* Check that second/data descriptor is not in use */

      desc += NV_DESCSIZE / sizeof(ULONG);
      if((desc[NV_DESC_CONTROL] & MakeLELong(NV_DESC_CONTROLF_TXVALID)) == 0
         && slot != unit->tx_in_slot)
      {
         /* Mark end of DMA on header buffer */

         buffer = unit->tx_headers + HEADER_BUFFER_SIZE * slot;
         dma_size = HEADER_BUFFER_SIZE;
         CachePostDMA(buffer, &dma_size, DMA_ReadFromRAM);

         /* Mark end of DMA on data buffer */

         buffer = unit->tx_buffers[slot];
         dma_size = unit->tx_requests[slot]->ios2_DataLength;
         CachePostDMA(buffer, &dma_size, DMA_ReadFromRAM);

         /* Check if unit's buffer is now free */

         if(buffer == unit->tx_buffer)
            unit->flags &= ~UNITF_TXBUFFERINUSE;

         RetireTXSlot(unit, slot, base);

         slot = (slot + 1) % TX_SLOT_COUNT;
      }
      else
         proceed = FALSE;
   }

   unit->tx_out_slot = slot;

   /* Restart downloads if they had stopped */

   if(unit->request_ports[WRITE_QUEUE]->mp_Flags == PA_IGNORE)
      Cause(&unit->tx_int);

   return;
}



/****i* nvidianet.device/RetireTXSlot **************************************
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



/****i* nvidianet.device/ResetHandler **************************************
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
   struct DevBase *base;

   base = unit->device;
   StopAdapter(unit, base);
   DeinitialiseAdapter(unit, base);

   return;
}



/****i* nvidianet.device/ReportEvents **************************************
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



/****i* nvidianet.device/UnitTask ******************************************
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
*	Completes deferred requests.
*
****************************************************************************
*
*/

static VOID UnitTask(struct DevUnit *unit)
{
   struct DevBase *base;
   struct IORequest *request;
   struct MsgPort *port;
   ULONG signal_mask;

   base = unit->device;

   /* Activate general request port */

   port = unit->request_ports[GENERAL_QUEUE];
   port->mp_SigTask = unit->task;
   port->mp_SigBit = AllocSignal(-1);
   port->mp_Flags = PA_SIGNAL;

   signal_mask = (1 << port->mp_SigBit);

   /* Tell ourselves to check port for old messages */

   Signal(unit->task, signal_mask);

   /* Infinite loop to service requests */

   while(TRUE)
   {
      Wait(signal_mask);

      while((request = (APTR)GetMsg(port)) != NULL)
      {
         /* Service the request as soon as the unit is free */

         ObtainSemaphore(&unit->access_lock);
         ServiceRequest((APTR)request, base);
      }
   }
}



/****i* nvidianet.device/ReadMII *******************************************
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

static UWORD ReadMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   struct DevBase *base)
{
   unit->LELongOut(unit->card, NV_REG_MIICTRL,
      phy_no << NV_REG_MIICTRLB_PHYNO
      | reg_no << NV_REG_MIICTRLB_REGNO);
   while((unit->LELongIn(unit->card, NV_REG_MIICTRL)
      & NV_REG_MIICTRLF_INUSE) != 0);

   return (UWORD)unit->LELongIn(unit->card, NV_REG_MIIDATA);
}



