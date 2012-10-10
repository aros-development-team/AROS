/*

Copyright (C) 2004-2010 Neil Cafferkey

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

#include "pci.h"
#include "plx9052.h"

#include "pci_protos.h"
#include "powerpci_protos.h"


/****i* prism2.device/GetPowerPCICount *************************************
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
      vendor_id = pci_read_conf_word(card, PCI_VENDOR_ID);
      product_id = pci_read_conf_word(card, PCI_DEVICE_ID);
      if(IsCardCompatible(vendor_id, product_id, base))
         count++;
   }

   return count;
}



/****i* prism2.device/AllocPowerPCICard ************************************
*
*   NAME
*	AllocPowerPCICard
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
   UBYTE io_range_no;
   volatile UBYTE *cor_reg;
   UPINT int_reg;

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

   if(success)
   {
      /* Find out what type of Prism II PCI card this is */

      context->bus_type = GetBusType(product_id, base);

      if(context->bus_type == TMD_BUS)
      {
         /* Enable the PCCard */

         cor_reg = pci_get_base_start(card, 1);
         BYTEOUT((UPINT)cor_reg, COR_ENABLE);
         io_range_no = 2;
      }
      else if(context->bus_type == PLX_BUS)
      {
         /* Enable the PCCard */

         cor_reg = pci_get_base_start(card, 2) + 0x3e0;
         *cor_reg = COR_ENABLE;

         /* Enable interrupts on the bridge */

         int_reg = (UPINT)pci_get_base_start(card, 1) + PLX9052_INTS;
         LELONGOUT(int_reg, LELONGIN(int_reg) | (1 << 6));
         io_range_no = 3;
      }
      else
         io_range_no = 0;

      /* Get the I/O base of the wireless chip */

      context->io_base = (UPINT)pci_get_base_start(card, io_range_no);
      if(context->io_base == NULL)
         success = FALSE;
   }

   if(!success)
   {
      FreePowerPCICard(context, base);
      context = NULL;
   }

   return context;
}



/****i* prism2.device/FreePowerPCICard *************************************
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
      FreeMem(context, sizeof(struct BusContext));

   return;
}



/****i* prism2.device/AddPowerPCIIntServer *********************************
*
*   NAME
*	AddPowerPCIIntServer
*
*   SYNOPSIS
*	success = AddPowerPCIIntServer(card, interrupt)
*
*	BOOL AddPowerPCIIntServer(APTR, struct Interrupt *);
*
****************************************************************************
*
*/

BOOL AddPowerPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   return pci_add_irq((ULONG)card, interrupt);
}



/****i* prism2.device/RemPowerPCIIntServer *********************************
*
*   NAME
*	RemPowerPCIIntServer
*
*   SYNOPSIS
*	RemPowerPCIIntServer(card, interrupt)
*
*	VOID RemPowerPCIIntServer(APTR, struct Interrupt *);
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



