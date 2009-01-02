/*

Copyright (C) 2004,2005 Neil Cafferkey

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

#ifdef __amigaos4__
#include <expansion/pci.h>
#endif

#include <proto/exec.h>
#include <proto/expansion.h>

#include "device.h"
#include "pci.h"

#include "pci_protos.h"
#include "prometheus_protos.h"
#include "powerpci_protos.h"
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
static BOOL WrapInt(struct Interrupt *interrupt, struct DevBase *base);
static VOID UnwrapInt(struct Interrupt *interrupt, struct DevBase *base);


const UWORD product_codes[] =
{
   0x8086, 0x1029,
   0x8086, 0x1030,
   0x8086, 0x1031,
   0x8086, 0x1032,
   0x8086, 0x1033,
   0x8086, 0x1034,
   0x8086, 0x1035,
   0x8086, 0x1036,
   0x8086, 0x1037,
   0x8086, 0x1038,
   0x8086, 0x1039,
   0x8086, 0x103A,
   0x8086, 0x103B,
   0x8086, 0x103C,
   0x8086, 0x103D,
   0x8086, 0x103E,
   0x8086, 0x1050,
   0x8086, 0x1059,
   0x8086, 0x1209,
   0x8086, 0x1227,
   0x8086, 0x1228,
   0x8086, 0x1229,
   0x8086, 0x2449,
   0x8086, 0x2459,
   0x8086, 0x245D,
   0x8086, 0x5200,
   0x8086, 0x5201,
   0xffff, 0xffff
};


static const struct TagItem unit_tags[] =
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


/****i* intelpro100.device/GetPCICount *************************************
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
#ifdef __mc68000__
   if(base->powerpci_base != NULL)
      count = GetPowerPCICount(base);
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



/****i* intelpro100.device/GetPCIUnit **************************************
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



/****i* intelpro100.device/FindPCIUnit *************************************
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



/****i* intelpro100.device/CreatePCIUnit ***********************************
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
      {
         context->unit_tags = unit_tags;
      }
   }

   if(success)
   {
      context->device = base;
      context->unit = unit =
         CreateUnit(index, context, context->unit_tags, PCI_BUS, base);
      if(unit == NULL)
         success = FALSE;
   }

   /* Add interrupt */

   if(success)
   {
      if(!(WrapInt(&unit->status_int, base)
         && WrapInt(&unit->rx_int, base)
         && WrapInt(&unit->tx_int, base)
         && WrapInt(&unit->tx_end_int, base)))
         success = FALSE;
      success = AddPCIIntServer(context->card, &unit->status_int, base);
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



/****i* intelpro100.device/DeletePCIUnit ***********************************
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
      RemPCIIntServer(context->card, &unit->status_int, base);
      UnwrapInt(&unit->tx_end_int, base);
      UnwrapInt(&unit->tx_int, base);
      UnwrapInt(&unit->rx_int, base);
      UnwrapInt(&unit->status_int, base);
      DeleteUnit(unit, base);
      FreeCard(context, base);
   }

   return;
}



/****i* intelpro100.device/AllocCard ***************************************
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
#ifdef __mc68000__
   if(base->powerpci_base != NULL)
      context = AllocPowerPCICard(index, base);
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



/****i* intelpro100.device/FreeCard ****************************************
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
#ifdef __mc68000
      if(base->powerpci_base != NULL)
         FreePowerPCICard(context, base);
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



/****i* intelpro100.device/AddPCIIntServer *********************************
*
*   NAME
*	AddPCIIntServer
*
*   SYNOPSIS
*	context = AddPCIIntServer(index)
*
*	struct BusContext *AddPCIIntServer(ULONG);
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
#ifdef __mc68000
   if(base->powerpci_base != NULL)
      success = AddPowerPCIIntServer(card, interrupt, base);
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



/****i* intelpro100.device/RemPCIIntServer *********************************
*
*   NAME
*	RemPCIIntServer
*
*   SYNOPSIS
*	RemPCIIntServer()
*
*	VOID RemPCIIntServer(ULONG);
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
#ifdef __mc68000
   if(base->powerpci_base != NULL)
      RemPowerPCIIntServer(card, interrupt, base);
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



/****i* intelpro100.device/IsCardCompatible ********************************
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



/****i* intelpro100.device/ByteInHook **************************************
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



/****i* intelpro100.device/ByteOutHook *************************************
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



/****i* intelpro100.device/LEWordInHook ************************************
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



/****i* intelpro100.device/LELongInHook ************************************
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



/****i* intelpro100.device/LEWordOutHook ***********************************
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



/****i* intelpro100.device/LELongOutHook ***********************************
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



/****i* intelpro100.device/AllocDMAMemHook *********************************
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

   return mem;
}



/****i* intelpro100.device/FreeDMAMemHook **********************************
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
   {
#if !(defined(__MORPHOS__) || defined(__amigaos4__))
      if(base->prometheus_base != NULL)
         FreePrometheusDMAMem(*((APTR *)mem - 1), *((UPINT *)mem - 2),
            base);
      else
#endif
         FreeMem(*((APTR *)mem - 1), *((UPINT *)mem - 2));
   }

   return;
}



/****i* intelpro100.device/WrapInt *****************************************
*
*   NAME
*	WrapInt
*
****************************************************************************
*
*/

BOOL WrapInt(struct Interrupt *interrupt, struct DevBase *base)
{
   BOOL success = TRUE;
#if defined(__amigaos4__) || defined(__MORPHOS__)
   APTR *int_data;

   int_data = AllocMem(2 * sizeof(APTR), MEMF_PUBLIC | MEMF_CLEAR);
   if(int_data != NULL)
   {
      int_data[0] = interrupt->is_Code;
      int_data[1] = interrupt->is_Data;
      interrupt->is_Code = base->wrapper_int_code;
      interrupt->is_Data = int_data;
   }
   else
      success = FALSE;
#endif

   return success;
}



/****i* intelpro100.device/UnwrapInt ***************************************
*
*   NAME
*	UnwrapInt
*
****************************************************************************
*
*/

static VOID UnwrapInt(struct Interrupt *interrupt, struct DevBase *base)
{
   if(interrupt->is_Code == base->wrapper_int_code)
      FreeMem(interrupt->is_Data, 2 * sizeof(APTR));

   return;
}



