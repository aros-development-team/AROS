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
#include <libraries/openpci.h>

#include <proto/exec.h>
#include <proto/openpci.h>

#include "pci.h"
#include "plx9052.h"
#include "prism2.h"

#include "pci_protos.h"
#include "openpci_protos.h"
#include "timer_protos.h"


/****i* prism2.device/GetOpenPCICount **************************************
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



/****i* prism2.device/AllocOpenPCICard *************************************
*
*   NAME
*	AllocOpenPCICard -- Create a unit.
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
   struct pci_dev *card = NULL;
   UWORD i = 0, vendor_id, product_id, io_range_no;
   ULONG value;
   UPINT io_reg, int_reg;
   volatile UBYTE *cor_reg;

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
      if(card == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Find out what type of Prism II PCI card this is */

      context->bus_type = GetBusType(product_id, base);
      pci_write_config_word(PCI_COMMAND,
         PCI_COMMAND_MEMORY | PCI_COMMAND_IO, card);

      if(context->bus_type == TMD_BUS)
      {
         /* Reset and enable the PCCard */

         io_reg = (UPINT)card->base_address[1];
         BYTEOUT(io_reg, COR_RESET);
         BusyMilliDelay(RESET_DELAY, base);
         BYTEOUT(io_reg, COR_ENABLE);
         BusyMilliDelay(RESET_DELAY, base);
         io_range_no = 2;
      }
      else if(context->bus_type == PLX_BUS)
      {
         /* Reset and enable the PCCard */

         cor_reg = (volatile UBYTE *)card->base_address[2] + 0x3e0;
         *cor_reg = COR_ENABLE;
         BusyMilliDelay(RESET_DELAY, base);

         /* Enable interrupts on the bridge */

         int_reg = (UPINT)card->base_address[1] + PLX9052_INTS;
         value = LONGIN(int_reg);
         LONGOUT(int_reg, value | (1 << 6));
         if((LONGIN(int_reg) & (1 << 6)) == 0)
            success = FALSE;
         io_range_no = 3;
      }
      else
         io_range_no = 0;

      /* Get the I/O base of the wireless chip */

      context->have_card = TRUE;
      context->io_base = (UPINT)card->base_address[io_range_no];

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
      FreeOpenPCICard(context, base);
      context = NULL;
   }

   return context;
}



/****i* prism2.device/FreeOpenPCICard **************************************
*
*   NAME
*	FreeOpenPCICard
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
   struct pci_dev *card;

   ULONG value;
   UPINT io_reg, int_reg;
   volatile UBYTE *cor_reg;

   if(context != NULL)
   {
      card = context->card;
      if(card != NULL)
      {
         if(context->bus_type == TMD_BUS)
         {
            /* Disable the PCCard */

            io_reg = (UPINT)card->base_address[1];
            BYTEOUT(io_reg, 0);
         }
         else if(context->bus_type == PLX_BUS)
         {
            /* Disable interrupts on the bridge */

            int_reg = (UPINT)card->base_address[1] + PLX9052_INTS;
            value = LONGIN(int_reg);
            LONGOUT(int_reg, value & ~(1 << 6));

            /* Disable the PCCard */

            cor_reg = (volatile UBYTE *)card->base_address[2] + 0x3e0;
            *cor_reg = COR_RESET;
            BusyMilliDelay(250, base);
            *cor_reg = 0;
         }

         FreeMem(context, sizeof(struct BusContext));
      }
   }

   return;
}



/****i* prism2.device/AddOpenPCIIntServer **********************************
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



/****i* prism2.device/RemOpenPCIIntServer **********************************
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



