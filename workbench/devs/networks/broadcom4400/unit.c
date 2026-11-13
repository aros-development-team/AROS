/*

Copyright (C) 2001-2023 Neil Cafferkey

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
#include "task.h"
#include "broadcom4400.h"
#include "mii.h"

#include "unit_protos.h"
#include "request_protos.h"
#include "timer_protos.h"


#define TASK_PRIORITY 0
#define STACK_SIZE 4096
#define INT_MASK (B44_INTF_MAC | B44_INTF_TXDONE | B44_INTF_RXDONE \
   | B44_INTF_LINK | B44_INTF_RXOFLOW)
#define TX_DESC_TABLE_SIZE (B44_DESCSIZE * 2 * TX_SLOT_COUNT)
#define RX_DESC_TABLE_SIZE (B44_DESCSIZE * RX_SLOT_COUNT)
#define MAX_FRAME_SIZE (ETH_HEADERSIZE + ETH_MTU + ETH_CRCSIZE)
#define FRAME_BUFFER_SIZE (B44_RXHEADERSIZE + MAX_FRAME_SIZE)
#define HEADER_BUFFER_SIZE (ETH_HEADERSIZE + 2)
#define TX_STAT(reg) \
   tx_stats[((reg) - B44_REG_TXCNTOKBYTES) / sizeof(ULONG)]
#define RX_STAT(reg) \
   rx_stats[((reg) - B44_REG_RXCNTOKBYTES) / sizeof(ULONG)]

static BOOL InitialiseAdapter(struct DevUnit *unit, struct DevBase *base);
static VOID DeinitialiseAdapter(struct DevUnit *unit, struct DevBase *base);
static struct AddressRange *FindMulticastRange(struct DevUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
   UWORD upper_bound_right, struct DevBase *base);
static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base);
static BOOL StatusInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID RXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID DistributeRXFrame(struct DevUnit *unit, const UBYTE *frame,
   UWORD frame_size, struct DevBase *base);
static VOID CopyFrame(struct DevUnit *unit, struct IOSana2Req *request,
   UWORD frame_size, UWORD frame_type, const UBYTE *buffer,
   struct DevBase *base);
static BOOL AddressFilter(struct DevUnit *unit, UBYTE *address,
   struct DevBase *base);
static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID TXEndInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code));
static VOID RetireTXSlot(struct DevUnit *unit, UWORD slot,
   struct DevBase *base);
static VOID RXOverflowInt(REG(a1, struct DevUnit *unit),
   REG(a5, APTR int_code));
static VOID ResetHandler(REG(a1, struct DevUnit *unit),
   REG(a5, APTR int_code));
static VOID ReportEvents(struct DevUnit *unit, ULONG events,
   struct DevBase *base);
static VOID UnitTask(struct DevUnit *unit);
UWORD ReadMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   struct DevBase *base);
static VOID WriteMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   UWORD value, struct DevBase *base);


/****i* broadcom4400.device/CreateUnit *************************************
*
*   NAME
*	CreateUnit -- Create a unit.
*
*   SYNOPSIS
*	unit = CreateUnit(index, io_base, id, card,
*	    io_tags)
*
*	struct DevUnit *CreateUnit(ULONG, APTR, UWORD, APTR,
*	    struct TagItem *);
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
   ULONG *desc, *descs, buffer_p, dma_size;
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

      unit->LEWordIn =
         (APTR)GetTagData(IOTAG_LEWordIn, (UPINT)NULL, io_tags);
      unit->LELongIn =
         (APTR)GetTagData(IOTAG_LELongIn, (UPINT)NULL, io_tags);
      unit->LELongOut =
         (APTR)GetTagData(IOTAG_LELongOut, (UPINT)NULL, io_tags);
      unit->AllocDMAMem =
         (APTR)GetTagData(IOTAG_AllocDMAMem, (UPINT)NULL, io_tags);
      unit->FreeDMAMem =
         (APTR)GetTagData(IOTAG_FreeDMAMem, (UPINT)NULL, io_tags);
      if(unit->LEWordIn == NULL
         || unit->LELongIn == NULL
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
         unit->AllocDMAMem(unit->card, TX_DESC_TABLE_SIZE, 4096);
      if(descs == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Get physical address of TX descriptors */

      dma_size = TX_DESC_TABLE_SIZE;
      unit->tx_descs_p = (ULONG)(UPINT)CachePreDMA(descs, &dma_size, 0);
      if(dma_size != TX_DESC_TABLE_SIZE)
         success = FALSE;
      CachePostDMA(descs, &dma_size, 0);
   }

   if(success)
   {
      /* Allocate RX descriptors */

      unit->rx_descs = descs =
         unit->AllocDMAMem(unit->card, RX_DESC_TABLE_SIZE, 4096);
      if(descs == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Get physical address of RX descriptors */

      dma_size = RX_DESC_TABLE_SIZE;
      unit->rx_descs_p = (ULONG)(UPINT)CachePreDMA(descs, &dma_size, 0);
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
         unit->rx_buffers[i] = unit->AllocDMAMem(unit->card,
            FRAME_BUFFER_SIZE, 1);
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
      /* Initialise RX ring */

      desc = unit->rx_descs;
      for(i = 0; i < RX_SLOT_COUNT; i++)
      {
         desc[B44_DESC_CONTROL] = MakeLELong(FRAME_BUFFER_SIZE);
         if(i == RX_SLOT_COUNT - 1)
            desc[B44_DESC_CONTROL] |=
               MakeLELong(B44_DESC_CONTROLF_LASTDESC);
         dma_size = FRAME_BUFFER_SIZE;
         buffer_p =
            (ULONG)(UPINT)CachePreDMA(unit->rx_buffers[i], &dma_size, 0);
         if(dma_size != FRAME_BUFFER_SIZE)
            success = FALSE;
         desc[B44_DESC_BUFFER] = MakeLELong(buffer_p + B44_DMAOFFSET);
         desc += B44_DESCSIZE / sizeof(ULONG);
      }
      dma_size = RX_DESC_TABLE_SIZE;
      CachePreDMA(unit->rx_descs, &dma_size, 0);
   }

   if(success)
   {
      /* Initialise network adapter hardware */

      success = InitialiseAdapter(unit, base);
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

      unit->rx_overflow_int.is_Node.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      unit->rx_overflow_int.is_Code = (APTR)RXOverflowInt;
      unit->rx_overflow_int.is_Data = unit;

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



/****i* broadcom4400.device/DeleteUnit *************************************
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

      if((unit->flags & UNITF_HAVEADAPTER) != 0)
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



/****i* broadcom4400.device/InitialiseAdapter ******************************
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
   UBYTE *p;
   UWORD i, address_part;
   ULONG value;

   /* Get default MAC address */

   p = unit->default_address;

   for(i = 0; i < ETH_ADDRESSSIZE / 2; i++)
   {
      address_part = unit->LEWordIn(unit->card, B44_ROM_ADDRESS + i * 2);
      *p++ = address_part >> 8;
      *p++ = address_part & 0xff;
   }

   /* Reset adapter */

   if((unit->LELongIn(unit->card, B44_REG_CORECTRL)
      & B44_REG_CORECTRLF_RESET) == 0)
   {
      if((unit->LELongIn(unit->card, B44_REG_CORECTRL)
         & B44_REG_CORECTRLF_CLKON) != 0)
      {
         unit->LELongOut(unit->card, B44_REG_CORECTRL,
            B44_REG_CORECTRLF_CLKON | B44_REG_CORECTRLF_REJECT);
         while((unit->LELongIn(unit->card, B44_REG_CORECTRL)
            & B44_REG_CORECTRLF_REJECT) == 0);
         while((unit->LELongIn(unit->card, B44_REG_CORESTATUS)
            & B44_REG_CORESTATUSF_BUSY) != 0);

         if((unit->LELongIn(unit->card, B44_REG_COREIDLOW)
            & B44_REG_COREIDLOWF_INITIATOR) != 0)
         {
            value = unit->LELongIn(unit->card, B44_REG_COREISTATE);
            value |= B44_REG_COREISTATEF_REJECT;
            unit->LELongOut(unit->card, B44_REG_COREISTATE, value);
            while((unit->LELongIn(unit->card, B44_REG_COREISTATE)
               & B44_REG_COREISTATEF_BUSY) != 0);
         }

         unit->LELongOut(unit->card, B44_REG_CORECTRL,
            B44_REG_CORECTRLF_GATEDCLKSON | B44_REG_CORECTRLF_CLKON
            | B44_REG_CORECTRLF_REJECT | B44_REG_CORECTRLF_RESET);
         unit->LELongIn(unit->card, B44_REG_CORECTRL);
         BusyMicroDelay(1, base);

         if((unit->LELongIn(unit->card, B44_REG_COREIDLOW)
            & B44_REG_COREIDLOWF_INITIATOR) != 0)
         {
            value = unit->LELongIn(unit->card, B44_REG_COREISTATE);
            value &= ~B44_REG_COREISTATEF_REJECT;
            unit->LELongOut(unit->card, B44_REG_COREISTATE, value);
         }
      }
      unit->LELongOut(unit->card, B44_REG_CORECTRL,
         B44_REG_CORECTRLF_REJECT | B44_REG_CORECTRLF_RESET);
      unit->LELongIn(unit->card, B44_REG_CORECTRL);
      BusyMicroDelay(1, base);
   }

   unit->LELongOut(unit->card, B44_REG_CORECTRL,
      B44_REG_CORECTRLF_GATEDCLKSON | B44_REG_CORECTRLF_CLKON
      | B44_REG_CORECTRLF_RESET);
   unit->LELongIn(unit->card, B44_REG_CORECTRL);
   BusyMicroDelay(1, base);

   if((unit->LELongIn(unit->card, B44_REG_CORESTATUS)
      & B44_REG_CORESTATUSF_SLAVEERR) != 0)
      unit->LELongOut(unit->card, B44_REG_CORESTATUS, 0);

   value = unit->LELongIn(unit->card, B44_REG_COREISTATE);
   if((value & (B44_REG_COREISTATEF_TIMEOUT | B44_REG_COREISTATEF_INBANDERR))
      != 0)
      unit->LELongOut(unit->card, B44_REG_COREISTATE, value
         & ~(B44_REG_COREISTATEF_TIMEOUT | B44_REG_COREISTATEF_INBANDERR));

   unit->LELongOut(unit->card, B44_REG_CORECTRL,
      B44_REG_CORECTRLF_GATEDCLKSON | B44_REG_CORECTRLF_CLKON);
   unit->LELongIn(unit->card, B44_REG_CORECTRL);
   BusyMicroDelay(1, base);
   unit->LELongOut(unit->card, B44_REG_CORECTRL,
      B44_REG_CORECTRLF_CLKON);
   unit->LELongIn(unit->card, B44_REG_CORECTRL);
   BusyMicroDelay(1, base);

   /* Clear statistics registers */

   unit->LELongOut(unit->card, B44_REG_STATSCTRL,
      B44_REG_STATSCTRLF_CLEARONREAD);
   for(i = 0; i < B44_TXSTATSCOUNT; i++)
      unit->LELongIn(unit->card, B44_REG_TXCNTOKBYTES + i * 4);
   for(i = 0; i < B44_RXSTATSCOUNT; i++)
      unit->LELongIn(unit->card, B44_REG_RXCNTOKBYTES + i * 4);

   /* Configure MII/PHY access */

   unit->LELongOut(unit->card, B44_REG_MIICTRL,
      B44_REG_MIICTRLF_PREAMBLE | 14);
   unit->LELongIn(unit->card, B44_REG_MIICTRL);

   if((unit->LELongIn(unit->card, B44_REG_DEVCTRL)
      & B44_REG_DEVCTRLF_INTPHY) != 0)
   {
      value = unit->LELongIn(unit->card, B44_REG_DEVCTRL);
      if((value & B44_REG_DEVCTRLF_PHYRESET) != 0)
      {
         value &= ~B44_REG_DEVCTRLF_PHYRESET;
         unit->LELongOut(unit->card, B44_REG_DEVCTRL, value);
         unit->LELongIn(unit->card, B44_REG_DEVCTRL);
         BusyMicroDelay(100, base);
      }

      unit->mii_phy_no = unit->LELongIn(unit->card, B44_ROM_MII)
         & B44_ROM_MIIF_PHYNO0;

      /* Configure PHY */

      WriteMII(unit, unit->mii_phy_no, MII_CONTROL, MII_CONTROLF_RESET, base);
      BusyMicroDelay(100, base);
   }
   else
   {
      unit->LELongOut(unit->card, B44_REG_ENETCTRL,
         B44_REG_ENETCTRLF_EXTPHY);
      unit->LELongIn(unit->card, B44_REG_ENETCTRL);
   }

   /* Set standard LED mode and enable CRC generation */

   unit->LELongOut(unit->card, B44_REG_MACCTRL,
      0x7 << B44_REG_MACCTRLB_LEDMODE | B44_REG_MACCTRLF_CRC);

   /* Generate an interrupt after every RX frame */

   unit->LELongOut(unit->card, B44_REG_LAZYRXCTRL,
      1 << B44_REG_LAZYRXCTRLB_FRAMECNT);

   /* Set DMA rings */

   unit->LELongOut(unit->card, B44_REG_TXDMALIST,
      unit->tx_descs_p + B44_DMAOFFSET);
   unit->LELongOut(unit->card, B44_REG_RXDMALIST,
      unit->rx_descs_p + B44_DMAOFFSET);

   /* Set frame-related limits and enable DMA */

   unit->LELongOut(unit->card, B44_REG_RXMAXSIZE, MAX_FRAME_SIZE);
   unit->LELongOut(unit->card, B44_REG_TXTHRESHOLD, B44_STDTXTHRESHOLD);
   unit->LELongOut(unit->card, B44_REG_TXDMACTRL,
      B44_REG_TXDMACTRLF_ENABLE);
   unit->LELongOut(unit->card, B44_REG_RXDMACTRL,
      B44_RXHEADERSIZE << B44_REG_RXDMACTRLB_HDRSIZE
      | B44_REG_RXDMACTRLF_ENABLE);

   /* Tell hardware where to stop filling RX buffers. This can't be
      initialised to zero because it can't be the same as the next
      descriptor the hardware will fill */

   unit->LELongOut(unit->card, B44_REG_RXDMALIMIT,
      (RX_SLOT_COUNT - 1) * B44_DESCSIZE);

   /* Choose frame types to receive */

   if((unit->flags & UNITF_PROM) != 0)
      unit->LELongOut(unit->card, B44_REG_RXCONFIG, B44_REG_RXCONFIGF_PROM);
   else
      SetMulticast(unit, base);

   /* Return */

   return success;
}



/****i* broadcom4400.device/DeinitialiseAdapter ****************************
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
   return;
}



/****i* broadcom4400.device/ConfigureAdapter *******************************
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
   unit->LELongOut(unit->card, B44_REG_ADDRESSHIGH,
      B44_REG_ADDRESSHIGHF_VALID | p[0] << 8 | p[1]);
   unit->LELongOut(unit->card, B44_REG_ADDRESSLOW,
      p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
   unit->LELongOut(unit->card, B44_REG_ADDRESSCTRL,
      B44_REG_ADDRESSCTRLF_WRITE);
   while((unit->LELongIn(unit->card, B44_REG_ADDRESSCTRL)
      & B44_REG_ADDRESSCTRLF_BUSY) != 0);
   unit->LELongOut(unit->card, B44_REG_ADDRESSCTRL,
      B44_REG_ADDRESSCTRLF_ON);

   /* Return */

   return;
}



/****i* broadcom4400.device/GoOnline ***************************************
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
   ULONG value;

   /* Enable interrupts */

   unit->flags |= UNITF_ONLINE;
   unit->LELongOut(unit->card, B44_REG_INTMASK, INT_MASK);
   unit->LELongOut(unit->card, B44_REG_MACINTMASK, B44_MACINTF_MIB);

   /* Enable frame transmission and reception */

   value = unit->LELongIn(unit->card, B44_REG_ENETCTRL);
   value |= B44_REG_ENETCTRLF_ENABLE;
   unit->LELongOut(unit->card, B44_REG_ENETCTRL, value);

   /* Record start time and report Online event */

   GetSysTime(&unit->stats.LastStart);
   ReportEvents(unit, S2EVENT_ONLINE, base);

   return;
}



/****i* broadcom4400.device/GoOffline **************************************
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
   ULONG value;

   unit->flags &= ~UNITF_ONLINE;

   if((unit->flags & UNITF_HAVEADAPTER) != 0)
   {
      /* Stop transmission and reception */

      value = unit->LELongIn(unit->card, B44_REG_ENETCTRL);
      value |= B44_REG_ENETCTRLF_DISABLE;
      unit->LELongOut(unit->card, B44_REG_ENETCTRL, value);

      /* Stop interrupts */

      unit->LELongOut(unit->card, B44_REG_INTMASK, 0);
      unit->LELongOut(unit->card, B44_REG_MACINTMASK, 0);
   }

   /* Flush pending read and write requests */

   FlushUnit(unit, MGMT_QUEUE, S2ERR_OUTOFSERVICE, base);

   /* Report Offline event and return */

   ReportEvents(unit, S2EVENT_OFFLINE, base);
   return;
}



/****i* broadcom4400.device/AddMulticastRange ******************************
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



/****i* broadcom4400.device/RemMulticastRange ******************************
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



/****i* broadcom4400.device/FindMulticastRange *****************************
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



/****i* broadcom4400.device/SetMulticast ***********************************
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
* The hardware has a 64-entry multicast address list that could be used but
* currently is not.
*
*/

static VOID SetMulticast(struct DevUnit *unit, struct DevBase *base)
{
   ULONG rx_config;

   rx_config = unit->LELongIn(unit->card, B44_REG_RXCONFIG);
   if(IsListEmpty(&unit->multicast_ranges))
      rx_config &= B44_REG_RXCONFIGF_ALLMCAST;
   else
      rx_config |= B44_REG_RXCONFIGF_ALLMCAST;
   unit->LELongOut(unit->card, B44_REG_RXCONFIG, rx_config);

   return;
}



/****i* broadcom4400.device/FindTypeStats **********************************
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



/****i* broadcom4400.device/FlushUnit **************************************
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



/****i* broadcom4400.device/StatusInt **************************************
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
   ULONG ints, value;

   base = unit->device;
   ints = unit->LELongIn(unit->card, B44_REG_INTSTATUS)
      & unit->LELongIn(unit->card, B44_REG_INTMASK);

   if(ints != 0)
   {
      /* Acknowledge (some) interrupts */

      unit->LELongOut(unit->card, B44_REG_INTSTATUS, ints);

      /* Handle interrupts */

      if((ints & B44_INTF_MAC) != 0)
      {
         UpdateStats(unit, base);
         unit->LELongOut(unit->card, B44_REG_MACINTSTATUS, B44_MACINTF_MIB);
      }
      if((ints & B44_INTF_TXDONE) != 0)
         Cause(&unit->tx_end_int);
      if((ints & B44_INTF_RXDONE) != 0)
         Cause(&unit->rx_int);
      if((ints & B44_INTF_LINK) != 0)
      {
         /* Set duplex mode */

         value = unit->LELongIn(unit->card, B44_REG_TXCTRL);
         if((ReadMII(unit, unit->mii_phy_no, MII_AUXCTRL, base)
            & MII_AUXCTRLF_FDUPLEX) != 0)
            value |= B44_REG_TXCTRLF_FDUPLEX;
         else
            value &= ~B44_REG_TXCTRLF_FDUPLEX;
         unit->LELongOut(unit->card, B44_REG_TXCTRL, value);
      }
      if((ints & B44_INTF_RXOFLOW) != 0)
      {
         /* Disable reception to clear the interrupt, then delegate
            re-enabling reception to a software interrupt so that the RX
            software interrupt gets a chance to finish any outstanding work
            first */

         unit->LELongOut(unit->card, B44_REG_RXDMACTRL,
            B44_RXHEADERSIZE << B44_REG_RXDMACTRLB_HDRSIZE);
         Cause(&unit->rx_overflow_int);
      }
   }

   return FALSE;
}



/****i* broadcom4400.device/RXInt ******************************************
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
   UWORD slot, new_slot, frame_size;
   struct DevBase *base;
   ULONG rx_status, dma_size;
   UBYTE *buffer, *frame;

   base = unit->device;
   slot = unit->rx_slot;
   new_slot = (unit->LELongIn(unit->card, B44_REG_RXDMASTATUS)
      & B44_REG_RXDMASTATUSF_DESC) / B44_DESCSIZE;

   while(slot != new_slot)
   {
      /* Get buffer and check for errors */

      buffer = unit->rx_buffers[slot];
      rx_status = LELong(((ULONG *)buffer)[B44_HDR_STATUS]);
      if((rx_status & (B44_HDR_STATUSF_OSIZE | B44_HDR_STATUSF_ODD
         | B44_HDR_STATUSF_BADSYM | B44_HDR_STATUSF_BADCRC
         | B44_HDR_STATUSF_OFLOW)) == 0)
      {
         dma_size = FRAME_BUFFER_SIZE;
         CachePostDMA(buffer, &dma_size, 0);

         frame = buffer + B44_RXHEADERSIZE;
         frame_size = ((rx_status & B44_HDR_STATUSF_LENGTH)
            >> B44_HDR_STATUSB_LENGTH) - ETH_CRCSIZE;
         if(AddressFilter(unit, frame + ETH_FRAME_DEST, base))
            DistributeRXFrame(unit, frame, frame_size, base);
         dma_size = FRAME_BUFFER_SIZE;
         CachePreDMA(buffer, &dma_size, 0);
      }
      else
      {
         ReportEvents(unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX,
            base);
      }

      slot = (slot + 1) % RX_SLOT_COUNT;
   }

   /* Tell hardware where to stop filling RX buffers */

   unit->LELongOut(unit->card, B44_REG_RXDMALIMIT, slot * B44_DESCSIZE);

   /* Return */

   unit->rx_slot = slot;
   return;
}



/****i* broadcom4400.device/DistributeRXFrame ******************************
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



/****i* broadcom4400.device/CopyFrame **************************************
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



/****i* broadcom4400.device/AddressFilter **********************************
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



/****i* broadcom4400.device/TXInt ******************************************
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
* Each TX DMA "slot" has a pair of descriptors. When sending a "cooked"
* frame, the Ethernet header is contained in the first descriptor, and the
* frame body in the second descriptor. When sending a "raw" frame, the
* first descriptor is empty, and the entire frame is contained in the
* second descriptor. Note that leaving the second descriptor empty is
* disallowed, and there is no method described to skip a descriptor.
*
*/

static VOID TXInt(REG(a1, struct DevUnit *unit), REG(a5, APTR int_code))
{
   struct DevBase *base;
   UWORD data_size, slot, next_slot, *p, *q, i, desc_offset;
   struct IOSana2Req *request;
   BOOL proceed = TRUE;
   struct Opener *opener;
   ULONG wire_error, *header_desc, *data_desc, dma_size, buffer_p,
      control_value;
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   BYTE error;
   UBYTE *buffer;
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
         desc_offset = slot * B44_DESCSIZE * 2;
         header_desc = (APTR)unit->tx_descs + desc_offset;
         data_desc = header_desc + B44_DESCSIZE / sizeof(ULONG);

         /* Write first descriptor */

         if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
         {
            /* Write Ethernet header */

            buffer = unit->tx_headers + HEADER_BUFFER_SIZE * slot;
            p = (UWORD *)buffer;
            for(i = 0, q = (UWORD *)request->ios2_DstAddr;
               i < ETH_ADDRESSSIZE / 2; i++)
               *p++ = *q++;
            for(i = 0, q = (UWORD *)unit->address;
               i < ETH_ADDRESSSIZE / 2; i++)
               *p++ = *q++;
            *p++ = MakeBEWord(request->ios2_PacketType);

            /* Use first descriptor for Ethernet header */

            dma_size = HEADER_BUFFER_SIZE;
            buffer_p = (ULONG)(UPINT)CachePreDMA(buffer, &dma_size,
               DMA_ReadFromRAM) + B44_DMAOFFSET;
            control_value =
               MakeLELong(B44_DESC_CONTROLF_FIRSTFRAG | ETH_HEADERSIZE);
         }
         else
         {
            /* Use empty first descriptor */

            buffer_p = 0;
            control_value = MakeLELong(B44_DESC_CONTROLF_FIRSTFRAG);
         }

         header_desc[B44_DESC_CONTROL] = control_value;
         header_desc[B44_DESC_BUFFER] = MakeLELong(buffer_p);

         /* Get frame data */

         data_size = request->ios2_DataLength;
         opener = (APTR)request->ios2_BufferManagement;
         dma_tx_function = opener->dma_tx_function;
         if(dma_tx_function != NULL)
            buffer = dma_tx_function(request->ios2_Data);
         else
            buffer = NULL;

         if(buffer == NULL || (UPINT)buffer >= B44_DMALIMIT)
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

         /* Fill main frame data into second descriptor */

         if(error == 0)
         {
            control_value = B44_DESC_CONTROLF_LASTFRAG
               | B44_DESC_CONTROLF_INT
               | data_size;
            if(next_slot == 0)
               control_value |= B44_DESC_CONTROLF_LASTDESC;
            data_desc[B44_DESC_CONTROL] = MakeLELong(control_value);

            dma_size = data_size;
            buffer_p = (ULONG)(UPINT)CachePreDMA(buffer, &dma_size,
               DMA_ReadFromRAM) + B44_DMAOFFSET;
            data_desc[B44_DESC_BUFFER] = MakeLELong(buffer_p);

            dma_size = B44_DESCSIZE * 2;
            CachePreDMA(header_desc, &dma_size, 0);

            /* Tell hardware where to stop reading TX buffers */

            unit->tx_in_slot = next_slot;
            desc_offset = next_slot * B44_DESCSIZE * 2;
            unit->LELongOut(unit->card, B44_REG_TXDMALIMIT, desc_offset);
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



/****i* broadcom4400.device/TXEndInt ***************************************
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
   UWORD slot, new_slot, desc_offset;
   UBYTE *buffer;
   struct DevBase *base;
   ULONG *desc, dma_size;

   /* Get index of first descriptor pair that hasn't yet been sent */

   base = unit->device;
   new_slot = (unit->LELongIn(unit->card, B44_REG_TXDMASTATUS)
      & B44_REG_TXDMASTATUSF_DESC) / (B44_DESCSIZE * 2);

   /* Retire sent frames */

   slot = unit->tx_out_slot;
   while(slot != new_slot)
   {
      /* Get descriptor pair */

      desc_offset = slot * B44_DESCSIZE * 2;
      desc = (APTR)unit->tx_descs + desc_offset;
      dma_size = B44_DESCSIZE * 2;
      CachePostDMA(desc, &dma_size, 0);

      /* Mark end of DMA on header buffer if it was used */

      if(desc[B44_DESC_BUFFER] != 0)
      {
         buffer = unit->tx_headers + HEADER_BUFFER_SIZE * slot;
         dma_size = HEADER_BUFFER_SIZE;
         CachePostDMA(buffer, &dma_size, DMA_ReadFromRAM);
      }

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

   unit->tx_out_slot = slot;

   /* Restart downloads if they had stopped */

   if(unit->request_ports[WRITE_QUEUE]->mp_Flags == PA_IGNORE)
      Cause(&unit->tx_int);

   return;
}



/****i* broadcom4400.device/RetireTXSlot ***********************************
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

   tracker = FindTypeStats(unit, &unit->type_trackers,
      request->ios2_PacketType, base);
   if(tracker != NULL)
   {
      frame_size = request->ios2_DataLength;
      if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
         frame_size += ETH_HEADERSIZE;
      tracker->stats.PacketsSent++;
      tracker->stats.BytesSent += frame_size;
   }

   /* Reply request */

   request->ios2_Req.io_Error = 0;
   ReplyMsg((APTR)request);

   return;
}



/****i* broadcom4400.device/RXOverflowInt **********************************
*
*   NAME
*	RXOverflowInt -- Soft interrupt to recover from RX overflow.
*
*   SYNOPSIS
*	RXOverflowInt(unit)
*
*	VOID RXOverflowInt(struct DevUnit *);
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
* Sometimes the overflow interrupt isn't cleared by this function. This may
* be due to a hardware bug. Hopefully the initial overflow will be a very
* rare occurence if sufficient descriptors are provided.
*
*/

static VOID RXOverflowInt(REG(a1, struct DevUnit *unit),
   REG(a5, APTR int_code))
{
   /* Wait for reception to be disabled */

   while((unit->LELongIn(unit->card, B44_REG_RXDMASTATUS)
      & B44_REG_RXDMASTATUSF_STATE) != 0);

   /* Re-enable reception */

   unit->LELongOut(unit->card, B44_REG_RXDMACTRL,
      B44_RXHEADERSIZE << B44_REG_RXDMACTRLB_HDRSIZE
      | B44_REG_RXDMACTRLF_ENABLE);

   /* Tell hardware where to stop filling RX buffers. This can't be
      initialised to zero because it can't be the same as the next
      descriptor the hardware will fill */

   unit->LELongOut(unit->card, B44_REG_RXDMALIMIT,
      (RX_SLOT_COUNT - 1) * B44_DESCSIZE);
   unit->rx_slot = 0;

   /* Return */

   return;
}



/****i* broadcom4400.device/UpdateStats ************************************
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
* All statistics registers are read before cherry-picking the interesting
* values, for these reasons:
* 1. Uninteresting registers will be cleared to avoid unnecessary interrupts
*    when they reach their maximum value.
* 2. Registers won't be updated between clearing them and reading them.
* 3. Uninteresting registers don't have to be cleared separately from
*    interesting ones.
*
*/

VOID UpdateStats(struct DevUnit *unit, struct DevBase *base)
{
   UWORD i;
   ULONG tx_stats[B44_TXSTATSCOUNT], rx_stats[B44_RXSTATSCOUNT], *p;

   /* Read and clear all statistics registers */

   for(i = 0, p = tx_stats; i < B44_TXSTATSCOUNT; i++)
      *p++ = unit->LELongIn(unit->card,
         B44_REG_TXCNTOKBYTES + i * sizeof(ULONG));
   for(i = 0, p = rx_stats; i < B44_RXSTATSCOUNT; i++)
      *p++ = unit->LELongIn(unit->card,
         B44_REG_RXCNTOKBYTES + i * sizeof(ULONG));

   /* Update our statistics counters from data read */

   unit->stats.Overruns += RX_STAT(B44_REG_RXCNTOVERRUN);
   unit->stats.PacketsSent += TX_STAT(B44_REG_TXCNTOK);
   unit->stats.PacketsReceived += RX_STAT(B44_REG_RXCNTOK);
   unit->stats.BadData +=
      RX_STAT(B44_REG_RXCNTALL) - RX_STAT(B44_REG_RXCNTOK);
   unit->special_stats[S2SS_ETHERNET_RETRIES & 0xffff] +=
      TX_STAT(B44_REG_TXCNTCOLLISION);
   unit->special_stats[S2SS_ETHERNET_FIFO_UNDERRUNS & 0xffff] +=
      TX_STAT(B44_REG_TXCNTUNDERRUN);

   return;
}



/****i* broadcom4400.device/ResetHandler ***********************************
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

      unit->LELongOut(unit->card, B44_REG_ENETCTRL,
         B44_REG_ENETCTRLF_DISABLE);

      /* Stop interrupts */

      unit->LELongOut(unit->card, B44_REG_INTMASK, 0);
   }

   return;
}



/****i* broadcom4400.device/ReportEvents ***********************************
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



/****i* broadcom4400.device/UnitTask ***************************************
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



/****i* broadcom4400.device/ReadMII ****************************************
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
   unit->LELongOut(unit->card, B44_REG_MACINTSTATUS, B44_MACINTF_MII);
   unit->LELongOut(unit->card, B44_REG_MIIDATA,
      B44_REG_MIIDATAF_START | B44_REG_MIIDATAF_READ
      | phy_no << B44_REG_MIIDATAB_PHYNO
      | reg_no << B44_REG_MIIDATAB_REGNO
      | B44_REG_MIIDATAF_TURNAROUND);
   while((unit->LELongIn(unit->card, B44_REG_MACINTSTATUS)
      & B44_MACINTF_MII) == 0);

   return (UWORD)unit->LELongIn(unit->card, B44_REG_MIIDATA);
}



/****i* broadcom4400.device/WriteMII ***************************************
*
*   NAME
*       WriteMII -- Write to a register on an MII PHY.
*
*   SYNOPSIS
*       WriteMII(unit, phy_no, reg_no, value)
*
*       VOID WriteMII(struct DevUnit *, UWORD, UWORD, UWORD);
*
*   INPUTS
*       unit - Device unit.
*       phy_no - .
*       reg_no - .
*       value - value to write to MII register.
*   
*   RESULT
*       None.
*
****************************************************************************
*
*/

static VOID WriteMII(struct DevUnit *unit, UWORD phy_no, UWORD reg_no,
   UWORD value, struct DevBase *base)
{
   unit->LELongOut(unit->card, B44_REG_MACINTSTATUS, B44_MACINTF_MII);
   unit->LELongOut(unit->card, B44_REG_MIIDATA,
      B44_REG_MIIDATAF_START | B44_REG_MIIDATAF_WRITE
      | phy_no << B44_REG_MIIDATAB_PHYNO
      | reg_no << B44_REG_MIIDATAB_REGNO
      | B44_REG_MIIDATAF_TURNAROUND | value);
   while((unit->LELongIn(unit->card, B44_REG_MACINTSTATUS)
      & B44_MACINTF_MII) == 0);

   return;
}



