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
#include <expansion/pci.h>

#include <proto/exec.h>
#include <proto/expansion.h>

#include "device.h"
#include "pci.h"

#include "pci_protos.h"
#include "expansion_protos.h"


/* Private prototypes */

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
static APTR AllocDMAMemHook(struct BusContext *context, UPINT size,
   UWORD alignment);
static VOID FreeDMAMemHook(struct BusContext *context, APTR mem);


IMPORT const UWORD product_codes[];


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
   {IOTAG_AllocDMAMem, (UPINT)AllocDMAMemHook},
   {IOTAG_FreeDMAMem, (UPINT)FreeDMAMemHook},
   {TAG_END, 0}
};


/****i* etherlink3.device/GetExpansionCount ********************************
*
*   NAME
*	GetExpansionCount -- Get the number of compatible PCI Cards.
*
*   SYNOPSIS
*	count = GetExpansionCount()
*
*	ULONG GetExpansionCount();
*
****************************************************************************
*
*/

ULONG GetExpansionCount(struct DevBase *base)
{
   ULONG count = 0;
   struct PCIDevice *card;

   if(base->i_pci != NULL)
   {
      while((card =
         base->i_pci->FindDeviceTags(FDT_CandidateList, product_codes,
         FDT_Index, count, TAG_END)) != NULL)
      {
         base->i_pci->FreeDevice(card);
         count++;
      }
   }

   return count;
}



/****i* etherlink3.device/AllocExpansionCard *******************************
*
*   NAME
*	AllocExpansionCard -- Get card from system.
*
*   SYNOPSIS
*	unit = AllocExpansionCard(index)
*
*	struct DevPCI *AllocExpansionCard(ULONG);
*
****************************************************************************
*
*/

struct BusContext *AllocExpansionCard(ULONG index, struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   struct PCIDevice *card = NULL;
   struct PCIResourceRange *io_range = NULL;
   UWORD product_id;

   /* Find a compatible card */

   context = AllocMem(sizeof(struct BusContext), MEMF_PUBLIC | MEMF_CLEAR);
   if(context == NULL)
      success = FALSE;

   if(success)
   {
      context->card = card =
         base->i_pci->FindDeviceTags(FDT_CandidateList, product_codes,
         FDT_Index, index, TAG_END);
      product_id = card->ReadConfigWord(PCI_DEVICE_ID);
      if(card == NULL)
         success = FALSE;
   }

   /* Lock card */

   if(success)
      success = card->Lock(PCI_LOCK_EXCLUSIVE);

   /* Get base address and generation */

   if(success)
   {
      context->have_card = TRUE;
      card->SetEndian(PCI_MODE_LITTLE_ENDIAN);
      io_range = card->GetResourceRange(BAR_NO);
      context->io_base = io_range->BaseAddress;
      card->FreeResourceRange(io_range);
      context->unit_tags = unit_tags;
      context->generation = GetGeneration(product_id, base);
   }

   if(!success)
   {
      FreeExpansionCard(context, base);
      context = NULL;
   }

   return context;
}



/****i* etherlink3.device/FreeExpansionCard ********************************
*
*   NAME
*	FreeExpansionCard
*
*   SYNOPSIS
*	FreeExpansionCard(context)
*
*	VOID FreeExpansionCard(struct BusContext *);
*
****************************************************************************
*
*/

VOID FreeExpansionCard(struct BusContext *context, struct DevBase *base)
{
   struct PCIDevice *card;

   if(context != NULL)
   {
      card = context->card;
      if(card != NULL)
      {
         if(context->have_card)
            card->Unlock();
         base->i_pci->FreeDevice(card);
         FreeMem(context, sizeof(struct BusContext));
      }
   }

   return;
}



/****i* etherlink3.device/AddExpansionIntServer ****************************
*
*   NAME
*	AddExpansionIntServer
*
*   SYNOPSIS
*	context = AddExpansionIntServer(index)
*
*	struct BusContext *AddExpansionIntServer(ULONG);
*
****************************************************************************
*
*/

BOOL AddExpansionIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   return AddIntServer(((struct PCIDevice *)card)->MapInterrupt(),
      interrupt);
}



/****i* etherlink3.device/RemExpansionIntServer ****************************
*
*   NAME
*	RemExpansionIntServer
*
*   SYNOPSIS
*	RemExpansionIntServer()
*
*	VOID RemExpansionIntServer(ULONG);
*
****************************************************************************
*
*/

VOID RemExpansionIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   RemIntServer(((struct PCIDevice *)card)->MapInterrupt(), interrupt);

   return;
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
   struct PCIDevice *card;

   card = context->card;
   return card->InByte(context->io_base + offset);
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
   struct PCIDevice *card;

   card = context->card;
   return card->InLong(context->io_base + offset);
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
   struct PCIDevice *card;

   card = context->card;
   card->OutByte(context->io_base + offset, value);

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
   struct PCIDevice *card;

   card = context->card;
   card->OutWord(context->io_base + offset, value);

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
   struct PCIDevice *card;

   card = context->card;
   card->OutLong(context->io_base + offset, value);

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
   struct PCIDevice *card;

   card = context->card;
   while(count-- > 0)
      *buffer++ = card->InLong(context->io_base + offset);

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
   struct PCIDevice *card;

   card = context->card;
   while(count-- > 0)
      card->OutLong(context->io_base + offset, *buffer++);

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
   struct PCIDevice *card;

   card = context->card;
   card->OutWord(context->io_base + offset, MakeBEWord(value));

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
   struct PCIDevice *card;

   card = context->card;
   return LEWord(card->InWord(context->io_base + offset));
}



/****i* etherlink3.device/LELongInHook *************************************
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
   struct PCIDevice *card;

   card = context->card;
   return LELong(card->InLong(context->io_base + offset));
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
   struct PCIDevice *card;

   card = context->card;
   card->OutWord(context->io_base + offset, MakeLEWord(value));

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
   struct PCIDevice *card;

   card = context->card;
   card->OutLong(context->io_base + offset, MakeLELong(value));

   return;
}



/****i* etherlink3.device/AllocDMAMemHook **********************************
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



/****i* etherlink3.device/FreeDMAMemHook ***********************************
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
         FreeMem(*((APTR *)mem - 1), *((UPINT *)mem - 2));

   return;
}



