
/* $Id: pnp_iterators.c,v 1.6 2001/05/07 12:32:42 lcs Exp $ */

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
#include "init.h"
#include "pnp_iterators.h"
#include "pnp_structs.h"
#include "isapnp_private.h"


struct ResourceIterator
{
  struct MinNode          m_MinNode;
  struct ISAPNP_Resource* m_Resource;
  UBYTE                   m_IRQBit;
  UBYTE                   m_ChannelBit;
  UWORD                   m_Base;
  UWORD                   m_Length;
  BOOL                    m_HasLock;
  BOOL                    m_InConflict;
};


#undef DEBUG_ITERATORS

struct ResourceIteratorList
{
  struct MinList m_ResourceIterators;
};


struct ResourceIteratorRef
{
  struct MinNode           m_MinNode;
  struct ResourceIterator* m_Iterator;
};


struct ResourceContext
{
  struct ResourceIterator* m_IRQ[ 16 ];
  struct ResourceIterator* m_DMA[ 8 ];
  struct MinList           m_IO;
};


static struct ResourceIterator*
AllocResourceIterator( struct ISAPNP_Resource* resource,
                       struct ResourceContext* ctx );

static BOOL
FreeResourceIterator( struct ResourceIterator* iter,
                       struct ResourceContext* ctx );

static BOOL
IncResourceIterator( struct ResourceIterator* iter,
                     struct ResourceContext*  ctx );


/******************************************************************************
** Allocate a clean resource iterator context *********************************
******************************************************************************/

struct ResourceContext*
AllocResourceIteratorContext( void )
{
  struct ResourceContext* ctx;
  
  ctx = AllocVec( sizeof( *ctx ), MEMF_PUBLIC | MEMF_CLEAR );
  
  if( ctx != NULL )
  {
    NewList( (struct List*) &ctx->m_IO );
  }
  
  return ctx;
}


/******************************************************************************
** Free a resource iterator context *******************************************
******************************************************************************/

void
FreeResourceIteratorContext( struct ResourceContext* ctx )
{
  struct ResourceIteratorRef* ref;

  if( ctx == NULL )
  {
    return;
  }

  while( ( ref = (struct ResourceIteratorRef*) 
                 RemHead( (struct List*) &ctx->m_IO ) ) )
  {
    FreeVec( ref );
  }

  FreeVec( ctx );
}


/******************************************************************************
** Lock resources from the context ********************************************
******************************************************************************/

static BOOL
LockResource( struct ResourceIterator* iter,
              struct ResourceContext*  ctx )
{
  struct ResourceIterator* conflict = NULL;

  switch( iter->m_Resource->isapnpr_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
#ifdef DEBUG_ITERATORS
KPrintF( "L IRQ %ld", iter->m_IRQBit );
#endif
      if( ctx->m_IRQ[ iter->m_IRQBit ] == NULL )
      {
        ctx->m_IRQ[ iter->m_IRQBit ] = iter;
      }
      else
      {
        conflict = ctx->m_IRQ[ iter->m_IRQBit ];
      }
      
      break;
    }


    case ISAPNP_NT_DMA_RESOURCE:
    {
#ifdef DEBUG_ITERATORS
KPrintF( "L DMA %ld", iter->m_ChannelBit );
#endif
      if( ctx->m_DMA[ iter->m_ChannelBit ] == NULL )
      {
        ctx->m_DMA[ iter->m_ChannelBit ] = iter;
      }
      else
      {
        conflict = ctx->m_DMA[ iter->m_ChannelBit ];
      }

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ResourceIteratorRef* io;
      struct ResourceIteratorRef* new_io;
      struct Node*                position = NULL;

      UWORD base   = iter->m_Base;
      UWORD length = iter->m_Length;

#ifdef DEBUG_ITERATORS
KPrintF( "L IO %lx-%lx", base, base + length );
#endif

      for( io = (struct ResourceIteratorRef*) ctx->m_IO.mlh_Head;
           io->m_MinNode.mln_Succ != NULL;
           io = (struct ResourceIteratorRef*) io->m_MinNode.mln_Succ )
      {
        UWORD io_base   = io->m_Iterator->m_Base;
        UWORD io_length = io->m_Iterator->m_Length;

        if( ( base <= io_base && ( base + length ) > io_base )    ||
            ( base >= io_base && base < ( io_base + io_length ) ) )
        {
          // Collision!

          conflict = io->m_Iterator;
          break;
        }

        if( base + length <= io_base )
        {
          // No more collisions possible; insert before this one
    
          position = (struct Node*) io->m_MinNode.mln_Pred;

          break;
        }
      }

      if( conflict == NULL )
      {
        // Insert the node
    
        new_io = AllocVec( sizeof( *new_io ), MEMF_PUBLIC );

        if( new_io == NULL )
        {
          return FALSE;
        }
        else
        {
          new_io->m_Iterator = iter;

          if( position == NULL )
          {
            AddTail( (struct List*) &ctx->m_IO, (struct Node*) new_io );
          }
          else
          {
            Insert( (struct List*) &ctx->m_IO, (struct Node*) new_io, position );
          }
        }
      }

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      break;
  }

  if( conflict != NULL )
  {
    conflict->m_InConflict = TRUE;
    iter->m_HasLock = FALSE;
  }
  else
  {
    iter->m_HasLock = TRUE;
  }

#ifdef DEBUG_ITERATORS
if( iter->m_HasLock ) KPrintF( " OK\n" ); else KPrintF( " failed\n" );
#endif

  return iter->m_HasLock;
}


/******************************************************************************
** Unlock resources from the context ******************************************
******************************************************************************/

static void
UnlockResource( struct ResourceIterator* iter,
                struct ResourceContext*  ctx )
{
  if( ! iter->m_HasLock )
  {
    return;
  }

  switch( iter->m_Resource->isapnpr_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
#ifdef DEBUG_ITERATORS
KPrintF( "U IRQ %ld\n", iter->m_IRQBit );
#endif
      ctx->m_IRQ[ iter->m_IRQBit ] = NULL;

      break;
    }


    case ISAPNP_NT_DMA_RESOURCE:
    {
#ifdef DEBUG_ITERATORS
KPrintF( "U DMA %ld\n", iter->m_ChannelBit );
#endif
      ctx->m_DMA[ iter->m_ChannelBit ] = NULL;

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ResourceIteratorRef* io;

#ifdef DEBUG_ITERATORS
KPrintF( "U IO %lx-%lx\n", iter->m_Base, 
         iter->m_Base + iter->m_Length );
#endif

      for( io = (struct ResourceIteratorRef*) ctx->m_IO.mlh_Head;
           io->m_MinNode.mln_Succ != NULL;
           io = (struct ResourceIteratorRef*) io->m_MinNode.mln_Succ )
      {
        if( io->m_Iterator->m_Base == iter->m_Base )
        {
          Remove( (struct Node*) io );
          FreeVec( io );
          break;
        }
      }

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      break;
  }

  iter->m_HasLock = FALSE;
}


/******************************************************************************
** Reset a resource iterator **************************************************
******************************************************************************/

static BOOL
ResetResourceIterator( struct ResourceIterator* iter,
                       struct ResourceContext*  ctx )
{
  BOOL rc = FALSE;

  switch( iter->m_Resource->isapnpr_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
      struct ISAPNP_IRQResource* r;

      r = (struct ISAPNP_IRQResource*) iter->m_Resource;

      iter->m_IRQBit = 0;
      
      while( iter->m_IRQBit < 16 )
      {
        if( r->isapnpirqr_IRQMask & ( 1 << iter->m_IRQBit ) )
        {
          if( LockResource( iter, ctx ) )
          {
            rc = TRUE;
            break;
          }
        }

        ++iter->m_IRQBit;
      }

      break;
    }


    case ISAPNP_NT_DMA_RESOURCE:
    {
      struct ISAPNP_DMAResource* r;

      r = (struct ISAPNP_DMAResource*) iter->m_Resource;

      iter->m_ChannelBit = 0;
      
      while( iter->m_ChannelBit < 8 )
      {
        if( r->isapnpdmar_ChannelMask & ( 1 << iter->m_ChannelBit ) )
        {
          if( LockResource( iter, ctx ) )
          {
            rc = TRUE;
            break;
          }
        }

        ++iter->m_ChannelBit;
      }

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ISAPNP_IOResource* r;
      
      r = (struct ISAPNP_IOResource*) iter->m_Resource;

      iter->m_Base   = r->isapnpior_MinBase;
      iter->m_Length = r->isapnpior_Length;


      while( iter->m_Base <= r->isapnpior_MaxBase )
      {
        if( LockResource( iter, ctx ) )
        {
          rc = TRUE;
          break;
        }

        iter->m_Base += r->isapnpior_Alignment;
      }

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      break;
  }

  return rc;
}
                      

/******************************************************************************
** Allocate a resource iterator ***********************************************
******************************************************************************/

static struct ResourceIterator*
AllocResourceIterator( struct ISAPNP_Resource* resource,
                       struct ResourceContext* ctx )
{
  struct ResourceIterator* iter;
  
  iter = AllocVec( sizeof( *iter ), MEMF_PUBLIC | MEMF_CLEAR );

  if( iter != NULL )
  {
    iter->m_Resource = resource;

    if( ! ResetResourceIterator( iter, ctx ) )
    {
      FreeResourceIterator( iter, ctx );
      iter = NULL;
    }
  }
  
  return iter;
}


/******************************************************************************
** Deallocate a resource iterator *********************************************
******************************************************************************/

static BOOL
FreeResourceIterator( struct ResourceIterator* iter,
                      struct ResourceContext* ctx )
{
  BOOL rc = FALSE;

  if( iter == NULL )
  {
    return FALSE;
  }

  UnlockResource( iter, ctx );

  rc = iter->m_InConflict;

  FreeVec( iter );
  
  return rc;
}


/******************************************************************************
** Allocate a resource iterator list ******************************************
******************************************************************************/

// This function cannot handle conflicts within the list itself

struct ResourceIteratorList*
AllocResourceIteratorList( struct MinList*         resource_list,
                           struct ResourceContext* ctx )
{
  struct ResourceIteratorList* result;
  
#ifdef DEBUG_ITERATORS
KPrintF( "AllocResourceIteratorList()\n" );
#endif

  result = AllocVec( sizeof( *result ), MEMF_PUBLIC | MEMF_CLEAR );
  
  if( result != NULL )
  {
    struct ISAPNP_Resource* r;

    NewList( (struct List*) &result->m_ResourceIterators );
    
    r = (struct ISAPNP_Resource*) resource_list->mlh_Head;
    
    while( r->isapnpr_MinNode.mln_Succ != NULL )
    {
      struct ResourceIterator* iter;
      
      iter = AllocResourceIterator( r, ctx );

      if( iter == NULL )
      {
        FreeResourceIteratorList( result, ctx );
        result = NULL;
        break;
      }
      
      AddTail( (struct List*) &result->m_ResourceIterators,
               (struct Node*) iter );
      
      r = (struct ISAPNP_Resource*) r->isapnpr_MinNode.mln_Succ;
    }
  }

#ifdef DEBUG_ITERATORS
if( result ) KPrintF( "AllocResourceIteratorList() succeeded\n" );
else KPrintF( "AllocResourceIteratorList() failed\n" );
#endif

  return result;
}


/******************************************************************************
** Deallocate a resource iterator list ****************************************
******************************************************************************/

BOOL
FreeResourceIteratorList( struct ResourceIteratorList* list,
                          struct ResourceContext*      ctx )
{
  BOOL rc = FALSE;

  struct ResourceIterator* iter;

#ifdef DEBUG_ITERATORS
KPrintF( "FreeResourceIteratorList()\n" );
#endif

  if( list == NULL )
  {
    return FALSE;
  }

  while( ( iter = (struct ResourceIterator*) 
                  RemHead( (struct List*) &list->m_ResourceIterators ) ) )
  {  
    if( FreeResourceIterator( iter, ctx ) )
    {
      rc = TRUE;
    }
  }

  FreeVec( list );

#ifdef DEBUG_ITERATORS
if( rc ) KPrintF( "FreeResourceIteratorList() T\n" ); 
else KPrintF( "FreeResourceIteratorList() F\n" );
#endif

  return rc;
}


/******************************************************************************
** Increase a resource iterator ***********************************************
******************************************************************************/

static BOOL
IncResourceIterator( struct ResourceIterator* iter,
                     struct ResourceContext*  ctx )
{
  BOOL rc = FALSE;

  UnlockResource( iter, ctx );

  switch( iter->m_Resource->isapnpr_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
      struct ISAPNP_IRQResource* r;

      r = (struct ISAPNP_IRQResource*) iter->m_Resource;
      
      while( ! rc && iter->m_IRQBit < 16 )
      {
        ++iter->m_IRQBit;

        if( r->isapnpirqr_IRQMask & ( 1 << iter->m_IRQBit ) )
        {
          rc = LockResource( iter, ctx );
        }
      }

      break;
    }

    case ISAPNP_NT_DMA_RESOURCE:
    {
      struct ISAPNP_DMAResource* r;

      r = (struct ISAPNP_DMAResource*) iter->m_Resource;
      
      while( ! rc && iter->m_ChannelBit < 8 )
      {
        ++iter->m_ChannelBit;

        if( r->isapnpdmar_ChannelMask & ( 1 << iter->m_ChannelBit ) )
        {
          rc = LockResource( iter, ctx );
        }
      }

      break;
    }

    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ISAPNP_IOResource* r;

      r = (struct ISAPNP_IOResource*) iter->m_Resource;
      
      while( ! rc && iter->m_Base <= r->isapnpior_MaxBase )
      {
        iter->m_Base += r->isapnpior_Alignment;

        if( iter->m_Base <= r->isapnpior_MaxBase )
        {
          rc = LockResource( iter, ctx );
        }
      }

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      break;;
  }
  
  return rc;
}


/******************************************************************************
** Increase a resource iterator list ******************************************
******************************************************************************/

BOOL
IncResourceIteratorList( struct ResourceIteratorList* iter_list,
                         struct ResourceContext*      ctx )
{
  BOOL                     rc = FALSE;
  struct ResourceIterator* current;

#ifdef DEBUG_ITERATORS
KPrintF( "IncResourceIteratorList()\n" );
#endif

  current = (struct ResourceIterator*) iter_list->m_ResourceIterators.mlh_Head;

  while( ! rc && current->m_MinNode.mln_Succ != NULL )
  {
    if( current->m_InConflict )
    {
      rc = IncResourceIterator( current, ctx );
    }    
    else
    {
      // This iterator is not in conflict. There is no need to 
      // try any more combinations

      UnlockResource( current, ctx );
    }

    if( ! rc )
    {
      ResetResourceIterator( current, ctx );

      current = (struct ResourceIterator*) current->m_MinNode.mln_Succ;
    }
  }
  
#ifdef DEBUG_ITERATORS
KPrintF( "IncResourceIteratorList(): %ld\n", rc );
#endif

  return rc;
}


/******************************************************************************
** Create a resource from an iterator *****************************************
******************************************************************************/

struct ISAPNP_Resource*
CreateResource( struct ResourceIterator* iter,
                struct ISAPNPBase*       res )
{
  struct ISAPNP_Resource* result = NULL;

  result = ISAPNP_AllocResource( iter->m_Resource->isapnpr_Type, res );

  if( result == NULL )
  {
    return NULL;
  }

  switch( iter->m_Resource->isapnpr_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
      struct ISAPNP_IRQResource* r;

      r = (struct ISAPNP_IRQResource*) result;
      
      // Make a copy of the iterators resource

      CopyMem( iter->m_Resource, r, sizeof( *r ) );
      r->isapnpirqr_MinNode.mln_Succ = NULL;
      r->isapnpirqr_MinNode.mln_Pred = NULL;
      
      r->isapnpirqr_IRQMask = 1 << iter->m_IRQBit;

      break;
    }

    case ISAPNP_NT_DMA_RESOURCE:
    {
      struct ISAPNP_DMAResource* r;

      r = (struct ISAPNP_DMAResource*) result;
      
      // Make a copy of the iterators resource

      CopyMem( iter->m_Resource, r, sizeof( *r ) );
      r->isapnpdmar_MinNode.mln_Succ = NULL;
      r->isapnpdmar_MinNode.mln_Pred = NULL;
      
      r->isapnpdmar_ChannelMask = 1 << iter->m_ChannelBit;

      break;
    }

    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ISAPNP_IOResource* r;

      r = (struct ISAPNP_IOResource*) result;
      
      // Make a copy of the iterators resource

      CopyMem( iter->m_Resource, r, sizeof( *r ) );
      r->isapnpior_MinNode.mln_Succ = NULL;
      r->isapnpior_MinNode.mln_Pred = NULL;

      r->isapnpior_MinBase   = iter->m_Base;
      r->isapnpior_MaxBase   = iter->m_Base;
      r->isapnpior_Alignment = 1;

      break;
    }

    case ISAPNP_NT_MEMORY_RESOURCE:
    default:
      ISAPNP_FreeResource( result, res );
      result = NULL;
      break;;
  }
  
  return result;
}


/******************************************************************************
** Create resources from a list of iterators **********************************
******************************************************************************/

BOOL
CreateResouces( struct ResourceIteratorList* ril,
                struct List*                 result,
                struct ISAPNPBase*           res )
{
  // Allocate resources for current iterators

  struct ResourceIterator* iter;

  for( iter = (struct ResourceIterator*) 
              ril->m_ResourceIterators.mlh_Head;
       iter->m_MinNode.mln_Succ != NULL;
       iter = (struct ResourceIterator*) 
              iter->m_MinNode.mln_Succ )
  {
    struct ISAPNP_Resource* resource;

    resource = CreateResource( iter, res );
    
    if( resource == NULL )
    {
      return FALSE;
    }
    
    AddTail( result, (struct Node*) resource );
  }

  return TRUE;
}
