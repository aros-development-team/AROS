/*

Copyright (C) 2004-2011 Neil Cafferkey

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

#include <proto/exec.h>
#include <proto/expansion.h>

#include "device.h"
#include "pci.h"

#include "pci_protos.h"
#include "device_protos.h"
#include "prometheus_protos.h"
#include "expansion_protos.h"
#include "openpci_protos.h"
#include "unit_protos.h"


/* Private prototypes */

static struct DevUnit *FindPCIUnit(ULONG index, struct DevBase *base);
static struct DevUnit *CreatePCIUnit(ULONG index, struct DevBase *base);
static struct BusContext *AllocCard(ULONG index, struct DevBase *base);
static VOID FreeCard(struct BusContext *context, struct DevBase *base);
static BOOL AddPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base);
static VOID RemPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base);
BOOL IsCardCompatible(UWORD vendor_id, UWORD product_id,
   struct DevBase *base);
static APTR AllocDMAMemHook(struct BusContext *context, UPINT size,
   UWORD alignment);
static VOID FreeDMAMemHook(struct BusContext *context, APTR mem);


const UWORD product_codes[] =
{
   0x10b7, 0x0013,
   0x168c, 0x0007,
   0x168c, 0x0012,
   0x168c, 0x0013,
   0x168c, 0x0015,
   0x168c, 0x0016,
   0x168c, 0x0017,
   0x168c, 0x0018,
   0x168c, 0x0019,
   0x168c, 0x001a,
   0x168c, 0x001b,
   0x168c, 0x001c,
   0x168c, 0x001d,
   0x168c, 0x1014,
   0x168c, 0x101a,
   0x168c, 0x9013,
   0xa727, 0x0013,
   0xffff, 0xffff
};


static const struct TagItem unit_tags[] =
{
   {IOTAG_AllocDMAMem, (UPINT)AllocDMAMemHook},
   {IOTAG_FreeDMAMem, (UPINT)FreeDMAMemHook},
   {TAG_END, 0}
};


/****i* atheros5000.device/GetPCICount *************************************
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

#if !(defined(__MORPHOS__) || defined(__amigaos4__))
   if(base->prometheus_base != NULL)
      count = GetPrometheusCount(base);
#endif
#ifdef __amigaos4__
   if(base->expansion_base != NULL)
      count = GetExpansionCount(base);
#endif
#ifdef __MORPHOS__
   if(base->openpci_base != NULL)
      count = GetOpenPCICount(base);
#endif

   return count;
}



/****i* atheros5000.device/GetPCIUnit **************************************
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



/****i* atheros5000.device/FindPCIUnit *************************************
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



/****i* atheros5000.device/CreatePCIUnit ***********************************
*
*   NAME
*	CreatePCIUnit -- Create a unit.
*
*   SYNOPSIS
*	unit = CreatePCIUnit(index)
*
*	struct DevUnit *CreatePCIUnit(ULONG);
*
*   FUNCTION
*	Creates a PCI new unit.
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
      context->unit = unit =
         CreateUnit(index, (APTR)context->io_base, context->id, context,
            context->unit_tags, 0, base);
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
         && WrapInt(&unit->mgmt_int, base)
         && WrapInt(&unit->mgmt_end_int, base)
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

#if defined(__amigaos4__) || defined(__AROS__)
      if(AddResetCallback(&unit->reset_handler))
         unit->flags |= UNITF_RESETADDED;
      else
         success = FALSE;
#endif
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



/****i* atheros5000.device/DeletePCIUnit ***********************************
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
#if defined(__amigaos4__) || defined(__AROS__)
      if((unit->flags & UNITF_RESETADDED) != 0)
         RemResetCallback(&unit->reset_handler);
#endif
      if((unit->flags & UNITF_INTADDED) != 0)
         RemPCIIntServer(context->card, &unit->status_int, base);
      UnwrapInt(&unit->reset_handler, base);
      UnwrapInt(&unit->mgmt_end_int, base);
      UnwrapInt(&unit->mgmt_int, base);
      UnwrapInt(&unit->tx_end_int, base);
      UnwrapInt(&unit->tx_int, base);
      UnwrapInt(&unit->rx_int, base);
      UnwrapInt(&unit->status_int, base);
      DeleteUnit(unit, base);
      FreeCard(context, base);
   }

   return;
}



/****i* atheros5000.device/AllocCard ***************************************
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
   struct BusContext *context;

#if !(defined(__MORPHOS__) || defined(__amigaos4__))
   if(base->prometheus_base != NULL)
      context = AllocPrometheusCard(index, base);
#endif
#ifdef __amigaos4__
   if(base->expansion_base != NULL)
      context = AllocExpansionCard(index, base);
#endif
#ifdef __MORPHOS__
   if(base->openpci_base != NULL)
      context = AllocOpenPCICard(index, base);
#endif

   return context;
}



/****i* atheros5000.device/FreeCard ****************************************
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

   if(context != NULL)
   {
#if !(defined(__MORPHOS__) || defined(__amigaos4__))
      if(base->prometheus_base != NULL)
         FreePrometheusCard(context, base);
#endif
#ifdef __amigaos4__
      if(base->expansion_base != NULL)
         FreeExpansionCard(context, base);
#endif
#ifdef __MORPHOS__
      if(base->openpci_base != NULL)
         FreeOpenPCICard(context, base);
#endif
   }

   return;
}



/****i* atheros5000.device/AddPCIIntServer *********************************
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
   BOOL success;

#if !(defined(__MORPHOS__) || defined(__amigaos4__))
   if(base->prometheus_base != NULL)
      success = AddPrometheusIntServer(card, interrupt, base);
#endif
#ifdef __amigaos4__
   if(base->expansion_base != NULL)
      success = AddExpansionIntServer(card, interrupt, base);
#endif
#ifdef __MORPHOS__
   if(base->openpci_base != NULL)
      success = AddOpenPCIIntServer(card, interrupt, base);
#endif

   return success;
}



/****i* atheros5000.device/RemPCIIntServer *********************************
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
#if !(defined(__MORPHOS__) || defined(__amigaos4__))
   if(base->prometheus_base != NULL)
      RemPrometheusIntServer(card, interrupt, base);
#endif
#ifdef __amigaos4__
   if(base->expansion_base != NULL)
      RemExpansionIntServer(card, interrupt, base);
#endif
#ifdef __MORPHOS__
   if(base->openpci_base != NULL)
      RemOpenPCIIntServer(card, interrupt, base);
#endif

   return;
}



/****i* atheros5000.device/IsCardCompatible ********************************
*
*   NAME
*	IsCardCompatible
*
*   SYNOPSIS
*	compatible = IsCardCompatible(context)
*
*	BOOL IsCardCompatible(struct BusContext *);
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



/****i* atheros5000.device/AllocDMAMemHook *********************************
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
* Alignment currently must be minimum of 8 bytes.
*
*/

static APTR AllocDMAMemHook(struct BusContext *context, UPINT size,
   UWORD alignment)
{
   struct DevBase *base;
   APTR mem = NULL, original_mem;

   base = context->device;
#ifndef __amigaos4__
alignment = 4096;
   size += 2 * sizeof(APTR) + alignment;
#if !(defined(__MORPHOS__) || defined(__amigaos4__))
   if(base->prometheus_base != NULL)
      original_mem = AllocPrometheusDMAMem(size, base);
   else
#endif
      original_mem = AllocMem(size, MEMF_PUBLIC);
   if(original_mem != NULL)
   {
      mem = (APTR)((UPINT)(original_mem + 2 * sizeof(APTR) + alignment - 1)
         & ~(alignment - 1));
      *((APTR *)mem - 1) = original_mem;
      *((UPINT *)mem - 2) = size;
   }
#else
if(size < 4000) size = 4000;
   mem = AllocVec(size, MEMF_PUBLIC);
#endif

   return mem;
}



/****i* atheros5000.device/FreeDMAMemHook **********************************
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
#ifndef __amigaos4__
   if(mem != NULL)
   {
#if !(defined(__MORPHOS__) || defined(__amigaos4__))
      if(base->prometheus_base != NULL)
         FreePrometheusDMAMem(*((APTR *)mem - 1), *((UPINT *)mem - 2),
            base);
      else
#endif
         FreeMem(*((APTR *)mem - 1), *((UPINT *)mem - 2));
   }
#else
   FreeVec(mem);
#endif

   return;
}



