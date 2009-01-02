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
#include <pci/powerpci_pci.h>

#include <proto/exec.h>
#include <proto/powerpci.h>

#include "device.h"
#include "pci.h"

#include "pci_protos.h"
#include "powerpci_protos.h"


/****i* etherlink3.device/GetPowerPCICount *********************************
*
*   NAME
*	GetPowerPCICount
*
*   SYNOPSIS
*	count = GetPowerPCICount()
*
*	ULONG GetPowerPCICount();
*
****************************************************************************
*
*/

ULONG GetPowerPCICount(struct DevBase *base)
{
   ULONG count = 0;
   ULONG card = 0;
   UWORD vendor_id, product_id;

   while((card = pci_find_device(0xffff, 0xffff, card)) != NULL)
   {
      product_id = pci_read_conf_word(card, PCI_DEVICE_ID);
      vendor_id = pci_read_conf_word(card, PCI_VENDOR_ID);
      if(IsCardCompatible(vendor_id, product_id, base))
         count++;
   }

   return count;
}



/****i* etherlink3.device/AllocPowerPCICard ********************************
*
*   NAME
*	AllocPowerPCICard -- Get card from system.
*
*   SYNOPSIS
*	context = AllocPowerPCICard(index)
*
*	struct BusContext *AllocPowerPCICard(ULONG);
*
****************************************************************************
*
*/

struct BusContext *AllocPowerPCICard(ULONG index, struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   ULONG card = 0;
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
         product_id = pci_read_conf_word(card, PCI_DEVICE_ID);
         vendor_id = pci_read_conf_word(card, PCI_VENDOR_ID);
         if(IsCardCompatible(vendor_id, product_id, base))
            i++;
      }

      context->card = (APTR)card;
      if(card == NULL)
         success = FALSE;
   }

   /* Get base address and generation */

   if(success)
   {
      context->io_base = (UPINT)pci_get_base_start(card, BAR_NO);
      if(context->io_base == NULL)
         success = FALSE;
      if((context->io_base & 0xffff0000) == 0xee0000)
         context->io_base += 0x10000;
      context->generation = GetGeneration(product_id, base);
   }

   if(!success)
   {
      FreePowerPCICard(context, base);
      context = NULL;
   }

   return context;
}



/****i* etherlink3.device/FreePowerPCICard *********************************
*
*   NAME
*	FreePowerPCICard
*
*   SYNOPSIS
*	FreePowerPCICard(context)
*
*	VOID FreePowerPCICard(struct BusContext *);
*
****************************************************************************
*
*/

VOID FreePowerPCICard(struct BusContext *context, struct DevBase *base)
{
   if(context != NULL)
   {
      FreeMem(context, sizeof(struct BusContext));
   }

   return;
}



/****i* etherlink3.device/AddPowerPCIIntServer *****************************
*
*   NAME
*	AddPowerPCIIntServer
*
*   SYNOPSIS
*	context = AddPowerPCIIntServer(index)
*
*	struct BusContext *AddPowerPCIIntServer(ULONG);
*
****************************************************************************
*
*/

BOOL AddPowerPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   return pci_add_irq((ULONG)card, interrupt);
}



/****i* etherlink3.device/RemPowerPCIIntServer *****************************
*
*   NAME
*	RemPowerPCIIntServer
*
*   SYNOPSIS
*	RemPowerPCIIntServer()
*
*	VOID RemPowerPCIIntServer(ULONG);
*
****************************************************************************
*
*/

VOID RemPowerPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   pci_rem_irq((ULONG)card, interrupt);

   return;
}



