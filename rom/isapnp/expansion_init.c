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

#include <aros/libcall.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <devices/timer.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
/*#include <proto/utility.h>*/

#include <stdlib.h>

#include <resources/isapnp.h>
#include "isapnp_private.h"
#include "version.h"

#include "controller.h"
#include "devices.h"
#include "init.h"
#include "pnp.h"
#include "pnp_structs.h"


void
ReqA( const char* text, APTR args );

#define IntReq( text, args...) \
        ( { ULONG _args[] = { args }; ReqA( (text), (APTR) _args ); } )

/******************************************************************************
** ReqA ***********************************************************************
******************************************************************************/

void
ReqA( const char* text, APTR args, struct ISAPNPBase *res )
{
  struct EasyStruct es = 
  {
    sizeof (struct EasyStruct),
    0,
    (STRPTR) ISAPNPNAME " " VERS,
    (STRPTR) text,
    "OK"
  };
  struct IntuitionBase *IntuitionBase;

  IntuitionBase = (struct IntuitionBase *)OpenLibrary("intution.library", 36);
  if( IntuitionBase != NULL )
  {
    EasyRequestArgs( NULL, &es, NULL, args );
    CloseLibrary(&IntuitionBase->LibNode);
  }
}

void Req(const char *text, struct ISAPNPBase *res)
{
    ReqA(text, NULL, res);
}

/******************************************************************************
** Handle the tool types ******************************************************
******************************************************************************/

static int
HexToInt( UBYTE c )
{
  if( c >= '0' && c <= '9' )
  {
    return c - '0';
  }
  else if( c >= 'A' && c <= 'F' )
  {
    return c - 'A' + 10;
  }
  else if( c >= 'a' && c <= 'f' )
  {
    return c - 'a' + 10;
  }
  else
  {
    return -1;
  }
}

// CTL0048        => "CTL\0" 4 8 -1   -1
// CTL0048/1236   => "CTL\0" 4 8 1236 -1
// CTL0048:0      => "CTL\0" 4 8 -1   0
// CTL0048/1236:0 => "CTL\0" 4 8 1236 0

static int
ParseID( UBYTE* string,
         LONG*  manufacturer,
         WORD*  product,
         BYTE*  revision,
         LONG*  serial,
         WORD*  logical_device )
{
  int  chars = 0;
  LONG ser   = -1;
  LONG dev   = -1;

  *manufacturer = ISAPNP_MAKE_ID( ToUpper( string[ 0 ] ),
                                  ToUpper( string[ 1 ] ),
                                  ToUpper( string[ 2 ] ) );

  *product      = ( HexToInt( string[ 3 ] ) << 8 ) |
                  ( HexToInt( string[ 4 ] ) << 4 ) |
                  ( HexToInt( string[ 5 ] ) );


  if( *product == -1 )
  {
    return 0;
  }

  *revision = HexToInt( string[ 6 ] );

  if( *revision == -1 )
  {
    return 0;
  }

  chars = 7;

  if( string[ chars ] == '/' )
  {
    int conv;
      
    if( serial == NULL )
    {
      // Not allowed if we don't ask for it
      return NULL;
    }

    conv = StrToLong( string + chars + 1, &ser );

    if( conv == -1 )
    {
      return 0;
    }
    else
    {
      chars += conv + 1;
    }
  
  }

  if( serial != NULL )
  {
    *serial = ser;
  }

  if( string[ chars ] == ':' )
  {
    int conv;
   
    if( logical_device == NULL )
    {
      // Not allowed if we don't ask for it
      return NULL;
    }

    conv = StrToLong( string + chars + 1, &dev );

    if( conv == -1 )
    {
      return 0;
    }
    else
    {
      chars += conv + 1;
    }
  
  }

  if( logical_device != NULL )
  {
    *logical_device = dev;
  }

  if( string[ chars ] != 0 && string[ chars ] != ' ' )
  {
    return 0;
  }

  return chars;
}


BOOL
HandleStartArgs( struct ISAPNP_Card* card,
                 struct ISAPNPBase*  res )
{
  UBYTE **tool_types = current_binding.cb_ToolTypes;

  while( *tool_types )
  {
    if( Strnicmp( *tool_types, "DISABLE_CARD=", 13 ) == 0 )
    {
      LONG manufacturer;
      WORD product;
      BYTE revision;
      LONG serial;

      if( ParseID( *tool_types + 13, 
                   &manufacturer, &product, &revision, &serial, NULL ) )
      {
        struct ISAPNP_Card* card = NULL;

        while( ( card = ISAPNP_FindCard( card,
                                         manufacturer,
                                         product,
                                         revision,
                                         serial,
                                         res ) ) != NULL )
        {
          card->isapnpc_Disabled = TRUE;
        }
      }
      else
      {
        Req( "Illegal tool type: %s\n", (ULONG) *tool_types );
        return FALSE;
      }
    }
    else if( Strnicmp( *tool_types, "DISABLE_DEVICE=", 15 ) == 0 )
    {
      LONG manufacturer;
      WORD product;
      BYTE revision;
      LONG serial;
      WORD log_dev;

      if( ParseID( *tool_types + 15, 
                   &manufacturer, &product, &revision, &serial, &log_dev ) )
      {
        if( log_dev == -1 )
        {
          struct ISAPNP_Device* dev = NULL;

          while( ( dev = ISAPNP_FindDevice( dev,
                                            manufacturer,
                                            product,
                                            revision,
                                            res ) ) != NULL )
          {
            dev->isapnpd_Disabled = TRUE;
          }
        }
        else
        {
          struct ISAPNP_Card* card = NULL;

          while( ( card = ISAPNP_FindCard( card,
                                           manufacturer,
                                           product,
                                           revision,
                                           serial,
                                           res ) ) != NULL )
          {
            struct ISAPNP_Device* dev;
            
            for( dev = (struct ISAPNP_Device*) card->isapnpc_Devices.lh_Head;
                 dev->isapnpd_Node.ln_Succ != NULL;
                 dev = (struct ISAPNP_Device*) dev->isapnpd_Node.ln_Succ )
            {
              if( dev->isapnpd_DeviceNumber == (UWORD) log_dev )
              {
                dev->isapnpd_Disabled = TRUE;
              }
            }
          }
        }
      }
      else
      {
        Req( "Illegal tool type value: %s\n", (ULONG) *tool_types );
        return FALSE;
      }
    }
    else if( Strnicmp( *tool_types, "LEGACY_DEVICE=", 14 ) == 0 )
    {
      UBYTE* str;
      int    conv;
      LONG   manufacturer;
      WORD   product;
      BYTE   revision;
      UWORD  dev_num = 0;

      str  = *tool_types + 14;
      conv = ParseID( str,  &manufacturer, &product, &revision, NULL, NULL );

      str += conv;

      if( conv != 0 )
      {
        struct ISAPNP_Device*     dev;
        struct ISAPNP_Identifier* id;

        dev = ISAPNP_AllocDevice( res );

        if( dev == NULL )
        {
          Req( "Out of memory!" );
          return FALSE;
        }

        dev->isapnpd_Card = card;
        
        id  = AllocVec( sizeof( *id ), MEMF_PUBLIC | MEMF_CLEAR );
        
        if( id == NULL )
        {
          Req( "Out of memory!" );
          ISAPNP_FreeDevice( dev, res );
          return FALSE;
        }

        id->isapnpid_Vendor[ 0 ]  = ( manufacturer >> 24 ) & 0xff;
        id->isapnpid_Vendor[ 1 ]  = ( manufacturer >> 16 ) & 0xff;
        id->isapnpid_Vendor[ 2 ]  = ( manufacturer >>  8 ) & 0xff;
        id->isapnpid_Vendor[ 3 ]  = 0;

        id->isapnpid_ProductID    = product;
        id->isapnpid_Revision     = revision;
        
        AddTail( (struct List*) &dev->isapnpd_IDs, (struct Node*) id );
        
        if( card->isapnpc_Devices.lh_Head->ln_Succ != NULL )
        {
          dev_num = ( (struct ISAPNP_Device*) 
                      card->isapnpc_Devices.lh_TailPred )->isapnpd_DeviceNumber;
          ++dev_num;
        }
        
        dev->isapnpd_DeviceNumber = dev_num;

        AddTail( &card->isapnpc_Devices, (struct Node*) dev );
        
        while( *str != 0 )
        {
          if( *str != ' ' )
          {
            if( Strnicmp( str, "IRQ=", 4 ) == 0 )
            {
              int irq;
            
              irq = strtol( str + 4, (char**) &str, 0 );
            
              if( irq <= 0 || irq >= 16 )
              {
                Req( "Invalid IRQ value '%ld' in tooltype line\n"
                     "'%s'",
                     irq,
                     (ULONG) *tool_types );
                return FALSE;
              }
              else
              {
                struct ISAPNP_IRQResource* r;
        
                r = (struct ISAPNP_IRQResource*) 
                    ISAPNP_AllocResource( ISAPNP_NT_IRQ_RESOURCE, res );
            
                if( r == NULL )
                {
                  Req( "Out of memory!" );
                  return FALSE;
                }

                r->isapnpirqr_IRQMask = 1 << irq;
                r->isapnpirqr_IRQType = ISAPNP_IRQRESOURCE_ITF_HIGH_EDGE;
          
                AddTail( (struct List*) &dev->isapnpd_Options->isapnprg_Resources,
                         (struct Node*) r );
              }
            }
            else if( Strnicmp( str, "DMA=", 4 ) == 0 )
            {
              int dma;
            
              dma = strtol( str + 4, (char**) &str, 0 );
            
              if( dma <= 0 || dma >= 8 )
              {
                Req( "Invalid DMA value '%ld' in tooltype line\n"
                     "'%s'",
                     dma,
                     (ULONG) *tool_types );
                return FALSE;
              }
              else
              {
                struct ISAPNP_DMAResource* r;
        
                r = (struct ISAPNP_DMAResource*) 
                    ISAPNP_AllocResource( ISAPNP_NT_DMA_RESOURCE, res );
            
                if( r == NULL )
                {
                  Req( "Out of memory!" );
                  return FALSE;
                }

                r->isapnpdmar_ChannelMask = 1 << dma;
                r->isapnpdmar_Flags       = 0;
          
                AddTail( (struct List*) &dev->isapnpd_Options->isapnprg_Resources,
                         (struct Node*) r );
              }
            }
            else if( Strnicmp( str, "IO=", 3 ) == 0 )
            {
              int base;
              int length;

              struct ISAPNP_IOResource* r;
            
              base = strtol( str + 3, (char**) &str, 0 );

              if( *str != '/' )
              {
                Req( "Length missing from IO value in tooltype line\n"
                     "'%s'",
                     (ULONG) *tool_types );
                return FALSE;
              }

              ++str;

              length = strtol( str, (char**) &str, 0 );

              if( base <= 0 || base >= 0xffff )
              {
                Req( "Invalid IO base value '%ld' in tooltype line\n"
                     "'%s'",
                     base,
                     (ULONG) *tool_types );
                return FALSE;
              }

              if( length <= 0 || length >= 0xffff )
              {
                Req( "Invalid IO length value '%ld' in tooltype line\n"
                     "'%s'",
                     length,
                     (ULONG) *tool_types );
                return FALSE;
              }

              r = (struct ISAPNP_IOResource*) 
                  ISAPNP_AllocResource( ISAPNP_NT_IO_RESOURCE, res );
            
              if( r == NULL )
              {
                Req( "Out of memory!" );
                return FALSE;
              }

              r->isapnpior_MinBase   = base;
              r->isapnpior_MaxBase   = base;
              r->isapnpior_Length    = length;
              r->isapnpior_Alignment = 1;
          
              AddTail( (struct List*) &dev->isapnpd_Options->isapnprg_Resources,
                       (struct Node*) r );
            }
            else
            {
              Req( "Parse error near '%s'\n"
                   "in tooltype line\n"
                   "'%s'",
                   (ULONG) str,
                   (ULONG) *tool_types );
              return FALSE;
            }
          }
          
          if( *str )
          {
            ++str;
          }
        }
      }
      else
      {
        Req( "Illegal tool type: '%s'\n", (ULONG) *tool_types );
        return FALSE;
      }
    }
    else
    {
      // Ignore unknown tool types
    }

    ++tool_types;
  }

  return TRUE;
}
