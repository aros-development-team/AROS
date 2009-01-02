/*

Copyright (C) 2000-2006 Neil Cafferkey

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
/*#include <utility/utility.h>*/
#include <libraries/isapnp.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/isapnp.h>

#include "device.h"

#include "isa_protos.h"
#include "unit_protos.h"


struct BusContext
{
   struct DevUnit *unit;
   struct ISAPNP_Device *card;
   UPINT io_base;
   APTR lock;
};


/* Private prototypes */

static struct DevUnit *FindISAUnit(ULONG index, struct DevBase *base);
static struct DevUnit *CreateISAUnit(ULONG index, struct DevBase *base);
static struct BusContext *AllocCard(ULONG index, struct DevBase *base);
static VOID FreeCard(struct BusContext *card, struct DevBase *base);
static BOOL IsCardCompatible(struct ISAPNP_Device *card,
   struct DevBase *base);
/*static BOOL InitialiseCard(struct BusContext *card, struct DevBase *base);*/
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


static const UWORD product_codes[] =
{
   0x6d50, 0x5090,
   0x6d50, 0x5091,
   0x6d50, 0x5094,
   0x6d50, 0x5095,
   0x6d50, 0x5098,
/*   'PNP', 0x80f7,
   'PNP', 0x80f8,*/
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


/****i* etherlink3.device/GetISACount **************************************
*
*   NAME
*	GetISACount -- Get the number of compatible ISA cards.
*
*   SYNOPSIS
*	count = GetISACount()
*
*	ULONG GetISACount();
*
****************************************************************************
*
*/

ULONG GetISACount(struct DevBase *base)
{
   ULONG count = 0;
   struct ISAPNP_Device *card = NULL;

   while((card = ISAPNP_FindDevice(card, -1, -1, -1)) != NULL)
   {
      if(IsCardCompatible(card, base))
         count++;
   }

   return count;
}



/****i* etherlink3.device/GetISAUnit ***************************************
*
*   NAME
*	GetISAUnit -- Get a unit by number.
*
*   SYNOPSIS
*	unit = GetISAUnit(index)
*
*	struct DevUnit *GetISAUnit(ULONG);
*
****************************************************************************
*
*/

struct DevUnit *GetISAUnit(ULONG index, struct DevBase *base)
{
   struct DevUnit *unit;
   struct BusContext *card;

   unit = FindISAUnit(index, base);

   if(unit == NULL)
   {
      card = CreateISAUnit(index, base);
      if(card != NULL)
      {
         unit = card->unit;
         AddTail((APTR)&base->isa_units, (APTR)unit);
      }
   }

   return unit;
}



/****i* etherlink3.device/FindISAUnit **************************************
*
*   NAME
*	FindISAUnit -- Find an existing unit by number.
*
*   SYNOPSIS
*	unit = FindISAUnit(index)
*
*	struct DevUnit *FindISAUnit(ULONG);
*
****************************************************************************
*
*/

static struct DevUnit *FindISAUnit(ULONG index, struct DevBase *base)
{
   struct DevUnit *unit, *tail;
   BOOL found = FALSE;

   unit = (APTR)base->isa_units.mlh_Head;
   tail = (APTR)&base->isa_units.mlh_Tail;

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



/****i* etherlink3.device/CreateISAUnit ************************************
*
*   NAME
*	CreateISAUnit -- Create an ISA unit.
*
*   SYNOPSIS
*	unit = CreateISAUnit(index)
*
*	struct DevUnit *CreateISAUnit(ULONG);
*
*   FUNCTION
*	Creates a new ISA unit.
*
****************************************************************************
*
*/

struct DevUnit *CreateISAUnit(ULONG index, struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   struct DevUnit *unit;
   UWORD generation, id, i;
   UPINT io_base, id_port;

   context = AllocCard(index, base);
   if(context == NULL)
      success = FALSE;



#if 0   /* Stuff below is for non-PnP only? */

   /* Send initialisation ID sequence */

   id_port = 0xef0100;   /* !!! */
   BYTEOUT(id_port, 0);
   BYTEOUT(id_port, 0);

   id = 0xff;
   for(i = 0; i < 0xff; i++)
   {
      BYTEOUT(id_port, (UBYTE)id);
      id <<= 1;
      if ((id & 0x100) != 0)
         id ^= 0xcf;
   }

   /* Activate adapter */

   BYTEOUT(id_port, 0xff);

   io_base = 0xef0300;
   generation = FIRST_GEN;
#endif


   if(success)
   {
      generation = SECOND_GEN;
      unit = CreateUnit(unit_num, NULL, io_base, generation, ISA_BUS, base);
      if(unit == NULL)
         success = FALSE;
   }

   /* Add interrupt */

   if(success)
      AddIntServer(x, &unit->status_int);

   if(!success)
   {
      DeleteISAUnit(unit, base);
      unit = NULL;
   }

   return unit;
}



/****i* etherlink3.device/DeleteISAUnit ************************************
*
*   NAME
*	DeleteISAUnit -- Delete a unit.
*
*   SYNOPSIS
*	DeleteISAUnit(unit)
*
*	VOID DeleteISAUnit(struct DevUnit *);
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

VOID DeleteISAUnit(struct DevUnit *unit, struct DevBase *base)
{
   if(unit != NULL)
   {
      RemIntServer(x, &unit->status_int);
      DeleteUnit(unit, base);
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

static struct BusContext *AllocCard(ULONG index, struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   struct ISAPNP_Device *card = NULL;
   ULONG i;

   /* Find a compatible card */

   card = AllocMem(sizeof(struct BusContext), MEMF_PUBLIC | MEMF_CLEAR);
   if(card == NULL)
      success = FALSE;

   if(success)
   {
      while(i <= index)
      {
         card = ISAPNP_FindDevice(card, -1, -1, -1);
         if(IsCardCompatible(card, base))
            i++;
      }
      context->card = card;
   }

   /* Get base address */

   if(success)
   {
   }

   /* Lock card */

   if(success)
   {
      context->lock = ISAPNP_LockDevices(ISAPNP_LOCKF_NONE, card);
      if(context->lock == NULL)
         success = FALSE;
   }

   if(!success)
   {
      FreeCard(context, base);
   }

   return card;
}



/****i* etherlink3.device/FreeCard *****************************************
*
*   NAME
*	FreeCard
*
****************************************************************************
*
*/

static VOID FreeCard(struct BusContext *context, struct DevBase *base)
{
   struct ISAPNP_Device *card = NULL;

   if(context != NULL)
   {
      /* Unlock card */

      ISAPNP_UnlockDevices(context->lock);

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
*	compatible = IsCardCompatible(card)
*
*	BOOL IsCardCompatible(struct ISAPNP_Device *);
*
****************************************************************************
*
*/

static BOOL IsCardCompatible(struct ISAPNP_Device *card,
   struct DevBase *base)
{
   struct ISAPNP_Identifier *id, *tail;
   BOOL compatible = FALSE;
   const UWORD *p;

   id = (APTR)card->isapnpd_IDs.mlh_Head;
   tail = (APTR)&card->isapnpd_IDs.mlh_Tail;

   while(id != tail)
   {
      vendor_id =
         ISAPNP_MAKE_ID(id->isapnpid_Vendor[0], id->isapnpid_Vendor[1],
         id->isapnpid_Vendor[2]);
      product_id = id->isapnpid_ProductID;

      for(p = product_codes; p[0] != 0; p += 2)
      {
         if(p[0] == vendor_id && p[1] == product_id)
            compatible = TRUE;
      }

      id = (APTR)id->isapnpid_MinNode.mln_Succ;
   }

   return compatible;
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



