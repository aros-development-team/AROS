/*

File: prometheus.c
Author: Neil Cafferkey
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
#include <libraries/prometheus.h>

#include <proto/exec.h>
#include <proto/prometheus.h>

#include "device.h"
#include "pci.h"

#include "pci_protos.h"
#include "prometheus_protos.h"


#define RETRY_COUNT 15

#define PLX9052_INTS 0x4c
#define PLX9052_CNTRL 0x50

#define PLX9052_CNTRLB_PCI21   14
#define PLX9052_CNTRLB_RETRIES 19

#define PLX9052_CNTRLF_PCI21   (1 << PLX9052_CNTRLB_PCI21)
#define PLX9052_CNTRLF_RETRIES (0xf << PLX9052_CNTRLB_RETRIES)


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
         BYTEOUT((UPINT)cor_reg, COR_VALUE);
         io_range_no = 2;
      }
      else if(context->bus_type == PLX_BUS)
      {
         /* Enable interrupts on the bridge */

         Prm_GetBoardAttrsTags(card, PRM_MemoryAddr1, (UPINT)&plx_base,
            TAG_END);
         LELONGOUT(plx_base + PLX9052_INTS,
            LELONGIN(plx_base + PLX9052_INTS) | (1 << 6));

#ifdef __mc68000
         /* Delay PCI retries as long as possible, so that they will
            hopefully never occur */

         pci_control = LELONGIN(plx_base + PLX9052_CNTRL);
         pci_control &= ~(PLX9052_CNTRLF_RETRIES | PLX9052_CNTRLF_PCI21);
         pci_control |= RETRY_COUNT << PLX9052_CNTRLB_RETRIES;
         LELONGOUT(plx_base + PLX9052_CNTRL, pci_control);
#endif

         /* Enable the PCCard */

         Prm_GetBoardAttrsTags(card, PRM_MemoryAddr2, (UPINT)&cor_reg,
            TAG_END);
         cor_reg += 0x3e0;
         *cor_reg = COR_VALUE;
         io_range_no = 3;
      }
      else
         io_range_no = 0;

      /* Get the I/O base of the wireless chip */

      Prm_GetBoardAttrsTags(card, PRM_MemoryAddr0 + io_range_no,
         (UPINT)&context->io_base, TAG_END);
      if(context->io_base == 0)
         success = FALSE;
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
   APTR owner;

   if(context != NULL)
   {
      card = context->card;
      if(card != NULL)
      {
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
*	context = AddPrometheusIntServer(index)
*
*	struct BusContext *AddPrometheusIntServer(ULONG);
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
*	RemPrometheusIntServer()
*
*	VOID RemPrometheusIntServer(ULONG);
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



