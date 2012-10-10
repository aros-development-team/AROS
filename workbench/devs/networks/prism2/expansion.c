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
#include <expansion/pci.h>

#include <proto/exec.h>
#include <proto/expansion.h>

#include "pci.h"
#include "plx9052.h"
#include "prism2.h"

#include "pci_protos.h"
#include "expansion_protos.h"
#include "timer_protos.h"


/* Private prototypes */

static VOID WordsInIOHook(struct BusContext *context, ULONG offset,
   UWORD *buffer, ULONG count);
static VOID WordsOutIOHook(struct BusContext *context, ULONG offset,
   const UWORD *buffer, ULONG count);
static VOID BEWordOutIOHook(struct BusContext *context, ULONG offset,
   UWORD value);
static UWORD LEWordInIOHook(struct BusContext *context, ULONG offset);
static VOID LEWordOutIOHook(struct BusContext *context, ULONG offset,
   UWORD value);


IMPORT const UWORD product_codes[];


static const struct TagItem bridge_unit_tags[] =
{
   {IOTAG_WordsIn, (UPINT)WordsInIOHook},
   {IOTAG_WordsOut, (UPINT)WordsOutIOHook},
   {IOTAG_BEWordOut, (UPINT)BEWordOutIOHook},
   {IOTAG_LEWordIn, (UPINT)LEWordInIOHook},
   {IOTAG_LEWordOut, (UPINT)LEWordOutIOHook},
   {TAG_END, 0}
};


/****i* prism2.device/GetExpansionCount ************************************
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



/****i* prism2.device/AllocExpansionCard ***********************************
*
*   NAME
*	AllocExpansionCard
*
*   SYNOPSIS
*	context = AllocExpansionCard(index)
*
*	struct BusContext *AllocExpansionCard(ULONG);
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
   UBYTE io_range_no;
   ULONG value;
   UPINT int_reg;
   volatile UBYTE *cor_reg;

   /* Find a compatible card */

   context = AllocMem(sizeof(struct BusContext), MEMF_PUBLIC | MEMF_CLEAR);
   if(context == NULL)
      success = FALSE;

   if(success)
   {
      context->card = card =
         base->i_pci->FindDeviceTags(FDT_CandidateList, product_codes,
         FDT_Index, index, TAG_END);
      if(card == NULL)
         success = FALSE;
   }

   /* Lock card */

   if(success)
      success = card->Lock(PCI_LOCK_EXCLUSIVE);

   if(success)
   {
      /* Find out what type of Prism II PCI card this is */

      card->SetEndian(PCI_MODE_BIG_ENDIAN);
      context->bus_type = GetBusType(card->ReadConfigWord(PCI_DEVICE_ID),
         base);

      if(context->bus_type == TMD_BUS)
      {
         /* Reset and enable the PCCard */

         io_range = card->GetResourceRange(1);
         card->OutByte(io_range->BaseAddress, COR_RESET);
         BusyMilliDelay(RESET_DELAY, base);
         card->OutByte(io_range->BaseAddress, COR_ENABLE);
         BusyMilliDelay(RESET_DELAY, base);
         card->FreeResourceRange(io_range);
         io_range_no = 2;
         context->unit_tags = bridge_unit_tags;
      }
      else if(context->bus_type == PLX_BUS)
      {
         /* Reset and enable the PCCard */

         io_range = card->GetResourceRange(2);
         cor_reg = (volatile UBYTE *)io_range->BaseAddress + 0x3e0;
         *cor_reg = COR_ENABLE;
         BusyMilliDelay(RESET_DELAY, base);
         card->FreeResourceRange(io_range);

         /* Enable interrupts on the bridge */

         io_range = card->GetResourceRange(1);
         int_reg = io_range->BaseAddress + PLX9052_INTS;
         card->FreeResourceRange(io_range);
         value = card->InLong(int_reg);
         card->OutLong(int_reg, value | (1 << 6));
         if((card->InLong(int_reg) & (1 << 6)) == 0)
            success = FALSE;
         io_range_no = 3;
         context->unit_tags = bridge_unit_tags;
      }
      else
      {
         card->WriteConfigWord(PCI_COMMAND, PCI_COMMAND_MEMORY);
            /* For PegII */
         io_range_no = 0;
      }

      /* Get the I/O base of the wireless chip */

      context->have_card = TRUE;
      card->SetEndian(PCI_MODE_LITTLE_ENDIAN);
      io_range = card->GetResourceRange(io_range_no);
      context->io_base = io_range->BaseAddress;
      card->FreeResourceRange(io_range);

      if(context->bus_type == PCI_BUS)
      {
         /* Reset and enable the card */

         cor_reg = (volatile UBYTE *)context->io_base + (P2_REG_PCICOR * 2);
         *cor_reg = COR_RESET;
         BusyMilliDelay(250, base);
         *cor_reg = 0;
         BusyMilliDelay(500, base);
      }
   }

   if(!success)
   {
      FreeExpansionCard(context, base);
      context = NULL;
   }

   return context;
}



/****i* prism2.device/FreeExpansionCard ************************************
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
   struct PCIResourceRange *io_range = NULL;
   ULONG value;
   UPINT int_reg;
   volatile UBYTE *cor_reg;

   if(context != NULL)
   {
      card = context->card;
      if(card != NULL)
      {
         if(context->bus_type == TMD_BUS)
         {
            /* Disable the PCCard */

            io_range = card->GetResourceRange(1);
            card->OutByte(io_range->BaseAddress, 0);
            card->FreeResourceRange(io_range);
         }
         else if(context->bus_type == PLX_BUS)
         {
            /* Disable interrupts on the bridge */

            io_range = card->GetResourceRange(1);
            int_reg = io_range->BaseAddress + PLX9052_INTS;
            card->FreeResourceRange(io_range);
            value = card->InLong(int_reg);
            card->OutLong(int_reg, value & ~(1 << 6));

            /* Disable the PCCard */

            io_range = card->GetResourceRange(2);
            cor_reg = (volatile UBYTE *)io_range->BaseAddress + 0x3e0;
            *cor_reg = COR_RESET;
            BusyMilliDelay(250, base);
            *cor_reg = 0;

            card->FreeResourceRange(io_range);
         }

         if(context->have_card)
            card->Unlock();
         base->i_pci->FreeDevice(card);
         FreeMem(context, sizeof(struct BusContext));
      }
   }

   return;
}



/****i* prism2.device/AddExpansionIntServer ********************************
*
*   NAME
*	AddExpansionIntServer
*
*   SYNOPSIS
*	success = AddExpansionIntServer(card, interrupt)
*
*	BOOL AddExpansionIntServer(APTR, struct Interrupt *);
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



/****i* prism2.device/RemExpansionIntServer ********************************
*
*   NAME
*	RemExpansionIntServer
*
*   SYNOPSIS
*	RemExpansionIntServer(card, interrupt)
*
*	VOID RemExpansionIntServer(APTR, struct Interrupt *);
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
   struct PCIDevice *card;

   card = context->card;
   while(count-- > 0)
      *buffer++ = card->InWord(context->io_base + offset);

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
   struct PCIDevice *card;

   card = context->card;
   while(count-- > 0)
      card->OutWord(context->io_base + offset, *buffer++);

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
   struct PCIDevice *card;

   card = context->card;
   card->OutWord(context->io_base + offset, MakeBEWord(value));

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
   struct PCIDevice *card;

   card = context->card;
   return LEWord(card->InWord(context->io_base + offset));
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
   struct PCIDevice *card;

   card = context->card;
   card->OutWord(context->io_base + offset, MakeLEWord(value));

   return;
}



