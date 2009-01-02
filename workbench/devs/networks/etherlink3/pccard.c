/*

Copyright (C) 2000-2005 Neil Cafferkey

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
/*#include "io.h"*/

#include "pccard_protos.h"
#include "unit_protos.h"

#define MAX_TUPLE_SIZE 0xff
#define TUPLE_BUFFER_SIZE (MAX_TUPLE_SIZE + 8)
#define HANDLE_PRIORITY 10
#define IO_WINDOW_SIZE 0x10


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
static UBYTE CardStatusInt(REG(d0, UBYTE mask),
   REG(a1, struct BusContext *context), REG(a6, APTR int_code));
static UBYTE ByteInHook(struct BusContext *context, ULONG offset);
static ULONG LongInHook(struct BusContext *context, ULONG offset);
static VOID ByteOutHook(struct BusContext *context, ULONG offset,
   UBYTE value);
static VOID WordOutHook(struct BusContext *context, ULONG offset,
   UWORD value);
static VOID LongOutHook(struct BusContext *context, ULONG offset,
   ULONG value);
static VOID LongsInHook(struct BusContext *context, ULONG offset,
   ULONG *buffer, ULONG count);
static VOID LongsOutHook(struct BusContext *context, ULONG offset,
   const ULONG *buffer, ULONG count);
static VOID BEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value);
static UWORD LEWordInHook(struct BusContext *context, ULONG offset);
static ULONG LELongInHook(struct BusContext *context, ULONG offset);
static VOID LEWordOutHook(struct BusContext *context, ULONG offset,
   UWORD value);
static VOID LELongOutHook(struct BusContext *context, ULONG offset,
   ULONG value);


static const ULONG product_codes[] =
{
   0x01010035,
   0x0101003d,
   0x01010562,
   0x01010589,
   0
};


static const struct TagItem unit_tags[] =
{
   {IOTAG_ByteIn, (UPINT)ByteInHook},
   {IOTAG_LongIn, (UPINT)LongInHook},
   {IOTAG_ByteOut, (UPINT)ByteOutHook},
   {IOTAG_WordOut, (UPINT)WordOutHook},
   {IOTAG_LongOut, (UPINT)LongOutHook},
   {IOTAG_LongsIn, (UPINT)LongsInHook},
   {IOTAG_LongsOut, (UPINT)LongsOutHook},
   {IOTAG_BEWordOut, (UPINT)BEWordOutHook},
   {IOTAG_LEWordIn, (UPINT)LEWordInHook},
   {IOTAG_LELongIn, (UPINT)LELongInHook},
   {IOTAG_LEWordOut, (UPINT)LEWordOutHook},
   {IOTAG_LELongOut, (UPINT)LELongOutHook},
   {TAG_END, 0}
};


/****i* etherlink3.device/GetPCCardCount ***********************************
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


/****i* etherlink3.device/GetPCCardUnit ************************************
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



/****i* etherlink3.device/FindPCCardUnit ***********************************
*
*   NAME
*	FindPCCardUnit -- Find a unit by number.
*
*   SYNOPSIS
*	unit = FindPCCardUnit(unit_num)
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



/****i* etherlink3.device/CreatePCCardUnit *********************************
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
         CreateUnit(index, context, unit_tags, SECOND_GEN,
            PCCARD_BUS, base);
      if(unit == NULL)
         success = FALSE;
   }

   if(success)
   {
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



/****i* etherlink3.device/DeletePCCardUnit *********************************
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
      context = unit->card;
      DeleteUnit(unit, base);
      FreeCard(context, base);
   }

   return;
}



/****i* etherlink3.device/AllocCard ****************************************
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
   BOOL success = TRUE, have_card = FALSE;
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
/*      card_handle->cah_CardFlags = CARDF_IFAVAILABLE | CARDF_POSTSTATUS;*/
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

      if(OwnCard(card_handle) != 0)
         success = FALSE;
   }

   if(success)
   {
      have_card = TRUE;
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



/****i* etherlink3.device/FreeCard *****************************************
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

      FreeVec(card_handle->cah_CardStatus);
      FreeVec(card_handle->cah_CardInserted);
      FreeVec(card_handle->cah_CardRemoved);
      FreeVec(context->tuple_buffer);
      FreeMem(card_handle, sizeof(struct CardHandle));

      FreeMem(context, sizeof(struct BusContext));
   }

   return;
}



/****i* etherlink3.device/IsCardCompatible *********************************
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



/****i* etherlink3.device/InitialiseCard ***********************************
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

      for(i = 0; (i < window_count) && (io_base_offset == 0); i++)
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



/****i* etherlink3.device/CardRemovedHook **********************************
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



/****i* etherlink3.device/CardInsertedHook *********************************
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



/****i* etherlink3.device/CardRemovedInt ***********************************
*
*   NAME
*	CardRemovedInt
*
*   SYNOPSIS
*	CardRemovedInt(unit)
*
*	VOID CardRemovedInt(struct DevUnit *);
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
   base = unit->device;
   if((unit->flags & UNITF_ONLINE) != 0)
      unit->flags |= UNITF_WASONLINE;
   unit->flags &= ~(UNITF_HAVEADAPTER | UNITF_ONLINE);
   context->have_card = FALSE;
   Signal(unit->task, unit->card_removed_signal);

   return;
}



/****i* etherlink3.device/CardInsertedInt **********************************
*
*   NAME
*	CardInsertedInt
*
*   SYNOPSIS
*	CardInsertedInt(unit)
*
*	VOID CardInsertedInt(struct DevUnit *);
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
   base = unit->device;
   context->have_card = TRUE;
   Signal(unit->task, unit->card_inserted_signal);

   return;
}



/****i* etherlink3.device/CardStatusInt ************************************
*
*   NAME
*	CardStatusInt
*
*   SYNOPSIS
*	mask = CardStatusInt(mask, unit)
*
*	UBYTE CardStatusInt(UBYTE mask, struct DevUnit *);
*
****************************************************************************
*
* We pretend the int_code parameter goes in A6 rather than A5 because 68k
* GCC can't cope with A5 and we know the parameter isn't used in this case.
*
*/

static UBYTE CardStatusInt(REG(d0, UBYTE mask),
   REG(a1, struct BusContext *context), REG(a6, APTR int_code))
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



/****i* etherlink3.device/ByteInHook ***************************************
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



/****i* etherlink3.device/LongInHook ***************************************
*
*   NAME
*	LongInHook
*
*   SYNOPSIS
*	value = LongInHook(context, offset)
*
*	ULONG LongInHook(struct BusContext *, ULONG);
*
****************************************************************************
*
*/

static ULONG LongInHook(struct BusContext *context, ULONG offset)
{
   return LONGIN(context->io_base + offset);
}



/****i* etherlink3.device/ByteOutHook **************************************
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



/****i* etherlink3.device/WordOutHook **************************************
*
*   NAME
*	WordOutHook
*
*   SYNOPSIS
*	WordOutHook(context, offset, value)
*
*	VOID WordOutHook(struct BusContext *, ULONG, UWORD);
*
****************************************************************************
*
*/

static VOID WordOutHook(struct BusContext *context, ULONG offset,
   UWORD value)
{
   WORDOUT(context->io_base + offset, value);

   return;
}



/****i* etherlink3.device/LongOutHook **************************************
*
*   NAME
*	LongOutHook
*
*   SYNOPSIS
*	LongOutHook(context, offset, value)
*
*	VOID LongOutHook(struct BusContext *, ULONG, ULONG);
*
****************************************************************************
*
*/

static VOID LongOutHook(struct BusContext *context, ULONG offset,
   ULONG value)
{
   LONGOUT(context->io_base + offset, value);

   return;
}



/****i* etherlink3.device/LongsInHook **************************************
*
*   NAME
*	LongsInHook
*
*   SYNOPSIS
*	LongsInHook(context, offset, buffer, count)
*
*	VOID LongsInHook(struct BusContext *, ULONG, ULONG *, ULONG);
*
****************************************************************************
*
*/

static VOID LongsInHook(struct BusContext *context, ULONG offset,
   ULONG *buffer, ULONG count)
{
   LONGSIN(context->io_base + offset, buffer, count);

   return;
}



/****i* etherlink3.device/LongsOutHook *************************************
*
*   NAME
*	LongsOutHook
*
*   SYNOPSIS
*	LongsOutHook(context, offset, buffer, count)
*
*	VOID LongsOutHook(struct BusContext *, ULONG, const ULONG *, ULONG);
*
****************************************************************************
*
*/

static VOID LongsOutHook(struct BusContext *context, ULONG offset,
   const ULONG *buffer, ULONG count)
{
   LONGSOUT(context->io_base + offset, buffer, count);

   return;
}



/****i* etherlink3.device/BEWordOutHook ************************************
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



/****i* etherlink3.device/LEWordInHook *************************************
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



/****i* etherlink3.device/LELongInHook ***************************************
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



/****i* etherlink3.device/LEWordOutHook ************************************
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



/****i* etherlink3.device/LELongOutHook ************************************
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



