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

#define DEBUG 1

#include "CompilerSpecific.h"

#include <exec/memory.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <resources/isapnp.h>
#include "isapnp_private.h"

#include "pnpisa.h"

#include "controller.h"
#include "devices.h"
#include "init.h"
#include "pnp.h"
#include "pnp_iterators.h"
#include "pnp_structs.h"


/******************************************************************************
** PnP ISA helper functions ***************************************************
******************************************************************************/

static UBYTE
GetPnPReg( UBYTE              pnp_reg,
           struct ISAPNPBase* res )
{
  ISAC_SetRegByte( PNPISA_ADDRESS, pnp_reg, res );

  return ISAC_GetRegByte( ( res->m_RegReadData << 2 ) | 3, res );
}


static UBYTE
GetLastPnPReg( struct ISAPNPBase* res )
{
  return ISAC_GetRegByte( ( res->m_RegReadData << 2 ) | 3, res );
}


static void
SetPnPReg( UBYTE              pnp_reg,
           UBYTE              value,
           struct ISAPNPBase* res )
{
  ISAC_SetRegByte( PNPISA_ADDRESS,    pnp_reg, res );
  ISAC_SetRegByte( PNPISA_WRITE_DATA, value,   res );
}


static UBYTE
GetNextResourceData( struct ISAPNPBase* res )
{
  // Poll for data
  while( ( GetPnPReg( PNPISA_REG_STATUS, res ) & PNPISA_SF_AVAILABLE ) == 0 );

  return GetPnPReg( PNPISA_REG_RESOURCE_DATA, res );

}


static void
BusyWait( ULONG micro_seconds )
{
  typedef unsigned long long uint64_t;

  ULONG            freq;
  struct EClockVal eclock;
  uint64_t         current;
  uint64_t         end;

  freq = ReadEClock( &eclock );

  current = ( ( (uint64_t) eclock.ev_hi ) << 32 ) + eclock.ev_lo;
  end     = current + (uint64_t) freq * micro_seconds / 1000000;

  while( current < end )
  {
    ReadEClock( &eclock );

    current = ( ( (uint64_t) eclock.ev_hi ) << 32 ) + eclock.ev_lo;
  }
}


static BOOL
DecodeID( UBYTE* buf, struct ISAPNP_Identifier* id )
{
  // Decode information

  if( buf[ 0 ] & 0x80 )
  {
    return FALSE;
  }

  id->isapnpid_Vendor[ 0 ]  = 0x40 + ( buf[ 0 ] >> 2 );
  id->isapnpid_Vendor[ 1 ]  = 0x40 + ( ( ( buf[ 0 ] & 0x03 ) << 3 ) | 
                                       ( buf[ 1 ] >> 5 ) );
  id->isapnpid_Vendor[ 2 ]  = 0x40 + ( buf[ 1 ] & 0x1f );
  id->isapnpid_Vendor[ 3 ]  = 0;

  id->isapnpid_ProductID    = ( buf[ 2 ] << 4 ) | ( buf[ 3 ] >> 4 );
  id->isapnpid_Revision     = buf[ 3 ] & 0x0f;
  
  return TRUE;
}


/******************************************************************************
** Move all cards to Sleep state **********************************************
******************************************************************************/

static void
SendInitiationKey( struct ISAPNPBase* res )
{
  static const UBYTE key[] =
  {
    PNPISA_INITIATION_KEY
  };
  
  unsigned int i;

  // Make sure the LFSR is in its initial state

  ISAC_SetRegByte( PNPISA_ADDRESS, 0, res );
  ISAC_SetRegByte( PNPISA_ADDRESS, 0, res );

  for( i = 0; i < sizeof( key ); ++i )
  {
    ISAC_SetRegByte( PNPISA_ADDRESS, key[ i ], res );
  }
}


/******************************************************************************
** Read the serial identifier from a card *************************************
******************************************************************************/

static BOOL
ReadSerialIdentifier( struct ISAPNP_Card* card,
                      struct ISAPNPBase*  res )
{
  UBYTE buf[ 9 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int   i;
  UBYTE check_sum;
  
  ISAC_SetRegByte( PNPISA_ADDRESS, PNPISA_REG_SERIAL_ISOLATION, res );

  // Wait 1 ms before we read the first pair

  BusyWait( 1000 );

  check_sum = 0x6a;

  for( i = 0; i < 72; ++i )
  {
    UBYTE value1;
    UBYTE value2;

    value1 = GetLastPnPReg( res );
    value2 = GetLastPnPReg( res );
    
    if( value1 == 0x55 && value2 == 0xaa )
    {
      buf[ i >> 3 ] |= ( 1 << ( i & 7 ) );
    }

    if( i < 64 )
    {
      UBYTE bitten = value1 == 0x55 && value2 == 0xaa ? 1 : 0;

      check_sum = PNPISA_LFSR( check_sum, bitten );
    }

    // Wait 250 µs before we read the next pair
    
    BusyWait( 250 );
  }

  if( check_sum != buf[ 8 ] )
  {
    return FALSE;
  }

  if( ! DecodeID( buf, &card->isapnpc_ID ) )
  {
    return FALSE;
  }
  
  card->isapnpc_SerialNumber = ( buf[ 7 ] << 25 ) |
                               ( buf[ 6 ] << 16 ) |
                               ( buf[ 5 ] << 8 )  | 
                                 buf[ 4 ];

  return TRUE;
}


/******************************************************************************
** Read the resource data from a card *****************************************
******************************************************************************/

static BOOL
ReadResourceData( struct ISAPNP_Card* card,
                  struct ISAPNPBase*  res )
{
  UBYTE                        check_sum     = 0;
  UWORD                        dev_num       = 0;
  BOOL                         read_more     = TRUE;
  struct ISAPNP_Device*        dev           = NULL;
  struct ISAPNP_ResourceGroup* options       = NULL;
  struct ISAPNP_ResourceGroup* saved_options = NULL;

  while( read_more )
  {
    ULONG i;
    UBYTE rd;

    UBYTE name;
    ULONG length;

    rd = GetNextResourceData( res );

    check_sum += rd;

    if( rd & PNPISA_RDF_LARGE )
    {
      name    = PNPISA_RD_LARGE_ITEM_NAME( rd );
      length  = GetNextResourceData( res );
      length |= GetNextResourceData( res ) << 8;
    }
    else
    {
      name   = PNPISA_RD_SMALL_ITEM_NAME( rd );
      length = PNPISA_RD_SMALL_LENGTH( rd );
    }

    switch( name )
    {
      case PNPISA_RDN_PNP_VERSION:
      {
        UBYTE ver;
        
        ver = GetNextResourceData( res );

        card->isapnpc_MinorPnPVersion  = ver & 0x0f;
        card->isapnpc_MajorPnPVersion  = ver >> 4;
        card->isapnpc_VendorPnPVersion = GetNextResourceData( res );
        
        length -= 2;
        break;
      }

      case PNPISA_RDN_LOGICAL_DEVICE_ID:
      {
        struct ISAPNP_Identifier* id;
        UBYTE                     buf[ 4 ];

        dev = ISAPNP_AllocDevice( res );
        
        if( dev == NULL )
        {
          break;
        }

        dev->isapnpd_Card = card;

        id  = AllocVec( sizeof( *id ), MEMF_PUBLIC | MEMF_CLEAR );
        
        if( id == NULL )
        {
          ISAPNP_FreeDevice( dev, res );
          dev = NULL;
          break;
        }
        
        buf[ 0 ] = GetNextResourceData( res );
        buf[ 1 ] = GetNextResourceData( res );
        buf[ 2 ] = GetNextResourceData( res );
        buf[ 3 ] = GetNextResourceData( res );

        length -= 4;

        if( ! DecodeID( buf, id ) )
        {
          FreeVec( id );
          ISAPNP_FreeDevice( dev, res );
          dev = NULL;
          break;
        }
        
        AddTail( (struct List*) &dev->isapnpd_IDs, (struct Node*) id );

        options = dev->isapnpd_Options;

        dev->isapnpd_SupportedCommands = GetNextResourceData( res );
        --length;
        
        if( length > 0 )
        {
          dev->isapnpd_SupportedCommands |= GetNextResourceData( res ) << 8;
        }
        
        dev->isapnpd_DeviceNumber = dev_num;
        ++dev_num;

        AddTail( &card->isapnpc_Devices, (struct Node*) dev );

        break;
      }
      

      case PNPISA_RDN_COMPATIBLE_DEVICE_ID:
      {
        struct ISAPNP_Identifier* id;
        UBYTE                     buf[ 4 ];

        if( dev == NULL )
        {
          break;
        }

        id  = AllocVec( sizeof( *id ), MEMF_PUBLIC | MEMF_CLEAR );
        
        if( id == NULL )
        {
          break;
        }

        buf[ 0 ] = GetNextResourceData( res );
        buf[ 1 ] = GetNextResourceData( res );
        buf[ 2 ] = GetNextResourceData( res );
        buf[ 3 ] = GetNextResourceData( res );

        length -= 4;

        if( ! DecodeID( buf, id ) )
        {
          FreeVec( id );
          break;
        }

        AddTail( (struct List*) &dev->isapnpd_IDs, (struct Node*) id );

        break;
      }

      case PNPISA_RDN_IRQ_FORMAT:
      {
        struct ISAPNP_IRQResource* r;
        
        r = (struct ISAPNP_IRQResource*) 
            ISAPNP_AllocResource( ISAPNP_NT_IRQ_RESOURCE, res );
            
        if( r == NULL )
        {
          break;
        }
        
        r->isapnpirqr_IRQMask  = GetNextResourceData( res );
        r->isapnpirqr_IRQMask |= GetNextResourceData( res ) << 8;
        length -= 2;
        
        if( length > 0 )
        {
          r->isapnpirqr_IRQType = GetNextResourceData( res );
          --length;
        }
        else
        {
          r->isapnpirqr_IRQType = ISAPNP_IRQRESOURCE_ITF_HIGH_EDGE;
        }

        AddTail( (struct List*) &options->isapnprg_Resources, (struct Node*) r );

        break;
      }


      case PNPISA_RDN_DMA_FORMAT:
      {
        struct ISAPNP_DMAResource* r;
        
        r = (struct ISAPNP_DMAResource*) 
            ISAPNP_AllocResource( ISAPNP_NT_DMA_RESOURCE, res );
            
        if( r == NULL )
        {
          break;
        }
        
        r->isapnpdmar_ChannelMask = GetNextResourceData( res );
        r->isapnpdmar_Flags       = GetNextResourceData( res );

        length -= 2;

        AddTail( (struct List*) &options->isapnprg_Resources, (struct Node*) r );

        break;
      }

      case PNPISA_RDN_START_DF:
      {
        struct ISAPNP_ResourceGroup* rg;
        UBYTE  pri;
        
        if( length > 0 )
        {
          pri = GetNextResourceData( res );
          --length;
        }
        else
        {
          pri = 1;
        }
        
        switch( pri )
        {
          case 0:
            pri = ISAPNP_RG_PRI_GOOD;
            break;
            
          case 1:
          default:
            pri = ISAPNP_RG_PRI_ACCEPTABLE;
            break;

          case 2:
            pri = ISAPNP_RG_PRI_SUBOPTIMAL;
            break;
        }
        
        rg = ISAPNP_AllocResourceGroup( pri, res );
        
        if( rg == NULL )
        {
          break;
        }

        // Insert in priority order
        
        if( saved_options == NULL )
        {
          saved_options = options;
        }

        Enqueue( (struct List*) &saved_options->isapnprg_ResourceGroups, 
                 (struct Node*) rg );

        options = rg;

        break;
      }
      

      case PNPISA_RDN_END_DF:
      {
        options       = saved_options;
        saved_options = NULL;
        
        break;
      }

      case PNPISA_RDN_IO_PORT:
      {
        struct ISAPNP_IOResource* r;
        
        r = (struct ISAPNP_IOResource*) 
            ISAPNP_AllocResource( ISAPNP_NT_IO_RESOURCE, res );
            
        if( r == NULL )
        {
          break;
        }
        
        r->isapnpior_Flags      = GetNextResourceData( res );

        r->isapnpior_MinBase    = GetNextResourceData( res );
        r->isapnpior_MinBase   |= GetNextResourceData( res ) << 8;

        r->isapnpior_MaxBase    = GetNextResourceData( res );
        r->isapnpior_MaxBase   |= GetNextResourceData( res ) << 8;

        r->isapnpior_Alignment  = GetNextResourceData( res );
        r->isapnpior_Length     = GetNextResourceData( res );

        length -= 7;

        AddTail( (struct List*) &options->isapnprg_Resources, (struct Node*) r );

        break;
      }


      case PNPISA_RDN_FIXED_IO_PORT:
      {
        struct ISAPNP_IOResource* r;
        
        r = (struct ISAPNP_IOResource*) 
            ISAPNP_AllocResource( ISAPNP_NT_IO_RESOURCE, res );
            
        if( r == NULL )
        {
          break;
        }
        
        r->isapnpior_Flags      = 0;
        r->isapnpior_MinBase    = GetNextResourceData( res );
        r->isapnpior_MinBase   |= GetNextResourceData( res ) << 8;
        r->isapnpior_MaxBase    = r->isapnpior_MinBase;
        r->isapnpior_Alignment  = 1;
        r->isapnpior_Length     = GetNextResourceData( res );
        
        length -= 3;

        AddTail( (struct List*) &options->isapnprg_Resources, (struct Node*) r );

        break;
      }


      case PNPISA_RDN_ANSI_IDENTIFIER:
      {
        STRPTR id = AllocVec( length + 1, MEMF_PUBLIC );
        
        if( id == NULL )
        {
          break;
        }

        // Attach identifier to either the card or the logical device

        if( dev == NULL )
        {
          FreeVec( card->isapnpc_Node.ln_Name );

          card->isapnpc_Node.ln_Name = id;
        }
        else
        {
          FreeVec( dev->isapnpd_Node.ln_Name );

          dev->isapnpd_Node.ln_Name = id;
        }
        
        while( length > 0 )
        {
          *id = GetNextResourceData( res );

          ++id;
          --length;
        }
        
        // Terminate the string

        *id = '\0';

        break;
      }
      

      case PNPISA_RDN_END_TAG:
      {
        UBYTE cs;
        
        cs = GetNextResourceData( res );
        --length;
        
        if( cs == 0 )
        {
          check_sum = 0;
        }
        else
        {
          check_sum += cs;
        }

        read_more = FALSE;
        break;
      }


      default:
        bug( "UNKNOWN RESOURCE: %02lx, length %ld: ", name, length );

        while( length > 0 )
        {
          bug( "%02lx ", GetNextResourceData( res ) );
          --length;
        }

        bug( "\n" );
        break;
    }

    // Make sure we have read all data for this name

    while( length > 0 )
    {
      bug( "*** " );
      GetNextResourceData( res );
      --length;
    }
  }

  D(bug( "Check sum: %ld\n", check_sum ));
  
  return TRUE;
}


/******************************************************************************
** Assign a CSN to all cards and build the card/device database ***************
******************************************************************************/

static ULONG
AddCards( UBYTE              rd_data_port_value, 
          struct ISAPNPBase* res )
{
  UBYTE csn = 0;

  while( TRUE )
  {
    struct ISAPNP_Card* card;

    // Move all cards (without a CSN) to the isolate state and the card
    // in config state to the sleep state.
  
    SetPnPReg( PNPISA_REG_WAKE, 0, res );
  
    // Set port to read from
  
    SetPnPReg( PNPISA_REG_SET_RD_DATA_PORT, rd_data_port_value, res );
    res->m_RegReadData = rd_data_port_value;

    card = ISAPNP_AllocCard( res );

    if( card == NULL )
    {
      break;
    }

    if( ReadSerialIdentifier( card, res ) )
    {
      // Assign a CSN to the card and move it to the config state

      ++csn;
      SetPnPReg( PNPISA_REG_CARD_SELECT_NUMBER, csn, res );

      // Read the resource data
      
      if( ReadResourceData( card, res ) )
      {
        card->isapnpc_CSN = csn;
      
        AddTail( &res->m_Cards, (struct Node*) card );
      }
      else
      {
        ISAPNP_FreeCard( card, res );
      }
    }
    else
    {
      ISAPNP_FreeCard( card, res );

      break;
    }
  }

  return csn;
}


/******************************************************************************
** Scan for all PNP ISA cards *************************************************
******************************************************************************/

AROS_LH0(BOOL, ISAPNP_ScanCards,
         struct ISAPNPBase *, res, 26, ISAPNP)
{
  AROS_LIBFUNC_INIT

  int read_port_value;
  int cards = 0;
 
  SendInitiationKey( res );

  // Make all cards lose their CSN

  SetPnPReg( PNPISA_REG_CONFIG_CONTROL,
             PNPISA_CCF_RESET_CSN,
             res );

  for( read_port_value = 0x80; 
       read_port_value <= 0xff; 
       read_port_value += 0x10 )
  {
    cards = AddCards( read_port_value, res );

    if( cards > 0 )
    {
      break;
    }
  }

  // Reset all cards
/*
  SetPnPReg( PNPISA_REG_CONFIG_CONTROL,
             PNPISA_CCF_RESET        |
             PNPISA_CCF_WAIT_FOR_KEY |
             PNPISA_CCF_RESET_CSN,
             res );
*/
  return cards != 0;

  AROS_LIBFUNC_EXIT
}


/******************************************************************************
** Configure all cards ********************************************************
******************************************************************************/


static BOOL
FindNextCardConfiguration( struct ISAPNP_Device*   dev,
                           struct ResourceContext* ctx,
                           struct ISAPNPBase*      res );


int cp = 0;

static BOOL
FindConfiguration( struct ISAPNP_Device*   dev,
                   struct ResourceContext* ctx,
                   struct ISAPNPBase*      res )
{
  BOOL rc = FALSE;

  struct ISAPNP_ResourceGroup* rg;
  struct ResourceIteratorList* ril = NULL;

  if( dev->isapnpd_Disabled || dev->isapnpd_Card->isapnpc_Disabled )
  {
    // Skip to next device
    
    return FindNextCardConfiguration( dev, ctx, res );
  }

  ril = AllocResourceIteratorList( &dev->isapnpd_Options->isapnprg_Resources, ctx );

  if( ril != NULL )
  {
    BOOL ril_iter_ok = TRUE;

    while( ! rc && ril_iter_ok )
    {
      if( dev->isapnpd_Options->isapnprg_ResourceGroups.mlh_Head->mln_Succ != NULL )
      {
        // Handle resource options as well

        for( rg = (struct ISAPNP_ResourceGroup*) 
                  dev->isapnpd_Options->isapnprg_ResourceGroups.mlh_Head;
             ! rc && rg->isapnprg_MinNode.mln_Succ != NULL;
             rg = (struct ISAPNP_ResourceGroup*) rg->isapnprg_MinNode.mln_Succ )
        {
          struct ResourceIteratorList* ril_option = NULL;

          ril_option = AllocResourceIteratorList( &rg->isapnprg_Resources, ctx );

          if( ril_option != NULL )
          {
            BOOL ril2_iter_ok = TRUE;

            while( ! rc && ril2_iter_ok )
            {
              rc = FindNextCardConfiguration( dev, ctx, res );

//if( cp > 2 ) return FALSE; else ++cp;

              if( ! rc )
              {
                ril2_iter_ok = IncResourceIteratorList( ril_option, ctx );
              }
              else
              {
                // Allocate resources for current iterators

                rc = CreateResouces( ril_option, 
                                     (struct List*) &dev->isapnpd_Resources,
                                     res );
                break;
              }
            }

            if( ! FreeResourceIteratorList( ril_option, ctx ) )
            {
              break;
            }
          }
        }
      }
      else
      {
        // Fixed resources only
        
        rc = FindNextCardConfiguration( dev, ctx, res );
      }

      if( ! rc )
      {
        ril_iter_ok = IncResourceIteratorList( ril, ctx );
      }
      else
      {
        // Allocate resources for current iterators

        // NOTE! These resources muct be FIRST in the resource list
        // in order to maintain the descriptor order.

        struct MinList tmp;
        struct Node*   n;
        
        NewList( (struct List* ) &tmp );

        rc = CreateResouces( ril, (struct List*) &tmp,
                             res );

        while( ( n = RemTail( (struct List* ) &tmp ) ) != NULL )
        {
          AddHead( (struct List*) &dev->isapnpd_Resources, n );
        }

        break;
      }
    }

    FreeResourceIteratorList( ril, ctx );
  }
  
  return rc;
}


static BOOL
FindNextCardConfiguration( struct ISAPNP_Device*   dev,
                           struct ResourceContext* ctx,
                           struct ISAPNPBase*      res )
{
  BOOL rc = FALSE;

  // Configure next logical device, if any

  if( dev->isapnpd_Node.ln_Succ->ln_Succ != NULL )
  {
    // Same card, next device
    rc = FindConfiguration( (struct ISAPNP_Device*) dev->isapnpd_Node.ln_Succ,
                            ctx, res );
  }
  else 
  {
    struct ISAPNP_Card* next_card = (struct ISAPNP_Card*) 
                                    dev->isapnpd_Card->isapnpc_Node.ln_Succ;

    if( next_card->isapnpc_Node.ln_Succ != NULL &&
        next_card->isapnpc_Devices.lh_Head->ln_Succ != NULL )
        
    {
      rc = FindConfiguration( (struct ISAPNP_Device*)
                              next_card->isapnpc_Devices.lh_Head, 
                              ctx, res );
    }
    else
    {
      // This was the last device on the last card!

      rc = TRUE;
    }
  }

  return rc;
}


BOOL
ProgramConfiguration( struct ISAPNPBase* res )
{
  struct ISAPNP_Device* dev = NULL;

  UBYTE csn     = 0;

  while( ( dev = ISAPNP_FindDevice( dev, -1, -1, -1, res ) ) != NULL )
  {
    struct ISAPNP_Resource* resource;

    UBYTE mem_reg = PNPISA_REG_MEMORY_BASE_ADDRESS_HIGH_0;
    UBYTE io_reg  = PNPISA_REG_IO_PORT_BASE_ADDRESS_HIGH_0;
    UBYTE int_reg = PNPISA_REG_INTERRUPT_REQUEST_LEVEL_SELECT_0;
    UBYTE dma_reg = PNPISA_REG_DMA_CHANNEL_SELECT_0;

    if( dev->isapnpd_Card->isapnpc_CSN != 0 )
    {
      // Don't program non-pnp devices!

      if( dev->isapnpd_Card->isapnpc_CSN != csn )
      {
        csn = dev->isapnpd_Card->isapnpc_CSN;

        // Wake the new card
  
	D(bug( "Woke up card %ld\n", dev->isapnpd_Card->isapnpc_CSN ));
        SetPnPReg( PNPISA_REG_WAKE, dev->isapnpd_Card->isapnpc_CSN, res );
      }

      // Select logical device

      D(bug( "Selected device %ld\n", dev->isapnpd_DeviceNumber ));
      SetPnPReg( PNPISA_REG_LOGICAL_DEVICE_NUMBER, dev->isapnpd_DeviceNumber, res );
  
      for( resource = (struct ISAPNP_Resource*) dev->isapnpd_Resources.mlh_Head;
           resource->isapnpr_MinNode.mln_Succ != NULL;
           resource = (struct ISAPNP_Resource*) resource->isapnpr_MinNode.mln_Succ )
      {
        switch( resource->isapnpr_Type )
        {
          case ISAPNP_NT_IRQ_RESOURCE:
          {
            struct ISAPNP_IRQResource* r = (struct ISAPNP_IRQResource*) resource;
            int                        b;
        
            for( b = 0; b < 16; ++b )
            {
              if( r->isapnpirqr_IRQMask & ( 1 << b ) )
              {
		D(bug( "Programmed interrupt %ld in %lx\n", b, int_reg ));
                SetPnPReg( int_reg, b, res);
                break;
              }
            }
  
            b = 0;
            
            if( ( r->isapnpirqr_IRQType & ISAPNP_IRQRESOURCE_ITF_HIGH_EDGE ) ||
                ( r->isapnpirqr_IRQType & ISAPNP_IRQRESOURCE_ITF_HIGH_LEVEL ) )
            {
              b |= 2;
            }
            
            if( ( r->isapnpirqr_IRQType & ISAPNP_IRQRESOURCE_ITF_HIGH_LEVEL ) ||
                ( r->isapnpirqr_IRQType & ISAPNP_IRQRESOURCE_ITF_LOW_LEVEL ) )
            {
              b |= 1;
            }
  
	    D(bug( "Programmed interrupt mode %ld in %lx\n", b, int_reg + 1 ));
            SetPnPReg( int_reg + 1, b, res );
            
            int_reg += 2;
  
            break;
          }
  
          case ISAPNP_NT_DMA_RESOURCE:
          {
            struct ISAPNP_DMAResource* r = (struct ISAPNP_DMAResource*) resource;
            int                        b;
        
            for( b = 0; b < 8; ++b )
            {
              if( r->isapnpdmar_ChannelMask & ( 1 << b ) )
              {
		D(bug( "Programmed dma channel %ld in %lx\n", b, dma_reg ));
                SetPnPReg( dma_reg, b, res );
                break;
              }
            }
  
            dma_reg += 1;
  
            break;
          }
  
          case ISAPNP_NT_IO_RESOURCE:
          {
            struct ISAPNP_IOResource* r = (struct ISAPNP_IOResource*) resource;
  
	    D(bug( "Programmed IO base %04lx in %lx\n", r->isapnpior_MinBase, io_reg ));
  
            SetPnPReg( io_reg, r->isapnpior_MinBase >> 8, res );
            SetPnPReg( io_reg + 1, r->isapnpior_MinBase & 0xff, res );
  
            io_reg += 2;
  
            break;
          }
          
          default:
            bug( "Unsupported resource!\n" );
            return FALSE;
        }
      }
  
      // Activate the device
      D(bug( "Activated the device\n" ));
      SetPnPReg( PNPISA_REG_ACTIVATE, 1, res );
    }
  }

  // Move all cards to the wfk state

  D(bug( "Moved cards to wfk\n" ));

  SetPnPReg( PNPISA_REG_CONFIG_CONTROL,
             PNPISA_CCF_WAIT_FOR_KEY,
             res );

  return TRUE;
}


AROS_LH0(BOOL, ISAPNP_ConfigureCards,
         struct ISAPNPBase *, res, 27, ISAPNP)
{
  AROS_LIBFUNC_INIT

  BOOL rc = FALSE;

  struct ISAPNP_Device* dev;

  dev = ISAPNP_FindDevice( NULL, -1, -1, -1, res );

  if( dev != NULL )
  {
    struct ResourceContext* ctx;
      
    ctx = AllocResourceIteratorContext();
      
    if( ctx != NULL )
    {
      if( ! FindConfiguration( dev, ctx, res ) )
      {
          D(bug( "Unable to find a usable configuration.\n" ));
      }
      else
      {
        if( ! ProgramConfiguration( res ) )
        {
            D(bug( "Failed to program configuration!\n" ));
        }
        else
        {
          rc = TRUE;
        }
      }
        
      FreeResourceIteratorContext( ctx );
    }
  }

  return rc;

  AROS_LIBFUNC_EXIT
}
