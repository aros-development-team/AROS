/*

The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

The Original Code is (C) Copyright 2004-2011 Ross Vumbaca.

The Initial Developer of the Original Code is Ross Vumbaca.

All Rights Reserved.

*/

#include <aros/debug.h>

#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

#include "library.h"
#include "misc.h"
#include "regs.h"

#include "pci_wrapper.h"

struct DosLibrary* DOSBase;
struct DriverBase* AHIsubBase;
struct Library*             ExpansionBase = NULL;
#ifdef __AROS__
struct Library *StdCBase = NULL;
#endif

struct VendorDevice
{
    UWORD vendor;
    UWORD device;
};

#define CARD_STRING "SB128"
#define MAX_DEVICE_VENDORS 512

struct VendorDevice *vendor_device_list = NULL;
static int vendor_device_list_size = 0;

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* ahisubbase )
{
  struct SB128Base* SB128Base = (struct SB128Base*) ahisubbase;
  struct PCIDevice   *dev;
    int                 card_no, i;
    struct List		foundCards;
    struct Node         *devTmp;

    D(bug("[SB128]: %s()\n", __PRETTY_FUNCTION__));

  SB128Base->cards_found = 0;
  SB128Base->driverdatas = 0;
  AHIsubBase = ahisubbase;

    NewList(&foundCards);

  DOSBase  = OpenLibrary( DOSNAME, 37 );

  if( DOSBase == NULL )
  {
    Req( "Unable to open 'dos.library' version 37.\n" );
    return FALSE;
  }

  ExpansionBase = OpenLibrary( "expansion.library", 1 );
  if( ExpansionBase == NULL )
  {
    Req( "Unable to open 'expansion.library' version 1.\n" );
    return FALSE;
  }

#ifdef __AROS__
  StdCBase = OpenLibrary( "stdc.library", 0 );
  if( StdCBase == NULL)
  {
    Req( "Unable to open 'stdc.library'.\n" );
    return FALSE;
  }
#endif

    if (!ahi_pci_init(ahisubbase))
    {
        return FALSE;
    }

  InitSemaphore( &SB128Base->semaphore );

    /*** Count cards ***********************************************************/

    vendor_device_list = (struct VendorDevice *) AllocVec(sizeof(struct VendorDevice) * MAX_DEVICE_VENDORS, MEMF_PUBLIC | MEMF_CLEAR);

    vendor_device_list[0].vendor = 0x1274;
    vendor_device_list[0].device = 0x5000;
    vendor_device_list_size++;

    vendor_device_list[1].vendor = 0x1274;
    vendor_device_list[1].device = 0x1371;
    vendor_device_list_size++;
    
    vendor_device_list[2].vendor = 0x1274;
    vendor_device_list[2].device = 0x5880;
    vendor_device_list_size++;
    
    vendor_device_list[3].vendor = 0x1102;
    vendor_device_list[3].device = 0x8938;
    vendor_device_list_size++;

    D(bug("vendor_device_list_size = %ld\n", vendor_device_list_size));

    SB128Base->cards_found = 0;
    dev = NULL;

    for (i = 0; i < vendor_device_list_size; i++)
    {
        dev = ahi_pci_find_device(vendor_device_list[i].vendor, vendor_device_list[i].device, dev);
        
        if (dev != NULL)
        {
            D(bug("[SB128] %s: Found SB128 #%d [%4x:%4x] pci obj @ 0x%p\n", __PRETTY_FUNCTION__, i, vendor_device_list[i].vendor, vendor_device_list[i].device, dev));
            ++SB128Base->cards_found;

            devTmp = AllocVec(sizeof(struct Node), MEMF_CLEAR);
            devTmp->ln_Name = dev;
            AddTail(&foundCards, devTmp);
        }
    }

    // Fail if no hardware is present (prevents the audio modes from being added to
    // the database if the driver cannot be used).

    if(SB128Base->cards_found == 0 )
    {
        D(bug("[SB128] %s: No SB128 found! :-(\n", __PRETTY_FUNCTION__));
#if defined(VERBOSE_REQ)
        Req( "No card present.\n" );
#endif
        return FALSE;
    }

  /*** CAMD ******************************************************************/
#if 0
  InitSemaphore( &SB128Base->camd.Semaphore );
  SB128Base->camd.Semaphore.ss_Link.ln_Pri  = 0;

  SB128Base->camd.Semaphore.ss_Link.ln_Name = Card_CAMD_SEMAPHORE;
  AddSemaphore( &SB128Base->camd.Semaphore );
  
  SB128Base->camd.Cards    = SB128Base->cards_found;
  SB128Base->camd.Version  = VERSION;
  SB128Base->camd.Revision = REVISION;


  SB128Base->camd.OpenPortFunc.h_Entry    = OpenCAMDPort;
  SB128Base->camd.OpenPortFunc.h_SubEntry = NULL;
  SB128Base->camd.OpenPortFunc.h_Data     = NULL;

  SB128Base->camd.ClosePortFunc.h_Entry    = (HOOKFUNC) CloseCAMDPort;
  SB128Base->camd.ClosePortFunc.h_SubEntry = NULL;
  SB128Base->camd.ClosePortFunc.h_Data     = NULL;

  SB128Base->camd.ActivateXmitFunc.h_Entry    = (HOOKFUNC) ActivateCAMDXmit;
  SB128Base->camd.ActivateXmitFunc.h_SubEntry = NULL;
  SB128Base->camd.ActivateXmitFunc.h_Data     = NULL;
#endif
  

  /*** Allocate and init all cards *******************************************/

  SB128Base->driverdatas = AllocVec( sizeof( *SB128Base->driverdatas ) *
				       SB128Base->cards_found,
				       MEMF_PUBLIC );

  if( SB128Base->driverdatas == NULL )
  {
    Req( "Out of memory." );
    return FALSE;
  }

  card_no = 0;

    struct Node *scratchNode;
    ForeachNodeSafe(&foundCards, devTmp, scratchNode)
    {
        Remove(devTmp);

        dev = devTmp->ln_Name;
        D(bug("[SB128] %s: Preparing card #%d pci obj @ 0x%p\n", __PRETTY_FUNCTION__, card_no, dev));
        SB128Base->driverdatas[ card_no ] = AllocDriverData( dev, AHIsubBase );
        
        FreeVec(devTmp);
        ++card_no;
    }

    D(bug("[SB128] %s: Done.\n", __PRETTY_FUNCTION__));

  return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct SB128Base* SB128Base = (struct SB128Base*) AHIsubBase;
  int i;
#if 0
  if( SB128Base->camd.Semaphore.ss_Link.ln_Name != NULL )
  {
    ObtainSemaphore( &SB128Base->camd.Semaphore );
    RemSemaphore( &SB128Base->camd.Semaphore );
    ReleaseSemaphore( &SB128Base->camd.Semaphore );
  }
#endif
  
  for( i = 0; i < SB128Base->cards_found; ++i )
  {
    if (SB128Base->driverdatas)
    {
      pci_outl( 0, SB128_SCON, SB128Base->driverdatas[i] );
      FreeDriverData( SB128Base->driverdatas[ i ], AHIsubBase );
    }
  }

  if (SB128Base->driverdatas)
    FreeVec( SB128Base->driverdatas );
  
  /*if (TimerIO)
  {
    CloseDevice((struct IORequest *)TimerIO);
    DeleteIORequest((struct IORequest *)TimerIO);
  }*/
  
  /*if (replymp)
    DeletePort(replymp);*/

  ahi_pci_exit();

#ifdef __AROS__
  if (StdCBase)
      CloseLibrary( StdCBase );
#endif

  if (ExpansionBase)
    CloseLibrary( (struct Library*) ExpansionBase );
  
  if (UtilityBase)
    CloseLibrary( (struct Library*) UtilityBase );
  
  if (DOSBase)
    CloseLibrary( (struct Library*) DOSBase );
}
