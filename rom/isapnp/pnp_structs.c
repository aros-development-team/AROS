/* $Id$ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     Copyright (C) 2009-2013 The AROS Development Team
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#include "CompilerSpecific.h"

#include <exec/memory.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>

#include <resources/isapnp.h>
#include "isapnp_private.h"

#include "init.h"
#include "pnp_structs.h"

/******************************************************************************
** Allocate a card structure **************************************************
******************************************************************************/

// You should set isapnpc_Node.ln_Name. Allocate the string with AllocVec()!

AROS_LH0(struct ISAPNP_Card *, ISAPNP_AllocCard,
         struct ISAPNPBase *, res, 18, ISAPNP)
{
  AROS_LIBFUNC_INIT

  struct ISAPNP_Card* card;

  card = AllocVec( sizeof( *card ), MEMF_PUBLIC | MEMF_CLEAR );

  if( card != NULL )
  {
    card->isapnpc_Node.ln_Type = ISAPNP_NT_CARD;

    NewList( &card->isapnpc_Devices );
    InitSemaphore( &card->isapnpc_Lock );
  }

  return card;

  AROS_LIBFUNC_EXIT
}

/******************************************************************************
** Deallocate a card structure ************************************************
******************************************************************************/

AROS_LH1(void, ISAPNP_FreeCard,
         AROS_LHA(struct ISAPNP_Card *, card, A0),
         struct ISAPNPBase *, res, 19, ISAPNP)
{
  AROS_LIBFUNC_INIT

  struct ISAPNP_Device* dev;

  if( card == NULL )
  {
    return;
  }

//  KPrintF( "Nuking card %s%03lx%lx ('%s')\n",
//           card->isapnpc_ID.isapnpid_Vendor, card->isapnpc_ID.isapnpid_ProductID, card->isapnpc_ID.isapnpid_Revision,
//           card->isapnpc_Node.ln_Name != NULL ? card->isapnpc_Node.ln_Name : "" );

  while( ( dev = (struct ISAPNP_Device*) RemHead( &card->isapnpc_Devices ) ) )
  {
    ISAPNP_FreeDevice( dev, res );
  }

  if( card->isapnpc_Node.ln_Name != NULL )
  {
    FreeVec( card->isapnpc_Node.ln_Name );
  }

  FreeVec( card );

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Allocate a device structure ************************************************
******************************************************************************/

// You should set isapnpiod_Node.ln_Name. Allocate the string with AllocVec()!

AROS_LH0(struct ISAPNP_Device *, ISAPNP_AllocDevice,
         struct ISAPNPBase *, res, 20, ISAPNP)
{
  AROS_LIBFUNC_INIT

  struct ISAPNP_Device* dev;

  dev = AllocVec( sizeof( *dev ), MEMF_PUBLIC | MEMF_CLEAR );

  if( dev != NULL )
  {
    dev->isapnpd_Node.ln_Type = ISAPNP_NT_DEVICE;

    NewList( (struct List*) &dev->isapnpd_IDs );
    
    dev->isapnpd_Options = ISAPNP_AllocResourceGroup( ISAPNP_RG_PRI_GOOD, res );
    
    if( dev->isapnpd_Options == NULL )
    {
      ISAPNP_FreeDevice( dev, res );
      dev = NULL;
    }
    
    NewList( (struct List*) &dev->isapnpd_Resources );
    InitSemaphore( &dev->isapnpd_Lock );
  }

  return dev;

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Deallocate a device structure **********************************************
******************************************************************************/

AROS_LH1(void, ISAPNP_FreeDevice,
         AROS_LHA(struct ISAPNP_Device *, dev, A0),
         struct ISAPNPBase *, res, 21, ISAPNP)
{
  AROS_LIBFUNC_INIT

  struct ISAPNP_Identifier* id;
  struct ISAPNP_Resource*   r;

  if( dev == NULL )
  {
    return;
  }

//  KPrintF( "Nuking logical device '%s'\n",
//           dev->isapnpd_Node.ln_Name != NULL ? dev->isapnpd_Node.ln_Name : "" );


  while( ( id = (struct ISAPNP_Identifier*) 
             RemHead( (struct List*) &dev->isapnpd_IDs ) ) )
  {
//    KPrintF( "Nuking (compatible) device %s%03lx%lx\n",
//             id->isapnpid_Vendor, id->isapnpid_ProductID, id->isapnpid_Revision );

    FreeVec( id );
  }

  ISAPNP_FreeResourceGroup( dev->isapnpd_Options, res );

  while( ( r = (struct ISAPNP_Resource*) 
               RemHead( (struct List*) &dev->isapnpd_Resources ) ) )
  {
    ISAPNP_FreeResource( r, res );
  }


  if( dev->isapnpd_Node.ln_Name != NULL )
  {
    FreeVec( dev->isapnpd_Node.ln_Name );
  }

  FreeVec( dev );

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Allocate a resource group **************************************************
******************************************************************************/

AROS_LH1(struct ISAPNP_ResourceGroup *, ISAPNP_AllocResourceGroup,
         AROS_LHA(UBYTE, pri, D0),
         struct ISAPNPBase *, res, 22, ISAPNP)
{
  AROS_LIBFUNC_INIT

  struct ISAPNP_ResourceGroup* rg;

  rg = AllocVec( sizeof( *rg ), MEMF_PUBLIC | MEMF_CLEAR );

  if( rg != NULL )
  {
    rg->isapnprg_Type = ISAPNP_NT_RESOURCE_GROUP;
    rg->isapnprg_Pri  = pri;

    NewList( (struct List*) &rg->isapnprg_Resources );
    NewList( (struct List*) &rg->isapnprg_ResourceGroups );
  }

  return rg;

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Deallocate a resource group ************************************************
******************************************************************************/

AROS_LH1(void, ISAPNP_FreeResourceGroup,
         AROS_LHA(struct ISAPNP_ResourceGroup *, rg, A0),
         struct ISAPNPBase *, res, 23, ISAPNP)
{
  AROS_LIBFUNC_INIT

  struct ISAPNP_ResourceGroup* child_rg;
  struct ISAPNP_Resource*      r;

  if( rg == NULL )
  {
    return;
  }

//  KPrintF( "Nuking resource group.\n" );

  while( ( r = (struct ISAPNP_Resource*) 
               RemHead( (struct List*) &rg->isapnprg_Resources ) ) )
  {
    ISAPNP_FreeResource( r, res );
  }

  while( ( child_rg = (struct ISAPNP_ResourceGroup*) 
                      RemHead( (struct List*) &rg->isapnprg_ResourceGroups ) ) )
  {
    ISAPNP_FreeResourceGroup( child_rg, res );
  }

  FreeVec( rg );

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Allocate a resource ********************************************************
******************************************************************************/

AROS_LH1(struct ISAPNP_Resource *, ISAPNP_AllocResource,
         AROS_LHA(UBYTE, type, D0),
         struct ISAPNPBase *, res, 24, ISAPNP)
{
  AROS_LIBFUNC_INIT

  struct ISAPNP_Resource* r;
  ULONG                   size;

  switch( type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
      size = sizeof( struct ISAPNP_IRQResource );
      break;

    case ISAPNP_NT_DMA_RESOURCE:
      size = sizeof( struct ISAPNP_DMAResource );
      break;

    case ISAPNP_NT_IO_RESOURCE:
      size = sizeof( struct ISAPNP_IOResource );
      break;

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      return NULL;
  }

  r = AllocVec( size, MEMF_PUBLIC | MEMF_CLEAR );

  if( r != NULL )
  {
    r->isapnpr_Type = type;
  }

  return r;

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Deallocate a resource ******************************************************
******************************************************************************/

AROS_LH1(void, ISAPNP_FreeResource,
         AROS_LHA(struct ISAPNP_Resource *, r, A0),
         struct ISAPNPBase *, res, 25, ISAPNP)
{
  AROS_LIBFUNC_INIT

  if( r == NULL )
  {
    return;
  }

//  KPrintF( "Nuking resource %ld.\n", r->isapnpr_Type );

  FreeVec( r );

  AROS_LIBFUNC_EXIT
}
