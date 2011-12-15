/* $Id$ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     Copyright (C) 2009 The AROS Development Team
     
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

#include <aros/asmcall.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <resources/isapnp.h>

#include "isapnp_private.h"
#include "pnp_structs.h"

#define TEMPLATE "SHOWCONFIG/S,SHOWOPTIONS/S" /*,REMOVE/S"*/

/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

struct DosLibrary*     DOSBase       = NULL;
struct ExecBase*       SysBase       = NULL;

static void
ShowCards( BOOL               show_options, 
           struct ISAPNPBase* res );

static BOOL
OpenLibs( void );

static void
CloseLibs( void );

int
ResourceEntry( void );

/******************************************************************************
** Startup trampoline *********************************************************
******************************************************************************/

AROS_UFH3(__startup static int, Start,
	  AROS_UFHA(char *, argstr, A0),
	  AROS_UFHA(ULONG, argsize, D0),
	  AROS_UFHA(struct ExecBase *, sBase, A6))
{
    AROS_USERFUNC_INIT
	
    SysBase = sBase;
    return ResourceEntry();

    AROS_USERFUNC_EXIT
}

/******************************************************************************
** Resource entry *************************************************************
******************************************************************************/

int
ResourceEntry( void )
{
  struct ISAPNPBase* ISAPNPBase;
  struct RDArgs*     rdargs;
  int                rc = -1;

  struct 
  {
    IPTR   m_ShowConfig;
    IPTR   m_ShowOptions;
/*  IPTR   m_Remove;*/
  } args = { FALSE, FALSE/*, FALSE*/ };

  if( ! OpenLibs() )
  {
    CloseLibs();
    return RETURN_FAIL;
  }

  ISAPNPBase = (struct ISAPNPBase* ) OpenResource( ISAPNPNAME );

  if( ISAPNPBase == NULL )
  {
    Printf( ISAPNPNAME " not found.\n" );
    CloseLibs();
    return RETURN_FAIL;
  }

  rdargs = ReadArgs(TEMPLATE, (IPTR *)&args, NULL);

  if( rdargs != NULL )
  {
    if( ! args.m_ShowConfig && args.m_ShowOptions )
    {
      Printf( "SHOWOPTIONS can only be used together with SHOWCONFIG\n" );
      rc = RETURN_ERROR;
    }
    else
    {
      if( args.m_ShowConfig )
      {
        ShowCards( args.m_ShowOptions, ISAPNPBase );
        rc = RETURN_OK;
      }
      
/*    if( args.m_Remove )
      {
        // Dangerous! Only for debugging

        FreeISAPNPBase( ISAPNPBase );

        ISAPNPBase->m_ConfigDev->cd_Flags  |= CDF_CONFIGME;
        ISAPNPBase->m_ConfigDev->cd_Driver  = NULL;
        RemResource( ISAPNPBase );

        rc = RETURN_OK;
      }*/
    }

    FreeArgs( rdargs );
  }

  if( rc == -1 )
  {
    Printf( "Usage: ISA-PnP [ SHOWCONFIG [ SHOWOPTIONS ] ] [ REMOVE ]\n" );
    rc = RETURN_ERROR;
  }
  

  CloseLibs();

  return rc;
}


/******************************************************************************
** Prints information about all cards on a serial port terminal ***************
******************************************************************************/

static void
ShowResource( struct ISAPNP_Resource* resource,
              struct ISAPNPBase*      res )
{
  switch( resource->isapnpr_Type )
  {
    case ISAPNP_NT_IRQ_RESOURCE:
    {
      struct ISAPNP_IRQResource* r = (struct ISAPNP_IRQResource*) resource;
      int                        b;
      
      Printf( "IRQ" );
      
      for( b = 0; b < 16; ++b )
      {
        if( r->isapnpirqr_IRQMask & ( 1 << b ) )
        {
          Printf( " %ld", b );
        }
      }
      
      Printf( ", type" );
      
      if( r->isapnpirqr_IRQType & ISAPNP_IRQRESOURCE_ITF_HIGH_EDGE )
      {
        Printf( " +E" );
      }

      if( r->isapnpirqr_IRQType & ISAPNP_IRQRESOURCE_ITF_LOW_EDGE )
      {
        Printf( " -E" );
      }

      if( r->isapnpirqr_IRQType & ISAPNP_IRQRESOURCE_ITF_HIGH_LEVEL )
      {
        Printf( " +L" );
      }

      if( r->isapnpirqr_IRQType & ISAPNP_IRQRESOURCE_ITF_LOW_LEVEL )
      {
        Printf( " -L" );
      }
      
      Printf( "\n" );
      break;
    }

    case ISAPNP_NT_DMA_RESOURCE:
    {
      struct ISAPNP_DMAResource* r = (struct ISAPNP_DMAResource*) resource;
      int                        b;
      
      Printf( "DMA" );
      
      for( b = 0; b < 8; ++b )
      {
        if( r->isapnpdmar_ChannelMask & ( 1 << b ) )
        {
          Printf( " %ld", b );
        }
      }

      Printf( ", " );
      switch( r->isapnpdmar_Flags & ISAPNP_DMARESOURCE_F_TRANSFER_MASK )
      {
        case ISAPNP_DMARESOURCE_F_TRANSFER_8BIT:
          Printf( "8" );
          break;

        case ISAPNP_DMARESOURCE_F_TRANSFER_BOTH:
          Printf( "8 and 16" );
          break;
          
        case ISAPNP_DMARESOURCE_F_TRANSFER_16BIT:
          Printf( "16" );
          break;
        
      }
      Printf( " bit transfer, " );
      
      switch( r->isapnpdmar_Flags & ISAPNP_DMARESOURCE_F_SPEED_MASK )
      {
        case ISAPNP_DMARESOURCE_F_SPEED_COMPATIBLE:
          Printf( "compatible" );
          break;

        case ISAPNP_DMARESOURCE_F_SPEED_TYPE_A:
          Printf( "type A" );
          break;

        case ISAPNP_DMARESOURCE_F_SPEED_TYPE_B:
          Printf( "type B" );
          break;

        case ISAPNP_DMARESOURCE_F_SPEED_TYPE_F:     
          Printf( "type F" );
          break;
      }
      
      Printf( " speed." );

      if( r->isapnpdmar_Flags & ISAPNP_DMARESOURCE_FF_BUS_MASTER )
      {
        Printf( " [Bus master]" );
      }

      if( r->isapnpdmar_Flags & ISAPNP_DMARESOURCE_FF_BYTE_MODE )
      {
        Printf( " [Byte mode]" );
      }

      if( r->isapnpdmar_Flags & ISAPNP_DMARESOURCE_FF_WORD_MODE )
      {
        Printf( " [Word mode]" );
      }

      Printf( "\n" );

      break;
    }


    case ISAPNP_NT_IO_RESOURCE:
    {
      struct ISAPNP_IOResource* r = (struct ISAPNP_IOResource*) resource;

      if( r->isapnpior_MinBase == r->isapnpior_MaxBase )
      {
        Printf( "IO at 0x%04lx, length 0x%02lx.",
                 r->isapnpior_MinBase, r->isapnpior_Length );
      }
      else
      {
        Printf( "IO between 0x%04lx and 0x%04lx, length 0x%02lx, %ld byte aligned.",
                 r->isapnpior_MinBase, r->isapnpior_MaxBase, r->isapnpior_Length, r->isapnpior_Alignment );
      }
           
      if( ( r->isapnpior_Flags & ISAPNP_IORESOURCE_FF_FULL_DECODE ) == 0 )
      {
        Printf( " [10 bit decode only]" );
      }

      Printf( "\n" );
      break;
    }


    case ISAPNP_NT_MEMORY_RESOURCE:
      Printf( "Memory\n" );
      break;

    default:
      Printf( "Unknown resource!" );
      break;
  }
}

static void
ShowResourceGroup( struct ISAPNP_ResourceGroup* resource_group,
                   struct ISAPNPBase* res )
{
  struct ISAPNP_Resource*      r;
  struct ISAPNP_ResourceGroup* rg;

  for( r = (struct ISAPNP_Resource*) resource_group->isapnprg_Resources.mlh_Head;
       r->isapnpr_MinNode.mln_Succ != NULL;
       r = (struct ISAPNP_Resource*) r->isapnpr_MinNode.mln_Succ )
  {
    Printf( "      " );
    ShowResource( r, res );
  }

  if( resource_group->isapnprg_ResourceGroups.mlh_Head->mln_Succ != NULL )  
  {
    Printf( "    One of\n" );

    for( rg = (struct ISAPNP_ResourceGroup*) resource_group->isapnprg_ResourceGroups.mlh_Head;
         rg->isapnprg_MinNode.mln_Succ != NULL;
         rg = (struct ISAPNP_ResourceGroup*) rg->isapnprg_MinNode.mln_Succ )
    {
      Printf( "    {\n" );
      ShowResourceGroup( rg, res );
      Printf( "    }\n" );
    }
  }
}


static void
ShowCards( BOOL               show_options, 
           struct ISAPNPBase* res )
{
  struct ISAPNP_Card* card;

  for( card = (struct ISAPNP_Card*) res->m_Cards.lh_Head; 
       card->isapnpc_Node.ln_Succ != NULL; 
       card = (struct ISAPNP_Card*) card->isapnpc_Node.ln_Succ )
  {
    struct ISAPNP_Device* dev;

    Printf( "Card %ld: %s%03lx%lx/%ld ('%s')\n",
             card->isapnpc_CSN, 
             (ULONG) card->isapnpc_ID.isapnpid_Vendor, 
             card->isapnpc_ID.isapnpid_ProductID, 
             card->isapnpc_ID.isapnpid_Revision,
             card->isapnpc_SerialNumber,
             card->isapnpc_Node.ln_Name != NULL ? (ULONG) card->isapnpc_Node.ln_Name 
                                                : (ULONG) "" );

    Printf( "  PnP version: %ld.%ld\n",
            card->isapnpc_MajorPnPVersion,
            card->isapnpc_MinorPnPVersion );

    Printf( "  Vendor specific version number: %ld.%ld\n", 
            card->isapnpc_VendorPnPVersion >> 4, 
            card->isapnpc_VendorPnPVersion & 0x0f );

    for( dev = (struct ISAPNP_Device*) card->isapnpc_Devices.lh_Head;
         dev->isapnpd_Node.ln_Succ != NULL; 
         dev = (struct ISAPNP_Device*) dev->isapnpd_Node.ln_Succ )
    {
      struct ISAPNP_Identifier* id;
      struct ISAPNP_Resource*   r;

      Printf( "  Logical device %ld: ",
               dev->isapnpd_DeviceNumber );

      for( id = (struct ISAPNP_Identifier*) dev->isapnpd_IDs.mlh_Head;
           id->isapnpid_MinNode.mln_Succ != NULL;
           id = (struct ISAPNP_Identifier*) id->isapnpid_MinNode.mln_Succ )
      {
        Printf( "%s%03lx%lx ",
                (ULONG) id->isapnpid_Vendor, 
                id->isapnpid_ProductID, 
                id->isapnpid_Revision );
      }

      if( dev->isapnpd_Node.ln_Name != NULL )
      {
        Printf( "('%s')", (ULONG) dev->isapnpd_Node.ln_Name );
      }

      Printf( "\n" );

      if( dev->isapnpd_SupportedCommands & ISAPNP_DEVICE_SCF_BOOTABLE )
      {
        Printf( "    Device is capable of participating in the boot process.\n" );
      }

      if( dev->isapnpd_SupportedCommands & ISAPNP_DEVICE_SCF_RANGE_CHECK )
      {
        Printf( "    Device supports IO range checking.\n" );
      }

      Printf( "    Allocated resources:\n" );

      if( dev->isapnpd_Resources.mlh_Head->mln_Succ != NULL )
      {
        for( r = (struct ISAPNP_Resource*) dev->isapnpd_Resources.mlh_Head;
             r->isapnpr_MinNode.mln_Succ != NULL;
             r = (struct ISAPNP_Resource*) r->isapnpr_MinNode.mln_Succ )
        {
          Printf( "      " );
          ShowResource( r, res );
        }
      }
      else
      {
        Printf( "      None.\n" );
      }

      if( show_options )
      {
        Printf( "    Requested resources:\n" );
        ShowResourceGroup( dev->isapnpd_Options, res );
      }
    }
  }
}

/******************************************************************************
** OpenLibs *******************************************************************
******************************************************************************/

BOOL
OpenLibs( void )
{
  /* DOS Library */

  DOSBase = (struct DosLibrary*) OpenLibrary( "dos.library", 37 );

  if( DOSBase == NULL )
  {
    return FALSE;
  }

  return TRUE;
}


/******************************************************************************
** CloseLibs *******************************************************************
******************************************************************************/

void
CloseLibs( void )
{
  CloseLibrary( (struct Library*) DOSBase );

  DOSBase       = NULL;
}

