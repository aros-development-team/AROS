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
#include <libraries/prometheus.h>

#include <proto/exec.h>
#include <proto/prometheus.h>

#include "pci.h"

#include "pci_protos.h"
#include "prometheus_protos.h"


/****i* atheros5000.device/GetPrometheusCount ******************************
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



/****i* atheros5000.device/AllocPrometheusCard *****************************
*
*   NAME
*	AllocPrometheusCard -- Take control of a card.
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
   UPINT vendor_id, product_id;

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
      context->id = product_id;
      if(card == NULL)
         success = FALSE;
   }

   if(success)
   {
      /* Get I/O base address */

      Prm_GetBoardAttrsTags(card, PRM_MemoryAddr0 + BAR_NO,
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



/****i* atheros5000.device/FreePrometheusCard ******************************
*
*   NAME
*	FreePrometheusCard -- Release a card.
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



/****i* atheros5000.device/AddPrometheusIntServer **************************
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



/****i* atheros5000.device/RemPrometheusIntServer **************************
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



/****i* atheros5000.device/AllocPrometheusDMAMem ***************************
*
*   NAME
*	AllocPrometheusDMAMem
*
*   SYNOPSIS
*	mem = AllocPrometheusDMAMem(context, size, alignment)
*
*	APTR AllocPrometheusDMAMem(struct BusContext *, UPINT, UWORD);
*
****************************************************************************
*
* Alignment currently must be minimum of 8 bytes.
*
*/

APTR AllocPrometheusDMAMem(UPINT size, struct DevBase *base)
{
   return Prm_AllocDMABuffer(size);
}



/****i* atheros5000.device/FreePrometheusDMAMem ****************************
*
*   NAME
*	FreePrometheusDMAMem
*
*   SYNOPSIS
*	FreePrometheusDMAMem(context, mem)
*
*	VOID FreePrometheusDMAMem(struct BusContext *, APTR);
*
****************************************************************************
*
*/

VOID FreePrometheusDMAMem(APTR mem, UPINT size, struct DevBase *base)
{
   Prm_FreeDMABuffer(mem, size);

   return;
}



