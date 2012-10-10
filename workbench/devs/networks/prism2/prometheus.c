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
#include <libraries/prometheus.h>

#include <proto/exec.h>
#include <proto/prometheus.h>

#include "pci.h"
#include "plx9052.h"
#include "prism2.h"

#include "pci_protos.h"
#include "prometheus_protos.h"
#include "timer_protos.h"


/****i* prism2.device/GetPrometheusCount ***********************************
*
*   NAME
*	GetPrometheusCount
*
*   SYNOPSIS
*	count = GetPrometheusCount()
*
*	ULONG GetPrometheusCount();
*
****************************************************************************
*
*/

ULONG GetPrometheusCount(struct DevBase *base)
{
   ULONG count = 0;
   PCIBoard *card = NULL;
   UPINT vendor_id, product_id;

   while((card = Prm_FindBoardTagList(card, NULL)) != NULL)
   {
      Prm_GetBoardAttrsTags(card, PRM_Vendor, (UPINT)&vendor_id,
         PRM_Device, (UPINT)&product_id, TAG_END);
      if(IsCardCompatible(vendor_id, product_id, base))
         count++;
   }

   return count;
}



/****i* prism2.device/AllocPrometheusCard **********************************
*
*   NAME
*	AllocPrometheusCard -- Create a unit.
*
*   SYNOPSIS
*	context = AllocPrometheusCard(index)
*
*	struct BusContext *AllocPrometheusCard(ULONG);
*
****************************************************************************
*
*/

struct BusContext *AllocPrometheusCard(ULONG index, struct DevBase *base)
{
   BOOL success = TRUE;
   struct BusContext *context;
   PCIBoard *card = NULL;
   UWORD i = 0;
   UPINT vendor_id, product_id, plx_base, int_reg;
   UBYTE io_range_no;
   ULONG value;
   volatile UBYTE *cor_reg;

   /* Find a compatible card */

   context = AllocMem(sizeof(struct BusContext), MEMF_PUBLIC | MEMF_CLEAR);
   if(context == NULL)
      success = FALSE;

   if(success)
   {
      while(i <= index)
      {
         card = Prm_FindBoardTagList(card, NULL);
         Prm_GetBoardAttrsTags(card, PRM_Vendor, (UPINT)&vendor_id,
            PRM_Device, (UPINT)&product_id, TAG_END);
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

         Prm_GetBoardAttrsTags(card, PRM_MemoryAddr1, (UPINT)&cor_reg,
            TAG_END);
         BYTEOUT((UPINT)cor_reg, COR_RESET);
         BusyMilliDelay(RESET_DELAY, base);
         BYTEOUT((UPINT)cor_reg, COR_ENABLE);
         BusyMilliDelay(RESET_DELAY, base);
         io_range_no = 2;
      }
      else if(context->bus_type == PLX_BUS)
      {
         /* Reset and enable the PCCard */

         Prm_GetBoardAttrsTags(card, PRM_MemoryAddr2, (UPINT)&cor_reg,
            TAG_END);
         cor_reg += 0x3e0;
         *cor_reg = COR_ENABLE;
         BusyMilliDelay(RESET_DELAY, base);

         /* Enable interrupts on the bridge */

         Prm_GetBoardAttrsTags(card, PRM_MemoryAddr1, (UPINT)&plx_base,
            TAG_END);
         int_reg = plx_base + PLX9052_INTS;
         value = LELONGIN(int_reg);
         LELONGOUT(int_reg, value | (1 << 6));
         if((LELONGIN(int_reg) & (1 << 6)) == 0)
            success = FALSE;
         io_range_no = 3;
      }
      else
         io_range_no = 0;

      /* Get the I/O base of the wireless chip */

      Prm_GetBoardAttrsTags(card, PRM_MemoryAddr0 + io_range_no,
         (UPINT)&context->io_base, TAG_END);
      if(context->io_base == 0)
         success = FALSE;

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

   /* Lock card */

   if(success)
   {
      if(!Prm_SetBoardAttrsTags(card, PRM_BoardOwner, (UPINT)base, TAG_END))
         success = FALSE;
   }

   if(!success)
   {
      FreePrometheusCard(context, base);
      context = NULL;
   }

   return context;
}



/****i* prism2.device/FreePrometheusCard ***********************************
*
*   NAME
*	FreePrometheusCard
*
*   SYNOPSIS
*	FreePrometheusCard(context)
*
*	VOID FreePrometheusCard(struct BusContext *);
*
****************************************************************************
*
*/

VOID FreePrometheusCard(struct BusContext *context, struct DevBase *base)
{
   PCIBoard *card;
   ULONG value;
   UPINT plx_base, int_reg;
   volatile UBYTE *cor_reg;
   APTR owner;

   if(context != NULL)
   {
      card = context->card;
      if(card != NULL)
      {
         if(context->bus_type == TMD_BUS)
         {
            /* Disable the PCCard */

            Prm_GetBoardAttrsTags(card, PRM_MemoryAddr1, (UPINT)&cor_reg,
               TAG_END);
            BYTEOUT((UPINT)cor_reg, 0);
         }
         else if(context->bus_type == PLX_BUS)
         {
            /* Disable interrupts on the bridge */

            Prm_GetBoardAttrsTags(card, PRM_MemoryAddr1, (UPINT)&plx_base,
               TAG_END);
            int_reg = plx_base + PLX9052_INTS;
            value = LELONGIN(int_reg);
            LELONGOUT(int_reg, value & ~(1 << 6));

            /* Disable the PCCard */

            Prm_GetBoardAttrsTags(card, PRM_MemoryAddr2, (UPINT)&cor_reg,
               TAG_END);
            cor_reg += 0x3e0;
            *cor_reg = COR_RESET;
            BusyMilliDelay(250, base);
            *cor_reg = 0;
         }

         /* Unlock board */

         Prm_GetBoardAttrsTags(card, PRM_BoardOwner, (UPINT)&owner,
            TAG_END);
         if(owner == base)
            Prm_SetBoardAttrsTags(card, PRM_BoardOwner, NULL, TAG_END);
      }

      FreeMem(context, sizeof(struct BusContext));
   }

   return;
}



/****i* prism2.device/AddPrometheusIntServer *******************************
*
*   NAME
*	AddPrometheusIntServer
*
*   SYNOPSIS
*	success = AddPrometheusIntServer(card, interrupt)
*
*	BOOL AddPrometheusIntServer(APTR, struct Interrupt *);
*
****************************************************************************
*
*/

BOOL AddPrometheusIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   return Prm_AddIntServer(card, interrupt);
}



/****i* prism2.device/RemPrometheusIntServer *******************************
*
*   NAME
*	RemPrometheusIntServer
*
*   SYNOPSIS
*	RemPrometheusIntServer(card, interrupt)
*
*	VOID RemPrometheusIntServer(APTR, struct Interrupt *);
*
****************************************************************************
*
*/

VOID RemPrometheusIntServer(APTR card, struct Interrupt *interrupt,
   struct DevBase *base)
{
   Prm_RemIntServer(card, interrupt);

   return;
}



