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

static BOOL
PatchBrokenCards( struct ISAPNPBase* res );

/******************************************************************************
** Resource resident structure ************************************************
******************************************************************************/

extern const char LIBEND;
extern const char ResName[];
extern const char IDString[];
static const APTR InitTable[4];

const struct Resident RomTag =
{
  RTC_MATCHWORD,
  (struct Resident *) &RomTag,
  (struct Resident *) &LIBEND,
  RTF_AUTOINIT|RTF_COLDSTART,
  VERSION,
  NT_RESOURCE,
  12,                      /* priority */
  (BYTE *) ResName,
  (BYTE *) IDString,
  (APTR) &InitTable,
  0,
  0
};


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

struct Device*         TimerBase     = NULL;
/*struct ExecBase*     SysBase       = NULL;*/
/*struct IntuitionBase*  IntuitionBase = NULL;*/
struct ISAPNPBase*     ISAPNPBase    = NULL;
/*struct UtilityBase*    UtilityBase   = NULL;*/

static struct timerequest *TimerIO   = NULL;

const char ResName[]   = ISAPNPNAME;
const char IDString[]  = ISAPNPNAME " " VERS "\r\n";

static const char VersTag[] =
 "$VER: " ISAPNPNAME " " VERS " ©2001 Martin Blom.\r\n";

/******************************************************************************
** Resource initialization ****************************************************
******************************************************************************/

struct ISAPNPBase* ASMCALL
initRoutine( REG( d0, struct ISAPNPBase* res ),
             REG( a0, APTR                    seglist ),
             REG( a6, struct ExecBase*        sysbase ) )
{
    D(bug("[ISAPNP] Init\n"));
    SysBase = sysbase;

    if(OpenLibs() )
    {
        // Set up the ISAPNPBase structure

        res->m_Library.lib_Node.ln_Type = NT_RESOURCE;
        res->m_Library.lib_Node.ln_Name = (STRPTR) ResName;
        res->m_Library.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
        res->m_Library.lib_Version      = VERSION;
        res->m_Library.lib_Revision     = REVISION;
        res->m_Library.lib_IdString     = (STRPTR) IDString;

        NewList( &res->m_Cards );

	/* Base address, on i386 we don't have any mapping
        res->m_Base        = NULL; */
        res->m_RegReadData = 0x0000;

        if( ! ISAPNP_ScanCards( res ) )
        {
              // No cards found

              Req( "No PnP ISA cards found.", res );
              FreeISAPNPBase( res );
        }
        else
        {
            if( ! PatchBrokenCards( res ) )
            {
                FreeISAPNPBase( res );
            }
            else
            {
                struct ISAPNP_Card* card;
  
                card = ISAPNP_AllocCard( res );
  
                if( card == NULL )
                {
                  Req( "Out of memory!", res );
                  FreeISAPNPBase( res );
                }
                else
                {
                  static const char descr[] = "Non-PnP devices";
                  char*             d;                
                  
                  d = AllocVec( sizeof( descr ), MEMF_PUBLIC );
                  
                  if( d != NULL )
                  {
                    CopyMem( (void*) descr, d, sizeof( descr ) );
                    card->isapnpc_Node.ln_Name = d;
                  }
  
                  card->isapnpc_ID.isapnpid_Vendor[ 0 ] = '?';
                  card->isapnpc_ID.isapnpid_Vendor[ 1 ] = '?';
                  card->isapnpc_ID.isapnpid_Vendor[ 2 ] = '?';
                  card->isapnpc_SerialNumber = -1;
  
                  // Add *first*
                  AddHead( &res->m_Cards, (struct Node*) card );
  
                  // Let's see if we're to disable any cards or devices etc
                  if( ! HandleStartArgs( card, res ) )
                  {
                    // Error requester already displayed.
                    FreeISAPNPBase( res );
                  }
                  else
                  {
                    if( ! ISAPNP_ConfigureCards( res ) )
                    {
                      // Unable to configure cards
  
                      Req( "Unable to configure the cards. This is most likely\n"
                           "because of an unresolvable hardware conflict.\n\n"
                           "Use the DISABLE_DEVICE tool type to disable one of\n"
                           "the devices in conflict.", res );
                      FreeISAPNPBase( res );
                    }
                    else
                      ISAPNPBase = res;
                  }
                }
            }
        }
    }

    return ISAPNPBase;

}


/******************************************************************************
** Free all resources from ISAPNPBase *****************************************
******************************************************************************/

void
FreeISAPNPBase( struct ISAPNPBase* res )
{
  struct ISAPNP_Card* card;

  while( ( card = (struct ISAPNP_Card*) RemHead( &res->m_Cards ) ) )
  {
    ISAPNP_FreeCard( card, ISAPNPBase );
  }
}


/******************************************************************************
** Initialization tables ******************************************************
******************************************************************************/

static const APTR funcTable[] =
{
  AROS_SLIB_ENTRY(ISAC_SetMasterInt, ISAPNP, 1),
  AROS_SLIB_ENTRY(ISAC_GetMasterInt, ISAPNP, 2),
  AROS_SLIB_ENTRY(ISAC_SetWaitState, ISAPNP, 3),
  AROS_SLIB_ENTRY(ISAC_GetWaitState, ISAPNP, 4),
  AROS_SLIB_ENTRY(ISAC_GetInterruptStatus, ISAPNP, 5),
  AROS_SLIB_ENTRY(ISAC_GetRegByte, ISAPNP, 6),
  AROS_SLIB_ENTRY(ISAC_SetRegByte, ISAPNP, 7),
  AROS_SLIB_ENTRY(ISAC_GetRegWord, ISAPNP, 8),
  AROS_SLIB_ENTRY(ISAC_SetRegWord, ISAPNP, 9),
  AROS_SLIB_ENTRY(ISAC_GetRegLong, ISAPNP, 10),
  AROS_SLIB_ENTRY(ISAC_SetRegLong, ISAPNP, 11),
  AROS_SLIB_ENTRY(ISAC_ReadByte, ISAPNP, 12),
  AROS_SLIB_ENTRY(ISAC_WriteByte, ISAPNP, 13),
  AROS_SLIB_ENTRY(ISAC_ReadWord, ISAPNP, 14),
  AROS_SLIB_ENTRY(ISAC_WriteWord, ISAPNP, 15),
  AROS_SLIB_ENTRY(ISAC_ReadLong, ISAPNP, 16),
  AROS_SLIB_ENTRY(ISAC_WriteLong, ISAPNP, 17),

  AROS_SLIB_ENTRY(ISAPNP_AllocCard, ISAPNP, 18),
  AROS_SLIB_ENTRY(ISAPNP_FreeCard, ISAPNP, 19),
  AROS_SLIB_ENTRY(ISAPNP_AllocDevice, ISAPNP, 20),
  AROS_SLIB_ENTRY(ISAPNP_FreeDevice, ISAPNP, 21),
  AROS_SLIB_ENTRY(ISAPNP_AllocResourceGroup, ISAPNP, 22),
  AROS_SLIB_ENTRY(ISAPNP_FreeResourceGroup, ISAPNP, 23),
  AROS_SLIB_ENTRY(ISAPNP_AllocResource, ISAPNP, 24),
  AROS_SLIB_ENTRY(ISAPNP_FreeResource, ISAPNP, 25),

  AROS_SLIB_ENTRY(ISAPNP_ScanCards, ISAPNP, 26),
  AROS_SLIB_ENTRY(ISAPNP_ConfigureCards, ISAPNP, 27),

  AROS_SLIB_ENTRY(ISAPNP_FindCard, ISAPNP, 28),
  AROS_SLIB_ENTRY(ISAPNP_FindDevice, ISAPNP, 29),

  AROS_SLIB_ENTRY(ISAPNP_LockCardsA, ISAPNP, 30),
  AROS_SLIB_ENTRY(ISAPNP_UnlockCards, ISAPNP, 31),
  AROS_SLIB_ENTRY(ISAPNP_LockDevicesA, ISAPNP, 32),
  AROS_SLIB_ENTRY(ISAPNP_UnlockDevices, ISAPNP, 33),

  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) sizeof( struct ISAPNPBase ),
  (APTR) &funcTable,
  0,
  (APTR) initRoutine
};

/******************************************************************************
** OpenLibs *******************************************************************
******************************************************************************/

BOOL
OpenLibs( void )
{
  /* Utility Library (libnix depends on it, and our startup-code is not
     executed when BindDriver LoadSeg()s us!) */

  /* Intuition Library */
/*
  IntuitionBase  = (struct IntuitionBase*) OpenLibrary( "intuition.library", 37 );

  if( IntuitionBase == NULL )
  {
    return FALSE;
  }

  UtilityBase = (struct UtilityBase *) OpenLibrary( "utility.library", 37 );

  if( UtilityBase == NULL)
  {
    return FALSE;
  }
*/
  /* Timer Device */

  TimerIO = (struct timerequest *) AllocVec( sizeof(struct timerequest),
                                             MEMF_PUBLIC | MEMF_CLEAR );

  if( TimerIO == NULL)
  {
    return FALSE;
  }

  if( OpenDevice( "timer.device",
                  UNIT_MICROHZ,
                  (struct IORequest *)
                  TimerIO,
                  0) != 0 )
  {
    return FALSE;
  }

  TimerBase = (struct Device *) TimerIO->tr_node.io_Device;

  return TRUE;
}


/******************************************************************************
** CloseLibs *******************************************************************
******************************************************************************/

void
CloseLibs( void )
{
  if( TimerIO  != NULL )
  {
    CloseDevice( (struct IORequest *) TimerIO );
  }

  FreeVec( TimerIO );
/*
  CloseLibrary( (struct Library*) UtilityBase );
  CloseLibrary( (struct Library*) IntuitionBase );
*/
  TimerIO       = NULL;
  TimerBase     = NULL;
/*IntuitionBase = NULL;
  UtilityBase   = NULL;*/
}

/******************************************************************************
** Fix cards that have broken PnP ROMs ****************************************
******************************************************************************/

static BOOL
PatchBrokenCards( struct ISAPNPBase* res )
{
  struct ISAPNP_Device* dev = NULL;

  // Patch the wavetable device on SB AWE32 and AWE64

  while( ( dev = ISAPNP_FindDevice( dev,
                                    ISAPNP_MAKE_ID('C','T','L'),
                                    0x002,
                                    1,
                                    res ) ) != NULL )
  {
    struct ISAPNP_ResourceGroup* rg;
    struct ISAPNP_IOResource*    r1;
    struct ISAPNP_IOResource*    r2;
    struct ISAPNP_IOResource*    r3;

    // Nuke all dependent options

    while( ( rg = (struct ISAPNP_ResourceGroup*) 
                  RemHead( (struct List*) &dev->isapnpd_Options->isapnprg_ResourceGroups ) )
           != NULL )
    {
      ISAPNP_FreeResourceGroup( rg, res );
    }

    rg = ISAPNP_AllocResourceGroup( ISAPNP_RG_PRI_ACCEPTABLE, res );

    r1 = (struct ISAPNP_IOResource*) ISAPNP_AllocResource( ISAPNP_NT_IO_RESOURCE, res );
    r2 = (struct ISAPNP_IOResource*) ISAPNP_AllocResource( ISAPNP_NT_IO_RESOURCE, res );
    r3 = (struct ISAPNP_IOResource*) ISAPNP_AllocResource( ISAPNP_NT_IO_RESOURCE, res );

    if( rg == NULL || r1 == NULL || r2 == NULL || r3 == NULL )
    {
      ISAPNP_FreeResourceGroup( rg, res );
      ISAPNP_FreeResource( (struct ISAPNP_Resource*) r1, res );
      ISAPNP_FreeResource( (struct ISAPNP_Resource*) r2, res );
      ISAPNP_FreeResource( (struct ISAPNP_Resource*) r3, res );

      return FALSE;
    }

    r1->isapnpior_Flags = ISAPNP_IORESOURCE_FF_FULL_DECODE;    
    r2->isapnpior_Flags = ISAPNP_IORESOURCE_FF_FULL_DECODE;    
    r3->isapnpior_Flags = ISAPNP_IORESOURCE_FF_FULL_DECODE;    

    r1->isapnpior_Alignment = 1;
    r2->isapnpior_Alignment = 1;
    r3->isapnpior_Alignment = 1;

    r1->isapnpior_Length = 4;
    r2->isapnpior_Length = 4;
    r3->isapnpior_Length = 4;

    r1->isapnpior_MinBase = 0x620;
    r2->isapnpior_MinBase = 0xa20;
    r3->isapnpior_MinBase = 0xe20;

    r1->isapnpior_MaxBase = 0x620;
    r2->isapnpior_MaxBase = 0xa20;
    r3->isapnpior_MaxBase = 0xe20;

    AddTail( (struct List*) &rg->isapnprg_Resources, (struct Node*) r1 );
    AddTail( (struct List*) &rg->isapnprg_Resources, (struct Node*) r2 );
    AddTail( (struct List*) &rg->isapnprg_Resources, (struct Node*) r3 );
    
    AddTail( (struct List*) &dev->isapnpd_Options->isapnprg_ResourceGroups, 
             (struct Node*) rg );
  }
  
  return TRUE;
}
