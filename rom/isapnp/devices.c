/* $Id: devices.c,v 1.1 2001/05/10 14:07:50 lcs Exp $ */

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

#include <proto/exec.h>

#include <stdlib.h>

#include <resources/isapnp.h>
#include "isapnp_private.h"


/******************************************************************************
** Find a PNP ISA card  *******************************************************
******************************************************************************/

struct ISAPNP_Card* ASMCALL
ISAPNP_FindCard( REG( a0, struct ISAPNP_Card* last_card ), 
                 REG( d0, LONG                manufacturer ),
                 REG( d1, WORD                product ),
                 REG( d2, BYTE                revision ),
                 REG( d3, LONG                serial ),
                 REG( a6, struct ISAPNPBase*  res ) )
{
  struct ISAPNP_Card* card;

  if( last_card == NULL )
  {
    card = (struct ISAPNP_Card*) res->m_Cards.lh_Head;
  }
  else
  {
    card = (struct ISAPNP_Card*) last_card->isapnpc_Node.ln_Succ;
  }

  while( card->isapnpc_Node.ln_Succ != NULL )
  {
    if( manufacturer == -1 || 
        ISAPNP_MAKE_ID( card->isapnpc_ID.isapnpid_Vendor[ 0 ],
                        card->isapnpc_ID.isapnpid_Vendor[ 1 ],
                        card->isapnpc_ID.isapnpid_Vendor[ 2 ] ) == manufacturer )
    {
      if( product == -1 || card->isapnpc_ID.isapnpid_ProductID == product )
      {
        if( revision == -1 || card->isapnpc_ID.isapnpid_Revision == revision )
        {
          if( serial == -1 || (LONG) card->isapnpc_SerialNumber == serial )
          {
            return card;
          }
        }
      }
    }

    card = (struct ISAPNP_Card*) card->isapnpc_Node.ln_Succ;
  }

  return NULL;
}


/******************************************************************************
** Find a PNP ISA device  *****************************************************
******************************************************************************/

struct ISAPNP_Device* ASMCALL
ISAPNP_FindDevice( REG( a0, struct ISAPNP_Device* last_device ), 
                   REG( d0, LONG                  manufacturer ),
                   REG( d1, WORD                  product ),
                   REG( d2, BYTE                  revision ),
                   REG( a6, struct ISAPNPBase*    res ) )
{
  struct ISAPNP_Card*   card;
  struct ISAPNP_Device* dev;

  if( last_device == NULL )
  {
    card = (struct ISAPNP_Card*) res->m_Cards.lh_Head;
    dev  = (struct ISAPNP_Device*) card->isapnpc_Devices.lh_Head;
  }
  else
  {
    card = (struct ISAPNP_Card*) last_device->isapnpd_Card;
    dev  = (struct ISAPNP_Device*) last_device->isapnpd_Node.ln_Succ;
  }

  while( card->isapnpc_Node.ln_Succ != NULL )
  {
    while( dev->isapnpd_Node.ln_Succ != NULL )
    {
      struct ISAPNP_Identifier* id;

      for( id = (struct ISAPNP_Identifier*) dev->isapnpd_IDs.mlh_Head;
           id->isapnpid_MinNode.mln_Succ != NULL;
           id = (struct ISAPNP_Identifier*) id->isapnpid_MinNode.mln_Succ )
      {
        if( manufacturer == -1 || 
            ISAPNP_MAKE_ID( id->isapnpid_Vendor[ 0 ],
                            id->isapnpid_Vendor[ 1 ],
                            id->isapnpid_Vendor[ 2 ] ) == manufacturer )
        {
          if( product == -1 || id->isapnpid_ProductID == product )
          {
            if( revision == -1 || id->isapnpid_Revision == revision )
            {
              return dev;
            }
          }
        }
      }

      dev = (struct ISAPNP_Device*) dev->isapnpd_Node.ln_Succ;
    }

    card = (struct ISAPNP_Card*) card->isapnpc_Node.ln_Succ;
    dev  = (struct ISAPNP_Device*) card->isapnpc_Devices.lh_Head;
  }

  return NULL;
}



/******************************************************************************
** Helper functions for the card/device locking functions *********************
******************************************************************************/

static int 
ComparePtr( const void* a, 
            const void* b )
{
  struct ISAPNP_Card* p1 = *( (struct ISAPNP_Card**) a );
  struct ISAPNP_Card* p2 = *( (struct ISAPNP_Card**) b );
  
  return p1 - p2;
}


static void**
MakeSortedArray( void** ptrs )
{
  int    size;
  void** ptrs_ptr;
  void** result;

  // Find number of pointers to sort

  size      = 0;
  ptrs_ptr = ptrs;

  while( *ptrs_ptr != NULL )
  {
    ++size;
    ++ptrs_ptr;
  }
  
  result = AllocVec( sizeof( *ptrs_ptr ) * size + 1, MEMF_PUBLIC );
  
  if( result != NULL )
  {
    ptrs_ptr = result;
    
    while( *ptrs != NULL )
    {
      *ptrs_ptr = *ptrs;
      ++ptrs;
      ++ptrs_ptr;
    }
    
    qsort( result, size, sizeof( *result ), ComparePtr );
    
    result[ size ] = NULL;
  }
  
  return result;
}


/******************************************************************************
** Lock one or more PNP ISA cards  ********************************************
******************************************************************************/

APTR ASMCALL
ISAPNP_LockCardsA( REG( d0, ULONG                flags ),
                   REG( a0, struct ISAPNP_Card** cards ),
                   REG( a6, struct ISAPNPBase*   res ) )
{
  struct ISAPNP_Card** cards_ptr;
  struct ISAPNP_Card** result;

  result = (struct ISAPNP_Card**) MakeSortedArray( (void**) cards );
  
  if( result != NULL )
  {
    for( cards_ptr = result;
         *cards_ptr != NULL;
         ++cards_ptr )
    {
      if( flags & ISAPNP_LOCKF_NONBLOCKING )
      {
        if( ! AttemptSemaphore( &(*cards_ptr)->isapnpc_Lock ) )
        {
          struct ISAPNP_Card** cards_end;

          // Oops! Failed to lock one of the cards!
          
          cards_end = cards_ptr;

          for( cards_ptr = result;
               cards_ptr != cards_end;
               ++cards_ptr );
          {
            ReleaseSemaphore( &(*cards_ptr)->isapnpc_Lock );
          }
          
          FreeVec( result );
          result = NULL;
          break;
        }
      }
      else
      {
        ObtainSemaphore( &(*cards_ptr)->isapnpc_Lock );
      }
    }
  }
  
  return (APTR) result;
}


/******************************************************************************
** Unlock one or more PNP ISA cards  ******************************************
******************************************************************************/

void ASMCALL
ISAPNP_UnlockCards( REG( a0, APTR               card_lock_handle ),
                    REG( a6, struct ISAPNPBase* res ) )
{
  struct ISAPNP_Card** cards_ptr;

  if( card_lock_handle == NULL )
  {
    return;
  }

  for( cards_ptr = (struct ISAPNP_Card**) card_lock_handle;
       *cards_ptr != NULL;
       ++cards_ptr )
  {
    ReleaseSemaphore( &(*cards_ptr)->isapnpc_Lock );
  }

  FreeVec( card_lock_handle );
}


/******************************************************************************
** Lock one or more PNP ISA devices *******************************************
******************************************************************************/

APTR ASMCALL
ISAPNP_LockDevicesA( REG( d0, ULONG                  flags ),
                     REG( a0, struct ISAPNP_Device** devices ),
                     REG( a6, struct ISAPNPBase*     res ) )
{
  struct ISAPNP_Device** devices_ptr;
  struct ISAPNP_Device** result;

  result = (struct ISAPNP_Device**) MakeSortedArray( (void**) devices );
  
  if( result != NULL )
  {
    for( devices_ptr = result;
         *devices_ptr != NULL;
         ++devices_ptr )
    {
      if( flags & ISAPNP_LOCKF_NONBLOCKING )
      {
        if( ! AttemptSemaphore( &(*devices_ptr)->isapnpd_Lock ) )
        {
          struct ISAPNP_Device** devices_end;

          // Oops! Failed to lock one of the devices!
          
          devices_end = devices_ptr;

          for( devices_ptr = result;
               devices_ptr != devices_end;
               ++devices_ptr );
          {
            ReleaseSemaphore( &(*devices_ptr)->isapnpd_Lock );
          }
          
          FreeVec( result );
          result = NULL;
          break;
        }
      }
      else
      {
        ObtainSemaphore( &(*devices_ptr)->isapnpd_Lock );
      }
    }
  }
  
  return (APTR) result;
}


/******************************************************************************
** Unlock one or more PNP ISA devices  ****************************************
******************************************************************************/

void ASMCALL
ISAPNP_UnlockDevices( REG( a0, APTR               device_lock_handle ),
                      REG( a6, struct ISAPNPBase* res ) )
{
  struct ISAPNP_Device** devices_ptr;

  if( device_lock_handle == NULL )
  {
    return;
  }

  for( devices_ptr = (struct ISAPNP_Device**) device_lock_handle;
       *devices_ptr != NULL;
       ++devices_ptr )
  {
    ReleaseSemaphore( &(*devices_ptr)->isapnpd_Lock );
  }
  
  FreeVec( device_lock_handle );
}
