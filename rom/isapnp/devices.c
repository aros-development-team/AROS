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

#include <proto/exec.h>

#include <stdlib.h>

#include <resources/isapnp.h>
#include "isapnp_private.h"


/******************************************************************************
** Find a PNP ISA card  *******************************************************
******************************************************************************/

AROS_LH5(struct ISAPNP_Card *, ISAPNP_FindCard,
         AROS_LHA(struct ISAPNP_Card *, last_card, A0), 
         AROS_LHA(LONG, manufacturer, D0),
         AROS_LHA(WORD, product, D1),
         AROS_LHA(BYTE, revision, D2),
         AROS_LHA(LONG, serial, D3),
         struct ISAPNPBase *, res, 28, ISAPNP)
{
  AROS_LIBFUNC_INIT

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

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Find a PNP ISA device  *****************************************************
******************************************************************************/

AROS_LH4(struct ISAPNP_Device *, ISAPNP_FindDevice,
         AROS_LHA(struct ISAPNP_Device *, last_device, A0), 
         AROS_LHA(LONG, manufacturer, D0),
         AROS_LHA(WORD, product, D1),
         AROS_LHA(BYTE, revision, D2),
         struct ISAPNPBase *, res, 29, ISAPNP)
{
  AROS_LIBFUNC_INIT

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

  AROS_LIBFUNC_EXIT
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
MakeSortedArray( void** ptrs, struct ISAPNPBase *res)
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

AROS_LH2(APTR, ISAPNP_LockCardsA,
         AROS_LHA(ULONG, flags, D0),
         AROS_LHA(struct ISAPNP_Card **, cards, A0),
         struct ISAPNPBase *, res, 30, ISAPNP)
{
  AROS_LIBFUNC_INIT

  struct ISAPNP_Card** cards_ptr;
  struct ISAPNP_Card** result;

  result = (struct ISAPNP_Card**) MakeSortedArray( (void**) cards, res );
  
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

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Unlock one or more PNP ISA cards  ******************************************
******************************************************************************/

AROS_LH1(void, ISAPNP_UnlockCards,
         AROS_LHA(APTR, card_lock_handle, A0),
         struct ISAPNPBase *, res, 31, ISAPNP)
{
  AROS_LIBFUNC_INIT

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

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Lock one or more PNP ISA devices *******************************************
******************************************************************************/

AROS_LH2(APTR, ISAPNP_LockDevicesA,
         AROS_LHA(ULONG, flags, D0),
         AROS_LHA(struct ISAPNP_Device **, devices, A0),
         struct ISAPNPBase *, res, 32, ISAPNP)
{
  AROS_LIBFUNC_INIT

  struct ISAPNP_Device** devices_ptr;
  struct ISAPNP_Device** result;

  result = (struct ISAPNP_Device**) MakeSortedArray( (void**) devices, res );
  
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

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Unlock one or more PNP ISA devices  ****************************************
******************************************************************************/

AROS_LH1(void, ISAPNP_UnlockDevices,
         AROS_LHA(APTR, device_lock_handle, A0),
         struct ISAPNPBase *, res , 33, ISAPNP)
{
  AROS_LIBFUNC_INIT

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

  AROS_LIBFUNC_EXIT
}
