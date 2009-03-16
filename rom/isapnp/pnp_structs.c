/* $Id: pnp_structs.c,v 1.5 2001/05/07 12:32:42 lcs Exp $ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     
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

struct ISAPNP_Card* ASMCALL
ISAPNP_AllocCard( REG( a6, struct ISAPNPBase* res ) )
{
  struct ISAPNP_Card* card;

  card = AllocVec( sizeof( *card ), MEMF_PUBLIC | MEMF_CLEAR );

  if( card != NULL )
  {
    card->isapnpc_Node.ln_Type = ISAPNP_NT_CARD;

    NewList( &card->isapnpc_Devices );
    InitSemaphore( &card->isapnpc_Lock );
  }

  return card;
}

/******************************************************************************
** Deallocate a card structure ************************************************
******************************************************************************/

void ASMCALL
ISAPNP_FreeCard( REG( a0, struct ISAPNP_Card* card ),
                 REG( a6, struct ISAPNPBase*  res ) )
{
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
}


/******************************************************************************
** Allocate a device structure ************************************************
******************************************************************************/

// You should set isapnpiod_Node.ln_Name. Allocate the string with AllocVec()!

struct ISAPNP_Device* ASMCALL
ISAPNP_AllocDevice( REG( a6, struct ISAPNPBase* res ) )
{
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
}


/******************************************************************************
** Deallocate a device structure **********************************************
******************************************************************************/

void ASMCALL
ISAPNP_FreeDevice( REG( a0, struct ISAPNP_Device* dev ),
                   REG( a6, struct ISAPNPBase*    res ) )
{
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
}


/******************************************************************************
** Allocate a resource group **************************************************
******************************************************************************/

struct ISAPNP_ResourceGroup* ASMCALL
ISAPNP_AllocResourceGroup( REG( d0, UBYTE              pri ),
                           REG( a6, struct ISAPNPBase* res ) )
{
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
}


/******************************************************************************
** Deallocate a resource group ************************************************
******************************************************************************/

void ASMCALL
ISAPNP_FreeResourceGroup( REG( a0, struct ISAPNP_ResourceGroup* rg ),
                          REG( a6, struct ISAPNPBase*           res ) )
{
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
}


/******************************************************************************
** Allocate a resource ********************************************************
******************************************************************************/

struct ISAPNP_Resource* ASMCALL
ISAPNP_AllocResource( REG( d0, UBYTE              type ),
                      REG( a6, struct ISAPNPBase* res ) )
{
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
}


/******************************************************************************
** Deallocate a resource ******************************************************
******************************************************************************/

void ASMCALL
ISAPNP_FreeResource( REG( a0, struct ISAPNP_Resource* r ),
                     REG( a6, struct ISAPNPBase*      res ) )
{
  if( r == NULL )
  {
    return;
  }

//  KPrintF( "Nuking resource %ld.\n", r->isapnpr_Type );

  FreeVec( r );
}
