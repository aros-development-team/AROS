/*

Copyright (C) 2004-2012 Neil Cafferkey

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
#include <utility/tagitem.h>
#include <libraries/prometheus.h>

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/prometheus.h>

#include "device.h"

#include "pci_protos.h"
#include "device_protos.h"
#include "unit_protos.h"

#define BAR_NO 0


struct BusContext
{
   struct DevUnit *unit;
   struct DevBase *device;
   VOID *card;
   UPINT io_base;
   struct TagItem *unit_tags;
   BOOL have_card;
   UWORD generation;
};


/* Private prototypes */

static struct DevUnit *FindPCIUnit(ULONG index, struct DevBase *base);
static struct DevUnit *CreatePCIUnit(ULONG index, struct DevBase *base);
static struct BusContext *AllocCard(ULONG index, struct DevBase *base);
static VOID FreeCard(struct BusContext *context, struct DevBase *base);
static BOOL AddPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base);
static VOID RemPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base);
static UBYTE ByteInHook(struct BusContext *context, ULONG offset);
static VOID ByteOutHook(struct BusContext *context, ULONG offset,
   UBYTE value);
static UWORD LEWordInHook(struct BusContext *context, ULONG offset);
static ULONG LELongInHook(struct BusContext *context, ULONG offset);
static VOID LEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value);
static VOID LELongOutHook(struct BusContext *context, ULONG offset,
   ULONG value);
static APTR AllocDMAMemHook(struct BusContext *context, UPINT size,
   UWORD alignment);
static VOID FreeDMAMemHook(struct BusContext *context, APTR mem);


const UWORD product_codes[] =
{
   0x1106, 0x3043,
   0x1106, 0x3053,
   0x1106, 0x3065,
   0x1106, 0x3106,
   0x1106, 0x6100,
   0x1500, 0x1320,
   0x4033, 0x1320,
   0xffff, 0xffff
};


static struct TagItem unit_tags[] =
{
   {IOTAG_ByteIn, (UPINT)ByteInHook},
   {IOTAG_ByteOut, (UPINT)ByteOutHook},
   {IOTAG_LEWordIn, (UPINT)LEWordInHook},
   {IOTAG_LELongIn, (UPINT)LELongInHook},
   {IOTAG_LEWordOut, (UPINT)LEWordOutHook},
   {IOTAG_LELongOut, (UPINT)LELongOutHook},
   {IOTAG_AllocDMAMem, (UPINT)AllocDMAMemHook},
   {IOTAG_FreeDMAMem, (UPINT)FreeDMAMemHook},
   {TAG_END, 0}
};


/****i* rhine.device/GetPCICount *******************************************
*
*   NAME
*	GetPCICount -- Get the number of compatible PCI Cards.
*
*   SYNOPSIS
*	count = GetPCICount()
*
*	ULONG GetPCICount();
*
****************************************************************************
*
*/

ULONG GetPCICount(struct DevBase *base)
{
   ULONG count = 0;
   PCIBoard *card = NULL;
   UPINT vendor_id, product_id;

   while((card = Prm_FindBoardTagList(card, NULL)) != NULL)
   {
      Prm_GetBoardAttrsTags(card, PRM_Vendor, (UPINT)&vendor_id,
         PRM_Device, (UPINT)&product_id, TAG_END);
      if(IsCardCompatible(vendor_id, product_id, base))
         count++;
   }

   return count;
}



/****i* rhine.device/GetPCIUnit ********************************************
*
*   NAME
*	GetPCIUnit -- Get a unit by number.
*
*   SYNOPSIS
*	unit = GetPCIUnit(index)
*
*	struct DevUnit *GetPCIUnit(ULONG);
*
****************************************************************************
*
*/

struct DevUnit *GetPCIUnit(ULONG index, struct DevBase *base)
{
   struct DevUnit *unit;

   unit = FindPCIUnit(index, base);

   if(unit == NULL)
   {
      unit = CreatePCIUnit(index, base);
      if(unit != NULL)
      {
         AddTail((APTR)&base->pci_units, (APTR)unit);
      }
   }

   return unit;
}



/****i* rhine.device/FindPCIUnit *******************************************
*
*   NAME
*	FindPCIUnit -- Find a unit by number.
*
*   SYNOPSIS
*	unit = FindPCIUnit(index)
*
*	struct DevUnit *FindPCIUnit(ULONG);
*
****************************************************************************
*
*/

static struct DevUnit *FindPCIUnit(ULONG index, struct DevBase *base)
{
   struct DevUnit *unit, *tail;
   BOOL found = FALSE;

   unit = (APTR)base->pci_units.mlh_Head;
   tail = (APTR)&base->pci_units.mlh_Tail;

   while(unit != tail && !found)
   {
      if(unit->index == index)
         found = TRUE;
      else
         unit = (APTR)unit->node.mln_Succ;
   }

   if(!found)
      unit = NULL;

   return unit;
}



/****i* rhine.device/CreatePCIUnit *****************************************
*
*   NAME
*	CreatePCIUnit -- Create a PCI unit.
*
*   SYNOPSIS
*	unit = CreatePCIUnit(index)
*
*	struct DevUnit *CreatePCIUnit(ULONG);
*
*   FUNCTION
*	Creates a new PCI unit.
*
****************************************************************************
*
*/

static struct DevUnit *CreatePCIUnit(ULONG index, struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   struct DevUnit *unit = NULL;

   context = AllocCard(index, base);
   if(context == NULL)
      success = FALSE;

   if(success)
   {
      if(context->unit_tags == NULL)
         context->unit_tags = unit_tags;
      context->device = base;
      context->unit = unit = CreateUnit(index, context, context->unit_tags,
         PCI_BUS, base);
      if(unit == NULL)
         success = FALSE;
   }

   /* Add interrupt */

   if(success)
   {
      if(!(WrapInt(&unit->status_int, base)
         && WrapInt(&unit->rx_int, base)
         && WrapInt(&unit->tx_int, base)
         && WrapInt(&unit->tx_end_int, base)
         && WrapInt(&unit->reset_handler, base)))
         success = FALSE;
   }

   /* Add hardware interrupt and reset handler */

   if(success)
   {
      if(AddPCIIntServer(context->card, &unit->status_int, base))
         unit->flags |= UNITF_INTADDED;
      else
         success = FALSE;

      if(AddResetCallback(&unit->reset_handler))
         unit->flags |= UNITF_RESETADDED;
      else
         success = FALSE;
   }

   if(!success)
   {
      if(context != NULL)
      {
         DeleteUnit(context->unit, base);
         FreeCard(context, base);
      }
      unit = NULL;
   }

   return unit;
}



/****i* rhine.device/DeletePCIUnit *****************************************
*
*   NAME
*	DeletePCIUnit -- Delete a unit.
*
*   SYNOPSIS
*	DeletePCIUnit(unit)
*
*	VOID DeletePCIUnit(struct DevUnit *);
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

VOID DeletePCIUnit(struct DevUnit *unit, struct DevBase *base)
{
   struct BusContext *context;

   if(unit != NULL)
   {
      context = unit->card;
      if((unit->flags & UNITF_RESETADDED) != 0)
         RemResetCallback(&unit->reset_handler);
      if((unit->flags & UNITF_INTADDED) != 0)
         RemPCIIntServer(context->card, &unit->status_int, base);
      UnwrapInt(&unit->reset_handler, base);
      UnwrapInt(&unit->tx_end_int, base);
      UnwrapInt(&unit->tx_int, base);
      UnwrapInt(&unit->rx_int, base);
      UnwrapInt(&unit->status_int, base);
      DeleteUnit(unit, base);
      FreeCard(context, base);
   }

   return;
}



/****i* rhine.device/AllocCard *********************************************
*
*   NAME
*	AllocCard -- Get card from system.
*
*   SYNOPSIS
*	context = AllocCard(index)
*
*	struct BusContext *AllocCard(ULONG);
*
****************************************************************************
*
*/

static struct BusContext *AllocCard(ULONG index, struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   PCIBoard *card = NULL;
   UWORD i = 0;
   UPINT vendor_id, product_id;

   /* Find a compatible card */

   context = AllocMem(sizeof(struct BusContext), MEMF_PUBLIC | MEMF_CLEAR);
   if(context == NULL)
      success = FALSE;

   if(success)
   {
      while(i <= index)
      {
         card = Prm_FindBoardTagList(card, NULL);
         Prm_GetBoardAttrsTags(card, PRM_Vendor, (UPINT)&vendor_id,
            PRM_Device, (UPINT)&product_id, TAG_END);
         if(IsCardCompatible(vendor_id, product_id, base))
            i++;
      }

      context->card = card;
      if(card == NULL)
         success = FALSE;
   }

   /* Get base address */

   if(success)
   {
      Prm_GetBoardAttrsTags(card,
         PRM_MemoryAddr0 + BAR_NO, (UPINT)&context->io_base, TAG_END);
      if(context->io_base == 0)
         success = FALSE;
   }

   /* Lock card */

   if(success)
   {
      if(!Prm_SetBoardAttrsTags(card, PRM_BoardOwner, (UPINT)base, TAG_END))
         success = FALSE;
   }

   if(!success)
   {
      FreeCard(context, base);
      context = NULL;
   }

   return context;
}



/****i* rhine.device/FreeCard **********************************************
*
*   NAME
*	FreeCard
*
*   SYNOPSIS
*	FreeCard(context)
*
*	VOID FreeCard(struct BusContext *);
*
****************************************************************************
*
*/

static VOID FreeCard(struct BusContext *context, struct DevBase *base)
{
   PCIBoard *card;
   APTR owner;

   if(context != NULL)
   {
      card = context->card;
      if(card != NULL)
      {
         /* Unlock board */

         Prm_GetBoardAttrsTags(card, PRM_BoardOwner, (UPINT)&owner,
            TAG_END);
         if(owner == base)
            Prm_SetBoardAttrsTags(card, PRM_BoardOwner, NULL, TAG_END);
      }

      FreeMem(context, sizeof(struct BusContext));
   }

   return;
}



/****i* rhine.device/AddPCIIntServer ***************************************
*
*   NAME
*	AddPCIIntServer
*
*   SYNOPSIS
*	success = AddPCIIntServer(card, interrupt)
*
*	BOOL AddPCIIntServer(APTR, struct Interrupt *);
*
****************************************************************************
*
*/

static BOOL AddPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   return Prm_AddIntServer(card, interrupt);
}



/****i* rhine.device/RemPCIIntServer ***************************************
*
*   NAME
*	RemPCIIntServer
*
*   SYNOPSIS
*	RemPCIIntServer(card, interrupt)
*
*	VOID RemPCIIntServer(APTR, struct Interrupt *);
*
****************************************************************************
*
*/

static VOID RemPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   Prm_RemIntServer(card, interrupt);

   return;
}



/****i* rhine.device/IsCardCompatible **************************************
*
*   NAME
*	IsCardCompatible
*
*   SYNOPSIS
*	compatible = IsCardCompatible(vendor_id, product_id)
*
*	BOOL IsCardCompatible(UWORD, UWORD);
*
****************************************************************************
*
*/

BOOL IsCardCompatible(UWORD vendor_id, UWORD product_id,
   struct DevBase *base)
{
   BOOL compatible = FALSE;
   const UWORD *p;

   for(p = product_codes; p[0] != 0xffff; p += 2)
   {
      if(p[0] == vendor_id && p[1] == product_id)
         compatible = TRUE;
   }

   return compatible;
}



/****i* rhine.device/ByteInHook ********************************************
*
*   NAME
*	ByteInHook
*
*   SYNOPSIS
*	value = ByteInHook(context, offset)
*
*	UBYTE ByteInHook(struct BusContext *, ULONG);
*
****************************************************************************
*
*/

static UBYTE ByteInHook(struct BusContext *context, ULONG offset)
{
   return BYTEIN(context->io_base + offset);
}



/****i* rhine.device/ByteOutHook *******************************************
*
*   NAME
*	ByteOutHook
*
*   SYNOPSIS
*	ByteOutHook(context, offset, value)
*
*	VOID ByteOutHook(struct BusContext *, ULONG, UBYTE);
*
****************************************************************************
*
*/

static VOID ByteOutHook(struct BusContext *context, ULONG offset,
   UBYTE value)
{
   BYTEOUT(context->io_base + offset, value);

   return;
}



/****i* rhine.device/LEWordInHook ******************************************
*
*   NAME
*	LEWordInHook
*
*   SYNOPSIS
*	value = LEWordInHook(context, offset)
*
*	UWORD LEWordInHook(struct BusContext *, ULONG);
*
****************************************************************************
*
*/

static UWORD LEWordInHook(struct BusContext *context, ULONG offset)
{
   return LEWORDIN(context->io_base + offset);
}



/****i* rhine.device/LELongInHook ******************************************
*
*   NAME
*	LELongInHook
*
*   SYNOPSIS
*	value = LELongInHook(context, offset)
*
*	ULONG LELongInHook(struct BusContext *, ULONG);
*
****************************************************************************
*
*/

static ULONG LELongInHook(struct BusContext *context, ULONG offset)
{
   return LELONGIN(context->io_base + offset);
}



/****i* rhine.device/LEWordOutHook *****************************************
*
*   NAME
*	LEWordOutHook
*
*   SYNOPSIS
*	LEWordOutHook(context, offset, value)
*
*	VOID LEWordOutHook(struct BusContext *, ULONG, UWORD);
*
****************************************************************************
*
*/

static VOID LEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value)
{
   LEWORDOUT(context->io_base + offset, value);

   return;
}



/****i* rhine.device/LELongOutHook *****************************************
*
*   NAME
*	LELongOutHook
*
*   SYNOPSIS
*	LELongOutHook(context, offset, value)
*
*	VOID LELongOutHook(struct BusContext *, ULONG, ULONG);
*
****************************************************************************
*
*/

static VOID LELongOutHook(struct BusContext *context, ULONG offset,
   ULONG value)
{
   LELONGOUT(context->io_base + offset, value);

   return;
}



/****i* rhine.device/AllocDMAMemHook ***************************************
*
*   NAME
*	AllocDMAMemHook
*
*   SYNOPSIS
*	mem = AllocDMAMemHook(context, size, alignment)
*
*	APTR AllocDMAMemHook(struct BusContext *, UPINT, UWORD);
*
****************************************************************************
*
*/

static APTR AllocDMAMemHook(struct BusContext *context, UPINT size,
   UWORD alignment)
{
   struct DevBase *base;
   APTR mem = NULL, original_mem;

   base = context->device;
   if(alignment < 8)
      alignment = 8;
   size += 2 * sizeof(APTR) + alignment;
   original_mem = Prm_AllocDMABuffer(size);
   if(original_mem != NULL)
   {
      mem = (APTR)((UPINT)(original_mem + 2 * sizeof(APTR) + alignment - 1)
         & ~(alignment - 1));
      *((APTR *)mem - 1) = original_mem;
      *((UPINT *)mem - 2) = size;
   }

   return mem;
}



/****i* rhine.device/FreeDMAMemHook ****************************************
*
*   NAME
*	FreeDMAMemHook
*
*   SYNOPSIS
*	FreeDMAMemHook(context, mem)
*
*	VOID FreeDMAMemHook(struct BusContext *, APTR);
*
****************************************************************************
*
*/

static VOID FreeDMAMemHook(struct BusContext *context, APTR mem)
{
   struct DevBase *base;

   base = context->device;
   if(mem != NULL)
      Prm_FreeDMABuffer(*((APTR *)mem - 1), *((UPINT *)mem - 2));

   return;
}



