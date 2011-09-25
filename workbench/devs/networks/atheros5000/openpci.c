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
#include <libraries/openpci.h>

#include <proto/exec.h>
#include <proto/openpci.h>

#include "pci.h"

#include "pci_protos.h"
#include "openpci_protos.h"


/****i* atheros5000.device/GetOpenPCICount *********************************
*
*   NAME
*	GetOpenPCICount
*
*   SYNOPSIS
*	count = GetOpenPCICount()
*
*	ULONG GetOpenPCICount();
*
****************************************************************************
*
*/

ULONG GetOpenPCICount(struct DevBase *base)
{
   ULONG count = 0;
   struct pci_dev *card = NULL;
   UWORD vendor_id, product_id;

   while((card = pci_find_device(0xffff, 0xffff, card)) != NULL)
   {
      product_id = pci_read_config_word(PCI_DEVICE_ID, card);
      vendor_id = pci_read_config_word(PCI_VENDOR_ID, card);
      if(IsCardCompatible(vendor_id, product_id, base))
         count++;
   }

   return count;
}



/****i* atheros5000.device/AllocOpenPCICard ********************************
*
*   NAME
*	AllocOpenPCICard -- Take control of a card.
*
*   SYNOPSIS
*	context = AllocOpenPCICard(index)
*
*	struct BusContext *AllocOpenPCICard(ULONG);
*
****************************************************************************
*
*/

struct BusContext *AllocOpenPCICard(ULONG index, struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   struct pci_dev *card = 0;
   UWORD i = 0, vendor_id, product_id;

   /* Find a compatible card */

   context = AllocMem(sizeof(struct BusContext), MEMF_PUBLIC | MEMF_CLEAR);
   if(context == NULL)
      success = FALSE;

   if(success)
   {
      while(i <= index)
      {
         card = pci_find_device(0xffff, 0xffff, card);
         product_id = pci_read_config_word(PCI_DEVICE_ID, card);
         vendor_id = pci_read_config_word(PCI_VENDOR_ID, card);
         if(IsCardCompatible(vendor_id, product_id, base))
            i++;
      }

      context->card = card;
      context->id = product_id;
   }

   /* Get base address */

   if(success)
   {
      context->io_base = (UPINT)card->base_address[BAR_NO];
      if(context->io_base == NULL)
         success = FALSE;

      /* Only enable card once, otherwise it may stop working */

      if((pci_read_config_word(PCI_COMMAND, card)
         & (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER)) == 0)
         pci_write_config_word(PCI_COMMAND,
            PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER, card);
   }

   if(!success)
   {
      FreeOpenPCICard(context, base);
      context = NULL;
   }

   return context;
}



/****i* atheros5000.device/FreeOpenPCICard *********************************
*
*   NAME
*	FreeOpenPCICard -- Release a card.
*
*   SYNOPSIS
*	FreeOpenPCICard(context)
*
*	VOID FreeOpenPCICard(struct BusContext *);
*
****************************************************************************
*
*/

VOID FreeOpenPCICard(struct BusContext *context, struct DevBase *base)
{
   if(context != NULL)
      FreeMem(context, sizeof(struct BusContext));

   return;
}



/****i* atheros5000.device/AddOpenPCIIntServer *****************************
*
*   NAME
*	AddOpenPCIIntServer
*
*   SYNOPSIS
*	success = AddOpenPCIIntServer(card, interrupt)
*
*	BOOL AddOpenPCIIntServer(APTR, struct Interrupt *);
*
****************************************************************************
*
*/

BOOL AddOpenPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   return pci_add_intserver(interrupt, card);
}



/****i* atheros5000.device/RemOpenPCIIntServer *****************************
*
*   NAME
*	RemOpenPCIIntServer
*
*   SYNOPSIS
*	RemOpenPCIIntServer(card, interrupt)
*
*	VOID RemOpenPCIIntServer(APTR, struct Interrupt *);
*
****************************************************************************
*
*/

VOID RemOpenPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   pci_rem_intserver(interrupt, card);

   return;
}


