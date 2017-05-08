/*

Copyright (C) 2000-2010 Neil Cafferkey

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
#include <libraries/pccard.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/cardres.h>
#include <proto/pccard.h>

#include "device.h"

#include "pccard_protos.h"
#include "device_protos.h"
#include "unit_protos.h"

#define MAX_TUPLE_SIZE 0xff
#define TUPLE_BUFFER_SIZE (MAX_TUPLE_SIZE + 8)
#define HANDLE_PRIORITY 10


struct BusContext
{
   struct DevUnit *unit;
   struct CardHandle *card_handle;
   UBYTE *tuple_buffer;
   UPINT config_base;
   UPINT io_base;
   UWORD resource_version;
   BOOL have_card;
};


/* Private prototypes */

static struct DevUnit *FindPCCardUnit(ULONG index, struct DevBase *base);
static struct DevUnit *CreatePCCardUnit(ULONG index,
   struct DevBase *base);
static struct BusContext *AllocCard(struct DevBase *base);
static VOID FreeCard(struct BusContext *context, struct DevBase *base);
static BOOL IsCardCompatible(struct BusContext *context,
   struct DevBase *base);
static BOOL InitialiseCard(struct BusContext *context,
   struct DevBase *base);
static VOID CardRemovedHook(struct BusContext *context,
   struct DevBase *base);
static BOOL CardInsertedHook(struct BusContext *context,
   struct DevBase *base);
static VOID CardRemovedInt(REG(a1, struct BusContext *context),
   REG(a6, APTR int_code));
static VOID CardInsertedInt(REG(a1, struct BusContext *context),
   REG(a6, APTR int_code));
static UBYTE CardStatusInt(REG(a1, struct BusContext *context),
   REG(a6, APTR int_code), REG(d0, UBYTE mask));
static VOID WordsInHook(struct BusContext *context,
   ULONG offset, UWORD *buffer, ULONG count);
static VOID WordsOutHook(struct BusContext *context, ULONG offset,
   const UWORD *buffer, ULONG count);
static VOID BEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value);
static UWORD LEWordInHook(struct BusContext *context, ULONG offset);
static VOID LEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value);


static const ULONG product_codes[] =
{
   0x000b7300,
   0x00890001,
   0x01010001,
   0x01050000,
   0x01380002,
   0x014d0801,
   0x01560002,
   0x01560003,
   0x016b0001,
   0x016c0001,
   0x01eb080a,
   0x02500002,
   0x02610002,
   0x026f0305,
   0x026f030b,
   0x02741612,
   0x02741613,
   0x02743301,
   0x028a0002,
   0x02ac0002,
   0x02d20001,
   0x16680101,
   0xc0010008,
   0xc00f0000,
   0xc2500002,
   0xd6010002,
   0xd6010004,
   0xd6010005,
   0
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


/****i* prism2.device/GetPCCardCount ***************************************
*
*   NAME
*	GetPCCardCount -- Get the number of compatible PC Cards.
*
*   SYNOPSIS
*	count = GetPCCardCount()
*
*	ULONG GetPCCardCount();
*
****************************************************************************
*
*/

ULONG GetPCCardCount(struct DevBase *base)
{
   ULONG count = 0;
   struct BusContext *context;

   if(CardResource != NULL)
   {
      if(FindPCCardUnit(0, base) != NULL)
         count = 1;
      else
      {
         context = AllocCard(base);
         if(context != NULL)
         {
            count = 1;
            FreeCard(context, base);
         }
      }
   }

   return count;
}



/****i* prism2.device/GetPCCardUnit ****************************************
*
*   NAME
*	GetPCCardUnit -- Get a unit by number.
*
*   SYNOPSIS
*	unit = GetPCCardUnit(index)
*
*	struct DevUnit *GetPCCardUnit(ULONG);
*
****************************************************************************
*
*/

struct DevUnit *GetPCCardUnit(ULONG index, struct DevBase *base)
{
   struct DevUnit *unit;

   unit = FindPCCardUnit(index, base);

   if(unit == NULL)
   {
      unit = CreatePCCardUnit(index, base);
      if(unit != NULL)
         AddTail((APTR)&base->pccard_units, (APTR)unit);
   }

   return unit;
}



/****i* prism2.device/FindPCCardUnit ***************************************
*
*   NAME
*	FindPCCardUnit -- Find a unit by number.
*
*   SYNOPSIS
*	unit = FindPCCardUnit(index)
*
*	struct DevUnit *FindPCCardUnit(ULONG);
*
****************************************************************************
*
*/

static struct DevUnit *FindPCCardUnit(ULONG index, struct DevBase *base)
{
   struct DevUnit *unit, *tail;

   unit = (APTR)base->pccard_units.mlh_Head;
   tail = (APTR)&base->pccard_units.mlh_Tail;
   if(unit == tail)
      unit = NULL;

   return unit;
}



/****i* prism2.device/CreatePCCardUnit *************************************
*
*   NAME
*	CreatePCCardUnit -- Create a unit.
*
*   SYNOPSIS
*	unit = CreatePCCardUnit(index)
*
*	struct DevUnit *CreatePCCardUnit(ULONG);
*
*   FUNCTION
*	Creates a new unit.
*
****************************************************************************
*
*/

static struct DevUnit *CreatePCCardUnit(ULONG index,
   struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   struct DevUnit *unit;

   /* Get card from system */

   context = AllocCard(base);
   if(context == NULL)
      success = FALSE;

   /* Prepare card for use */

   if(success)
   {
      if(!InitialiseCard(context, base))
         success = FALSE;
   }

   /* Create device driver unit */

   if(success)
   {
      context->unit = unit =
         CreateUnit(index, context, unit_tags, PCCARD_BUS, base);
      if(unit == NULL)
         success = FALSE;
   }

   if(success)
   {
     if(!(WrapInt(&unit->status_int, base)
         && WrapInt(&unit->rx_int, base)
         && WrapInt(&unit->tx_int, base)
         && WrapInt(&unit->info_int, base)))
         success = FALSE;

      unit->insertion_function = (APTR)CardInsertedHook;
      unit->removal_function = (APTR)CardRemovedHook;
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



/****i* prism2.device/DeletePCCardUnit *************************************
*
*   NAME
*	DeletePCCardUnit -- Delete a unit.
*
*   SYNOPSIS
*	DeletePCCardUnit(unit)
*
*	VOID DeletePCCardUnit(struct DevUnit *);
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

VOID DeletePCCardUnit(struct DevUnit *unit, struct DevBase *base)
{
   struct BusContext *context;

   if(unit != NULL)
   {
      UnwrapInt(&unit->info_int, base);
      UnwrapInt(&unit->tx_int, base);
      UnwrapInt(&unit->rx_int, base);
      UnwrapInt(&unit->status_int, base);
      context = unit->card;
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
*	unit = AllocCard()
*
*	struct BusContext *AllocCard();
*
****************************************************************************
*
*/

static struct BusContext *AllocCard(struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   struct CardHandle *card_handle;
   struct Interrupt *card_removed_int, *card_inserted_int, *card_status_int;

   context = AllocMem(sizeof(struct BusContext), MEMF_PUBLIC | MEMF_CLEAR);
   if(context == NULL)
      success = FALSE;

   if(success)
   {
      context->resource_version =
         ((struct Library *)base->card_base)->lib_Version;
      context->card_handle = card_handle =
         AllocMem(sizeof(struct CardHandle), MEMF_PUBLIC | MEMF_CLEAR);
      context->tuple_buffer =
         AllocVec(TUPLE_BUFFER_SIZE, MEMF_PUBLIC);

      if(card_handle == NULL || context->tuple_buffer == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Set up card handle */

      card_handle->cah_CardNode.ln_Pri = HANDLE_PRIORITY;
      card_handle->cah_CardNode.ln_Name =
         base->device.dd_Library.lib_Node.ln_Name;
      card_handle->cah_CardFlags = CARDF_POSTSTATUS;

      card_handle->cah_CardRemoved = card_removed_int =
         AllocVec(sizeof(struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);

      card_handle->cah_CardInserted = card_inserted_int =
         AllocVec(sizeof(struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);

      card_handle->cah_CardStatus = card_status_int =
         AllocVec(sizeof(struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);

      if(card_removed_int == NULL || card_inserted_int == NULL
         || card_status_int == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Try to gain access to card */

      card_removed_int->is_Code = CardRemovedInt;
      card_removed_int->is_Data = context;
      card_inserted_int->is_Code = CardInsertedInt;
      card_inserted_int->is_Data = context;
      card_status_int->is_Code = (APTR)CardStatusInt;
      card_status_int->is_Data = context;

      if(!(WrapCardInt(card_removed_int, base)
         && WrapCardInt(card_inserted_int, base)
         && WrapCardInt(card_status_int, base)))
         success = FALSE;
   }

   if(success)
   {
      if(OwnCard(card_handle) != 0)
         success = FALSE;
   }

   if(success)
   {
      if(!IsCardCompatible(context, base))
         success = FALSE;
   }

   if(!success)
   {
      FreeCard(context, base);
      context = NULL;
   }
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
   struct CardHandle *card_handle;

   if(context != NULL)
   {
      card_handle = context->card_handle;

      if(context->have_card)
      {
         CardMiscControl(card_handle, 0);
         CardResetCard(card_handle);
      }
      ReleaseCard(card_handle, CARDF_REMOVEHANDLE);
      UnwrapCardInt(card_handle->cah_CardStatus, base);
      UnwrapCardInt(card_handle->cah_CardInserted, base);
      UnwrapCardInt(card_handle->cah_CardRemoved, base);

      FreeVec(card_handle->cah_CardStatus);
      FreeVec(card_handle->cah_CardInserted);
      FreeVec(card_handle->cah_CardRemoved);
      FreeVec(context->tuple_buffer);
      FreeMem(card_handle, sizeof(struct CardHandle));

      FreeMem(context, sizeof(struct BusContext));
   }

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

static BOOL IsCardCompatible(struct BusContext *context,
   struct DevBase *base)
{
   BOOL success = TRUE;
   struct CardHandle *card_handle;
   UBYTE *tuple_buffer;
   const struct TagItem *tuple_tags = NULL;
   ULONG code;
   const ULONG *p;
   UWORD maker = 0, product = 0;

   card_handle = context->card_handle;
   tuple_buffer = context->tuple_buffer;

   /* Get card's make and model */

   if(CopyTuple(card_handle, tuple_buffer, PCCARD_TPL_MANFID,
      MAX_TUPLE_SIZE))
   {
      tuple_tags = PCCard_GetTupleInfo(tuple_buffer);
      if(tuple_tags != NULL)
      {
         maker = GetTagData(PCCARD_Maker, 0, tuple_tags);
         product = GetTagData(PCCARD_Product, 0, tuple_tags);
      }
   }

   /* Check this is a card we can use */

   code = maker << 16 | product;
   for(success = FALSE, p = product_codes; *p != 0; p++)
      if(*p == code)
         success = TRUE;
   PCCard_FreeTupleInfo(tuple_tags);

   return success;
}



/****i* prism2.device/InitialiseCard ***************************************
*
*   NAME
*	InitialiseCard
*
*   SYNOPSIS
*	success = InitialiseCard(context)
*
*	BOOL InitialiseCard(struct BusContext *);
*
****************************************************************************
*
*/

static BOOL InitialiseCard(struct BusContext *context,
   struct DevBase *base)
{
   BOOL success = TRUE;
   struct CardHandle *card_handle;
   struct CardMemoryMap *card_map;
   UBYTE config_value, i, window_count, *tuple_buffer;
   const struct TagItem *tuple_tags = NULL;
   ULONG *io_bases, *io_lengths, io_base_offset = 0, config_base_offset;

   /* Wake up card's I/O functionality */

   card_handle = context->card_handle;
   tuple_buffer = context->tuple_buffer;
   CardMiscControl(card_handle,
      CARD_ENABLEF_DIGAUDIO | CARD_DISABLEF_WP);

   /* Get configuration data */

   if(!CopyTuple(card_handle, tuple_buffer, PCCARD_TPL_CONFIG,
      MAX_TUPLE_SIZE))
      success = FALSE;

   if(success)
   {
      PCCard_FreeTupleInfo(tuple_tags);
      tuple_tags = PCCard_GetTupleInfo(tuple_buffer);
      if(tuple_tags == NULL)
         success = FALSE;
   }

   if(success)
   {
      config_base_offset = GetTagData(PCCARD_RegisterBase, 0, tuple_tags);

      PCCard_FreeTupleInfo(tuple_tags);
      tuple_tags = NULL;

      /* Get IO base */

      if(!CopyTuple(card_handle, tuple_buffer, PCCARD_TPL_CFTABLEENTRY,
         MAX_TUPLE_SIZE))
         success = FALSE;
   }

   if(success)
   {
      tuple_tags = PCCard_GetTupleInfo(tuple_buffer);
      if(tuple_tags == NULL)
         success = FALSE;
   }

   if(success)
   {
      config_value = GetTagData(PCCARD_ModeNo, 0, tuple_tags);

      io_bases =
         (APTR)GetTagData(PCCARD_IOWinBases, (UPINT)NULL, tuple_tags);
      if(io_bases == NULL)
         success = FALSE;
   }

   /* Find the appropriate IO window */

   if(success)
   {
      io_lengths =
         (APTR)GetTagData(PCCARD_IOWinLengths, (UPINT)NULL, tuple_tags);

      window_count = GetTagData(PCCARD_IOWinCount, 0, tuple_tags);

      for(i = 0; i < window_count && io_base_offset == 0; i++)
         if(io_lengths[i] == IO_WINDOW_SIZE)
            io_base_offset = io_bases[i];
   }

   PCCard_FreeTupleInfo(tuple_tags);

   /* Configure card */

   if(success)
   {
      card_map = GetCardMap();
      context->config_base =
         (UPINT)card_map->cmm_AttributeMemory + config_base_offset;

      context->io_base = (UPINT)card_map->cmm_IOMemory + io_base_offset;
      BYTEOUT(context->config_base + PCCARD_REG_COR, config_value);
      BYTEOUT(context->config_base + PCCARD_REG_CCSR,
         BYTEIN(context->config_base + PCCARD_REG_CCSR)
         | PCCARD_REG_CCSRF_AUDIOENABLE);
   }

   return success;
}



/****i* prism2.device/CardRemovedHook **************************************
*
*   NAME
*	CardRemovedHook
*
*   SYNOPSIS
*	CardRemovedHook(context)
*
*	VOID CardRemovedHook(struct BusContext *);
*
****************************************************************************
*
*/

static VOID CardRemovedHook(struct BusContext *context,
   struct DevBase *base)
{
   ReleaseCard(context->card_handle, 0);

   return;
}



/****i* prism2.device/CardInsertedHook *************************************
*
*   NAME
*	CardInsertedHook
*
*   SYNOPSIS
*	success = CardInsertedHook(context)
*
*	BOOL CardInsertedHook(struct BusContext *);
*
****************************************************************************
*
*/

static BOOL CardInsertedHook(struct BusContext *context,
   struct DevBase *base)
{
   BOOL success = TRUE;

   success = IsCardCompatible(context, base);

   if(success)
      success = InitialiseCard(context, base);

   if(success)
      success = InitialiseAdapter(context->unit, TRUE, base);

   if(!success)
      ReleaseCard(context->card_handle, 0);

   return success;
}



/****i* prism2.device/CardRemovedInt ***************************************
*
*   NAME
*	CardRemovedInt
*
*   SYNOPSIS
*	CardRemovedInt(context)
*
*	VOID CardRemovedInt(struct BusContext *);
*
****************************************************************************
*
*/

static VOID CardRemovedInt(REG(a1, struct BusContext *context),
   REG(a6, APTR int_code))
{
   struct DevBase *base;
   struct DevUnit *unit;

   /* Record loss of card and get our task to call ReleaseCard() */

   unit = context->unit;
   if (unit != NULL)
   {
      base = unit->device;
      if((unit->flags & UNITF_ONLINE) != 0)
         unit->flags |= UNITF_WASONLINE;
      unit->flags &= ~(UNITF_HAVEADAPTER | UNITF_ONLINE);
   }
   context->have_card = FALSE;
   if (unit != NULL)
      Signal(unit->task, unit->card_removed_signal);

   return;
}



/****i* prism2.device/CardInsertedInt **************************************
*
*   NAME
*	CardInsertedInt
*
*   SYNOPSIS
*	CardInsertedInt(context)
*
*	VOID CardInsertedInt(struct BusContext *);
*
****************************************************************************
*
*/

static VOID CardInsertedInt(REG(a1, struct BusContext *context),
   REG(a6, APTR int_code))
{
   struct DevBase *base;
   struct DevUnit *unit;

   unit = context->unit;
   if (unit != NULL) {
      base = unit->device;
      context->have_card = TRUE;
      Signal(unit->task, unit->card_inserted_signal);
   }

   return;
}



/****i* prism2.device/CardStatusInt ****************************************
*
*   NAME
*	CardStatusInt
*
*   SYNOPSIS
*	mask = CardStatusInt(context, int_code, mask)
*
*	UBYTE CardStatusInt(struct BusContext *, APTR, UBYTE);
*
****************************************************************************
*
* int_code is really in A5, but GCC 2.95.3 doesn't seem able to handle that.
* Since we don't use this parameter, we can lie.
*
*/

static UBYTE CardStatusInt(REG(a1, struct BusContext *context),
   REG(a6, APTR int_code), REG(d0, UBYTE mask))
{
   if(context->resource_version < 39)
   {
      /* Work around gayle interrupt bug */

      *((volatile UBYTE *)0xda9000) = (mask ^ 0x2c) | 0xc0;
      mask = 0;
   }

   if(context->unit != NULL)
      StatusInt(context->unit, StatusInt);

   return mask;
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
   WORDSIN(context->io_base + offset, buffer, count);

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
   WORDSOUT(context->io_base + offset, buffer, count);

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
   BEWORDOUT(context->io_base + offset, value);

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
   return LEWORDIN(context->io_base + offset);
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
   LEWORDOUT(context->io_base + offset, value);

   return;
}



