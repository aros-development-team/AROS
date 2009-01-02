/*

File: pci.c
Author: Neil Cafferkey
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

#include <proto/exec.h>
#include <proto/expansion.h>

#include "device.h"
#include "pci.h"

#include "pci_protos.h"
#if !(defined(__MORPHOS__) || defined(__amigaos4__))
#include "prometheus_protos.h"
#endif
#ifdef __mc68000__
#include "powerpci_protos.h"
#endif
#ifdef __amigaos4__
#include "expansion_protos.h"
#endif
#ifdef __MORPHOS__
#include "openpci_protos.h"
#endif
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
static VOID WordsInHook(struct BusContext *context, ULONG offset,
   UWORD *buffer, ULONG count);
static VOID WordsOutHook(struct BusContext *context, ULONG offset,
   const UWORD *buffer, ULONG count);
static VOID BEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value);
static UWORD LEWordInHook(struct BusContext *context, ULONG offset);
static VOID LEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value);
static VOID WordsInIOHook(struct BusContext *context, ULONG offset,
   UWORD *buffer, ULONG count);
static VOID WordsOutIOHook(struct BusContext *context, ULONG offset,
   const UWORD *buffer, ULONG count);
static VOID BEWordOutIOHook(struct BusContext *context, ULONG offset,
   UWORD value);
static UWORD LEWordInIOHook(struct BusContext *context, ULONG offset);
static VOID LEWordOutIOHook(struct BusContext *context, ULONG offset,
   UWORD value);
BOOL WrapInt(struct Interrupt *interrupt, struct DevBase *base);
VOID UnwrapInt(struct Interrupt *interrupt, struct DevBase *base);


const UWORD product_codes[] =
{
   0x10b7, 0x7770,
   0x111a, 0x1023,
   0x1260, 0x3872,
   0x1260, 0x3873,
   0x1385, 0x4100,
   0x15e8, 0x0130,
   0x15e8, 0x0131,
   0x1638, 0x1100,
   0x16ab, 0x1100,
   0x16ab, 0x1101,
   0x16ab, 0x1102,
   0x16ec, 0x3685,
   0xec80, 0xec00,
   0xffff, 0xffff
};


static const struct TagItem unit_tags[] =
{
   {IOTAG_WordsIn, (UPINT)WordsInHook},
   {IOTAG_WordsOut, (UPINT)WordsOutHook},
   {IOTAG_BEWordOut, (UPINT)BEWordOutHook},
   {IOTAG_LEWordIn, (UPINT)LEWordInHook},
   {IOTAG_LEWordOut, (UPINT)LEWordOutHook},
   {TAG_END, 0}
};


static const struct TagItem bridge_unit_tags[] =
{
   {IOTAG_WordsIn, (UPINT)WordsInIOHook},
   {IOTAG_WordsOut, (UPINT)WordsOutIOHook},
   {IOTAG_BEWordOut, (UPINT)BEWordOutIOHook},
   {IOTAG_LEWordIn, (UPINT)LEWordInIOHook},
   {IOTAG_LEWordOut, (UPINT)LEWordOutIOHook},
   {TAG_END, 0}
};


/****i* prism2.device/GetPCICount ******************************************
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



/****i* prism2.device/GetPCIUnit *******************************************
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



/****i* prism2.device/FindPCIUnit ******************************************
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



/****i* prism2.device/CreatePCIUnit ****************************************
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
         if(context->bus_type == PCI_BUS)
            context->unit_tags = unit_tags;
         else
            context->unit_tags = bridge_unit_tags;
      }

      context->unit = unit =
         CreateUnit(index, context, context->unit_tags, context->bus_type,
            base);
      if(unit == NULL)
         success = FALSE;
   }

   /* Add interrupt */

   if(success)
   {
      if(!(WrapInt(&unit->status_int, base)
         && WrapInt(&unit->rx_int, base)
         && WrapInt(&unit->tx_int, base)
         && WrapInt(&unit->info_int, base)))
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



/****i* prism2.device/DeletePCIUnit ****************************************
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
      UnwrapInt(&unit->info_int, base);
      UnwrapInt(&unit->tx_int, base);
      UnwrapInt(&unit->rx_int, base);
      UnwrapInt(&unit->status_int, base);
      DeleteUnit(unit, base);
      FreeCard(context, base);
   }

   return;
}



/****i* prism2.device/AllocCard ********************************************
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



/****i* prism2.device/FreeCard *********************************************
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



/****i* prism2.device/AddPCIIntServer **************************************
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



/****i* prism2.device/RemPCIIntServer **************************************
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



/****i* prism2.device/IsCardCompatible *************************************
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



/****i* prism2.device/GetBusType *******************************************
*
*   NAME
*	GetBusType
*
*   SYNOPSIS
*	bus_type = GetBusType(product_id)
*
*	UWORD GetBusType(UWORD);
*
****************************************************************************
*
*/

UWORD GetBusType(UWORD product_id, struct DevBase *base)
{
   UWORD bus_type;

   switch(product_id)
   {
   case 0x3872:
   case 0x3873:
      bus_type = PCI_BUS;
      break;
   case 0x0131:
      bus_type = TMD_BUS;
      break;
   default:
      bus_type = PLX_BUS;
   }

   return bus_type;
}



/****i* prism2.device/WordsInHook ******************************************
*
*   NAME
*	WordsInHook
*
*   SYNOPSIS
*	WordsInHook(context, offset, buffer, count)
*
*	VOID WordsInHook(struct BusContext *, ULONG, UWORD *, ULONG);
*
****************************************************************************
*
*/

static VOID WordsInHook(struct BusContext *context, ULONG offset,
   UWORD *buffer, ULONG count)
{
   volatile UWORD *reg;

   reg = (volatile UWORD *)(context->io_base + (offset << 1));
   while(count-- > 0)
      *buffer++ = *reg;

   return;
}



/****i* prism2.device/WordsOutHook *****************************************
*
*   NAME
*	WordsOutHook
*
*   SYNOPSIS
*	WordsOutHook(context, offset, buffer, count)
*
*	VOID WordsOutHook(struct BusContext *, ULONG, const UWORD *, ULONG);
*
****************************************************************************
*
*/

static VOID WordsOutHook(struct BusContext *context, ULONG offset,
   const UWORD *buffer, ULONG count)
{
   volatile UWORD *reg;

   reg = (volatile UWORD *)(context->io_base + (offset << 1));
   while(count-- > 0)
      *reg = *buffer++;

   return;
}



/****i* prism2.device/BEWordOutHook ****************************************
*
*   NAME
*	BEWordOutHook
*
*   SYNOPSIS
*	BEWordOutHook(context, offset, value)
*
*	VOID BEWordOutHook(struct BusContext *, ULONG, UWORD);
*
****************************************************************************
*
*/

static VOID BEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value)
{
   *(volatile UWORD *)(context->io_base + (offset << 1)) =
      MakeBEWord(value);

   return;
}



/****i* prism2.device/LEWordInHook *****************************************
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
   return LEWord(*(volatile UWORD *)(context->io_base + (offset << 1)));
}



/****i* prism2.device/LEWordOutHook ****************************************
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
   *(volatile UWORD *)(context->io_base + (offset << 1)) =
      MakeLEWord(value);

   return;
}



/****i* prism2.device/WordsInIOHook ****************************************
*
*   NAME
*	WordsInIOHook
*
*   SYNOPSIS
*	WordsInIOHook(context, offset, buffer, count)
*
*	VOID WordsInIOHook(struct BusContext *, ULONG, UWORD *, ULONG);
*
****************************************************************************
*
*/

static VOID WordsInIOHook(struct BusContext *context, ULONG offset,
   UWORD *buffer, ULONG count)
{
   WORDSIN(context->io_base + offset, buffer, count);

   return;
}



/****i* prism2.device/WordsOutIOHook ***************************************
*
*   NAME
*	WordsOutIOHook
*
*   SYNOPSIS
*	WordsOutIOHook(context, offset, buffer,
*	    count)
*
*	VOID WordsOutIOHook(struct BusContext *, ULONG, const UWORD *,
*	    ULONG);
*
****************************************************************************
*
*/

static VOID WordsOutIOHook(struct BusContext *context, ULONG offset,
   const UWORD *buffer, ULONG count)
{
   WORDSOUT(context->io_base + offset, buffer, count);

   return;
}



/****i* prism2.device/BEWordOutIOHook **************************************
*
*   NAME
*	BEWordOutIOHook
*
*   SYNOPSIS
*	BEWordOutIOHook(context, offset, value)
*
*	VOID BEWordOutIOHook(struct BusContext *, ULONG, UWORD);
*
****************************************************************************
*
*/

static VOID BEWordOutIOHook(struct BusContext *context, ULONG offset,
   UWORD value)
{
   BEWORDOUT(context->io_base + offset, value);

   return;
}



/****i* prism2.device/LEWordInIOHook ***************************************
*
*   NAME
*	LEWordInIOHook
*
*   SYNOPSIS
*	value = LEWordInIOHook(context, offset)
*
*	UWORD LEWordInIOHook(struct BusContext *, ULONG);
*
****************************************************************************
*
*/

static UWORD LEWordInIOHook(struct BusContext *context, ULONG offset)
{
   return LEWORDIN(context->io_base + offset);
}



/****i* prism2.device/LEWordOutIOHook **************************************
*
*   NAME
*	LEWordOutIOHook
*
*   SYNOPSIS
*	LEWordOutIOHook(context, offset, value)
*
*	VOID LEWordOutIOHook(struct BusContext *, ULONG, UWORD);
*
****************************************************************************
*
*/

static VOID LEWordOutIOHook(struct BusContext *context, ULONG offset,
   UWORD value)
{
   LEWORDOUT(context->io_base + offset, value);

   return;
}



/****i* prism2.device/WrapInt **********************************************
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



/****i* prism2.device/UnwrapInt ********************************************
*
*   NAME
*	UnwrapInt
*
****************************************************************************
*
*/

VOID UnwrapInt(struct Interrupt *interrupt, struct DevBase *base)
{
   if(interrupt->is_Code == base->wrapper_int_code)
      FreeMem(interrupt->is_Data, 2 * sizeof(APTR));

   return;
}



