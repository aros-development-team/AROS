/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy of the License at 
http://www.aros.org/license.html 
Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF 
ANY KIND, either express or implied. See the License for the specific language governing rights and 
limitations under the License. 

The Original Code is written by Davy Wentzler.
*/

//#include <config.h>

#include <exec/memory.h>

#if !defined(__AROS__)
#undef __USE_INLINE__
#include <proto/expansion.h>
#endif

#include <proto/exec.h>
#include <proto/dos.h>

#ifdef __AROS__
#include <aros/debug.h>
#define DebugPrintF bug
#endif

#include "library.h"
#include "version.h"
#include "misc.h"
#include "regs.h"

#include "pci_wrapper.h"

struct DriverBase* AHIsubBase;

struct DosLibrary* DOSBase;
struct Library*             ExpansionBase = NULL;

struct VendorDevice
{
    UWORD vendor;
    UWORD device;
};

#define VENDOR_ID 0x13F6
#define DEVICE_ID 0x0111
#define CARD_STRING "CMI8738"
#define MAX_DEVICE_VENDORS 512

struct VendorDevice *vendor_device_list = NULL;
static int vendor_device_list_size = 0;

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* ahisubbase )
{
  struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) ahisubbase;
  struct PCIDevice   *dev;
  int                 card_no, i;

  //DebugPrintF("DRIVERINIT\n");

  CMI8738Base->driverdatas = 0;
  CMI8738Base->cards_found = 0;
  AHIsubBase = ahisubbase;

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

  if (!ahi_pci_init(ahisubbase))
  {
    return FALSE;
  }

  InitSemaphore( &CMI8738Base->semaphore );

  /*** Count cards ***********************************************************/

  vendor_device_list = (struct VendorDevice *) AllocVec(sizeof(struct VendorDevice) * MAX_DEVICE_VENDORS, MEMF_PUBLIC | MEMF_CLEAR);

  vendor_device_list[0].vendor = VENDOR_ID;
  vendor_device_list[0].device = DEVICE_ID;
  vendor_device_list_size++;

  bug("vendor_device_list_size = %ld\n", vendor_device_list_size);    

  CMI8738Base->cards_found = 0;
  dev = NULL;

  for (i = 0; i < vendor_device_list_size; i++)
  {
    dev = ahi_pci_find_device(vendor_device_list[i].vendor, vendor_device_list[i].device, dev);
        
    if (dev != NULL)
    {
      bug("Found CMI8738 #%d [%4x:%4x]\n", i, vendor_device_list[i].vendor, vendor_device_list[i].device);
      ++CMI8738Base->cards_found;
      break; // stop at first found controller
    }
  }

  // Fail if no hardware is present (prevents the audio modes from being added to
  // the database if the driver cannot be used).

  if(CMI8738Base->cards_found == 0 )
  {
    DebugPrintF("No CMI8738 found! :-(\n");
    Req( "No card present.\n" );
    return FALSE;
  }

  /*** CAMD ******************************************************************/
#if 0
  InitSemaphore( &CMI8738Base->camd.Semaphore );
  CMI8738Base->camd.Semaphore.ss_Link.ln_Pri  = 0;

  CMI8738Base->camd.Semaphore.ss_Link.ln_Name = Card_CAMD_SEMAPHORE;
  AddSemaphore( &CMI8738Base->camd.Semaphore );
  
  CMI8738Base->camd.Cards    = CMI8738Base->cards_found;
  CMI8738Base->camd.Version  = VERSION;
  CMI8738Base->camd.Revision = REVISION;


  CMI8738Base->camd.OpenPortFunc.h_Entry    = OpenCAMDPort;
  CMI8738Base->camd.OpenPortFunc.h_SubEntry = NULL;
  CMI8738Base->camd.OpenPortFunc.h_Data     = NULL;

  CMI8738Base->camd.ClosePortFunc.h_Entry    = (HOOKFUNC) CloseCAMDPort;
  CMI8738Base->camd.ClosePortFunc.h_SubEntry = NULL;
  CMI8738Base->camd.ClosePortFunc.h_Data     = NULL;

  CMI8738Base->camd.ActivateXmitFunc.h_Entry    = (HOOKFUNC) ActivateCAMDXmit;
  CMI8738Base->camd.ActivateXmitFunc.h_SubEntry = NULL;
  CMI8738Base->camd.ActivateXmitFunc.h_Data     = NULL;
#endif

  /*** Allocate and init all cards *******************************************/

  CMI8738Base->driverdatas = AllocVec( sizeof( *CMI8738Base->driverdatas ) *
				       CMI8738Base->cards_found,
				       MEMF_PUBLIC );

  if( CMI8738Base->driverdatas == NULL )
  {
    Req( "Out of memory." );
    return FALSE;
  }

  card_no = 0;

  if(dev)
  {
    CMI8738Base->driverdatas[ card_no ] = AllocDriverData( dev, AHIsubBase );
    if (CMI8738Base->driverdatas[card_no] == NULL)
    {
	return FALSE;
    }
    ++card_no;
  }

  //DebugPrintF("exit init\n");
  return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct CMI8738Base* CMI8738Base = (struct CMI8738Base*) AHIsubBase;
  int i;
  
  for( i = 0; i < CMI8738Base->cards_found; ++i )
  {
    FreeDriverData( CMI8738Base->driverdatas[ i ], AHIsubBase );
  }

  FreeVec( CMI8738Base->driverdatas ); 

  ahi_pci_exit();

  if (ExpansionBase)
    CloseLibrary( (struct Library*) ExpansionBase);

  if (UtilityBase)
    CloseLibrary( (struct Library*) UtilityBase);

  if (DOSBase)
    CloseLibrary( (struct Library*) DOSBase);
}

