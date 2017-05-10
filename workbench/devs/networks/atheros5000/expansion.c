/*

Copyright (C) 2004-2013 Neil Cafferkey

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

#include "pci_protos.h"
#include "expansion_protos.h"
#include "hal/ah.h"


IMPORT const UWORD product_codes[];


/****i* atheros5000.device/GetExpansionCount *******************************
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



/****i* atheros5000.device/AllocExpansionCard ******************************
*
*   NAME
*	AllocExpansionCard -- Take control of a card.
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

   /* Find a compatible card */

   context = AllocMem(sizeof(struct BusContext), MEMF_PUBLIC | MEMF_CLEAR);
   if(context == NULL)
      success = FALSE;

   if(success)
   {
      context->card = card =
         base->i_pci->FindDeviceTags(FDT_CandidateList, product_codes,
         FDT_Index, index, TAG_END);
      context->id = card->ReadConfigWord(PCI_DEVICE_ID);
      if(card == NULL)
         success = FALSE;
   }

   /* Lock card */

   if(success)
      success = card->Lock(PCI_LOCK_EXCLUSIVE);

   if(success)
   {
      card->WriteConfigWord(PCI_COMMAND,
         PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER); /* For PegII */

      /* Get the I/O base of the wireless chip */

      context->have_card = TRUE;
      card->SetEndian(PCI_MODE_LITTLE_ENDIAN);
      io_range = card->GetResourceRange(BAR_NO);
      context->io_base = io_range->BaseAddress;
      card->FreeResourceRange(io_range);
   }

   if(!success)
   {
      FreeExpansionCard(context, base);
      context = NULL;
   }

   return context;
}



/****i* atheros5000.device/FreeExpansionCard *******************************
*
*   NAME
*	FreeExpansionCard -- Release a card.
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



/****i* atheros5000.device/AddExpansionIntServer ***************************
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



/****i* atheros5000.device/RemExpansionIntServer ***************************
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



