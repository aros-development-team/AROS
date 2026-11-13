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
#include <libraries/cybpci.h>

#include <proto/exec.h>
#include <proto/cybpci.h>

#include "pci.h"
#include "plx9052.h"

#include "pci_protos.h"
#include "cybpci_protos.h"


/****i* prism2.device/GetCybPCICount ***************************************
*
*   NAME
*	GetCybPCICount
*
*   SYNOPSIS
*	count = GetCybPCICount()
*
*	ULONG GetCybPCICount();
*
****************************************************************************
*
*/

ULONG GetCybPCICount(struct DevBase *base)
{
   ULONG count = 0;
   APTR card = NULL;
   UWORD vendor_id, product_id;

   while((card = PCIFindBoardTagList(card, NULL)) != NULL)
   {
      vendor_id = PCIReadConfigWord(card, CYBPCICONFIG_VENDOR);
      product_id = PCIReadConfigWord(card, CYBPCICONFIG_DEVICE);
      if(IsCardCompatible(vendor_id, product_id, base))
         count++;
   }

   return count;
}



/****i* prism2.device/AllocCybPCICard **************************************
*
*   NAME
*	AllocCybPCICard -- Create a unit.
*
*   SYNOPSIS
*	context = AllocCybPCICard(index)
*
*	struct BusContext *AllocCybPCICard(ULONG);
*
****************************************************************************
*
*/

struct BusContext *AllocCybPCICard(ULONG index, struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   APTR card = NULL;
   UWORD i = 0;
   UPINT vendor_id, product_id, plx_base;
   UBYTE io_range_no;
   volatile UBYTE *cor_reg;
   ULONG pci_control;

   /* Find a compatible card */

   context = AllocMem(sizeof(struct BusContext), MEMF_PUBLIC | MEMF_CLEAR);
   if(context == NULL)
      success = FALSE;

   if(success)
   {
      while(i <= index)
      {
         card = PCIFindBoardTagList(card, NULL);
         vendor_id = PCIReadConfigWord(card, CYBPCICONFIG_VENDOR);
         product_id = PCIReadConfigWord(card, CYBPCICONFIG_DEVICE);
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

      if(context->bus_type == TMD_BUS)
      {
         /* Enable the PCCard */

         cor_reg = (APTR)PCIGetBoardAttr(card, CYBPCITAG_BASEADDRESS1);
         BYTEOUT((UPINT)cor_reg, COR_ENABLE);
         io_range_no = 2;
      }
      else if(context->bus_type == PLX_BUS)
      {
         /* Enable interrupts on the bridge */

         plx_base = PCIGetBoardAttr(card, CYBPCITAG_BASEADDRESS1);
         LELONGOUT(plx_base + PLX9052_INTS,
            LELONGIN(plx_base + PLX9052_INTS) | (1 << 6));

         /* Enable the PCCard */

         cor_reg = (APTR)PCIGetBoardAttr(card, CYBPCITAG_BASEADDRESS2);
         cor_reg += 0x3e0;
         *cor_reg = COR_ENABLE;
         io_range_no = 3;
      }
      else
         io_range_no = 0;

      /* Get the I/O base of the wireless chip */

      context->io_base = PCIGetBoardAttr(card,
         CYBPCITAG_BASEADDRESS0 + io_range_no);
      if(context->io_base == 0)
         success = FALSE;
   }

   if(!success)
   {
      FreeCybPCICard(context, base);
      context = NULL;
   }

   return context;
}



/****i* prism2.device/FreeCybPCICard ***************************************
*
*   NAME
*	FreeCybPCICard
*
*   SYNOPSIS
*	FreeCybPCICard(context)
*
*	VOID FreeCybPCICard(struct BusContext *);
*
****************************************************************************
*
*/

VOID FreeCybPCICard(struct BusContext *context, struct DevBase *base)
{
   APTR card;
   APTR owner;

   if(context != NULL)
      FreeMem(context, sizeof(struct BusContext));

   return;
}



/****i* prism2.device/AddCybPCIIntServer ***********************************
*
*   NAME
*	AddCybPCIIntServer
*
*   SYNOPSIS
*	success = AddCybPCIIntServer(card, interrupt)
*
*	BOOL AddCybPCIIntServer(APTR, struct Interrupt *);
*
****************************************************************************
*
*/

BOOL AddCybPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   return (interrupt->is_Data = PCICreateIntObjectTagList(card,
      (APTR)interrupt->is_Code, interrupt->is_Data, NULL)) != NULL;
}



/****i* prism2.device/RemCybPCIIntServer ***********************************
*
*   NAME
*	RemCybPCIIntServer
*
*   SYNOPSIS
*	RemCybPCIIntServer(card, interrupt)
*
*	VOID RemCybPCIIntServer(APTR, struct Interrupt *);
*
****************************************************************************
*
*/

VOID RemCybPCIIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   if(interrupt->is_Data != NULL)
      PCIDeleteIntObject(interrupt->is_Data);

   return;
}



