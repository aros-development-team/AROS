/*
    Copyright © 2004-2014, Davy Wentzler. All rights reserved.
    Copyright © 2010-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>

#if !defined(__AROS__)
#undef __USE_INLINE__
#include <proto/expansion.h>
#endif

#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

#include "library.h"
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

#define VENDOR_ID 0x1412
#define DEVICE_ID 0x1712
#define CARD_STRING "Envy24"
#define MAX_DEVICE_VENDORS 512

struct VendorDevice *vendor_device_list = NULL;
static int vendor_device_list_size = 0;

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* ahisubbase )
{
    struct CardBase  *CardBase = (struct CardBase*) ahisubbase;
    struct PCIDevice    *dev;
    int                 card_no, i;
    struct List		foundCards;
    struct Node         *devTmp;

    DebugPrintF("[Envy24]: %s()\n", __PRETTY_FUNCTION__);

    CardBase->driverdatas = 0;
    CardBase->cards_found = 0;
    AHIsubBase = ahisubbase;

    NewList(&foundCards);

    DOSBase  = (struct DosLibrary *)OpenLibrary( DOSNAME, 37 );

    if( DOSBase == NULL )
    {
        Req( "Envy24: Unable to open 'dos.library' version 37.\n" );
        return FALSE;
    }

    ExpansionBase = OpenLibrary( "expansion.library", 1 );
    if( ExpansionBase == NULL )
    {
        Req( "Envy24: Unable to open 'expansion.library' version 1.\n" );
        return FALSE;
    }

    if (!ahi_pci_init(ahisubbase))
    {
        return FALSE;
    }

    InitSemaphore( &CardBase->semaphore );

    /*** Count cards ***********************************************************/

    vendor_device_list = (struct VendorDevice *) AllocVec(sizeof(struct VendorDevice) * MAX_DEVICE_VENDORS, MEMF_PUBLIC | MEMF_CLEAR);

    vendor_device_list[0].vendor = VENDOR_ID;
    vendor_device_list[0].device = DEVICE_ID;
    vendor_device_list_size++;

    DebugPrintF("vendor_device_list_size = %ld\n", vendor_device_list_size);    

    CardBase->cards_found = 0;
    dev = NULL;

    for (i = 0; i < vendor_device_list_size; i++)
    {
        dev = ahi_pci_find_device(vendor_device_list[i].vendor, vendor_device_list[i].device, dev);
        
        if (dev != NULL)
        {
            DebugPrintF("[Envy24] %s: Found Envy24 #%d [%4x:%4x] pci obj @ 0x%p\n", __PRETTY_FUNCTION__, i, vendor_device_list[i].vendor, vendor_device_list[i].device, dev);
            ++CardBase->cards_found;

            devTmp = AllocVec(sizeof(struct Node), MEMF_CLEAR);
            devTmp->ln_Name = (APTR)dev;
            AddTail(&foundCards, devTmp);
        }
    }

    // Fail if no hardware is present (prevents the audio modes from being added to
    // the database if the driver cannot be used).

    if(CardBase->cards_found == 0 )
    {
        DebugPrintF("No Envy24 found! :-(\n");
#if defined(VERBOSE_REQ)
        Req( "No card present.\n" );
#endif
        return FALSE;
    }

    /*** Allocate and init all cards *******************************************/

    CardBase->driverdatas = AllocVec( sizeof( *CardBase->driverdatas ) *
                   CardBase->cards_found,
                   MEMF_PUBLIC | MEMF_CLEAR);

    if( CardBase->driverdatas == NULL )
    {
        Req( "Out of memory." );
        return FALSE;
    }

    card_no = 0;

    struct Node *scratchNode;
    ForeachNodeSafe(&foundCards, devTmp, scratchNode)
    {
        Remove(devTmp);

        dev = (struct PCIDevice *)devTmp->ln_Name;
        DebugPrintF("[Envy24] %s: Preparing card #%d pci obj @ 0x%p\n", __PRETTY_FUNCTION__, card_no, dev);
        CardBase->driverdatas[ card_no ] = AllocDriverData( dev, AHIsubBase );
        
        FreeVec(devTmp);
        ++card_no;
    }

    DebugPrintF("[Envy24] %s: Done.\n", __PRETTY_FUNCTION__);

    return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  int i;

    DebugPrintF("[Envy24]: %s()\n", __PRETTY_FUNCTION__);

  for( i = 0; i < CardBase->cards_found; ++i )
  {
    FreeDriverData( CardBase->driverdatas[ i ], AHIsubBase );
  }

  FreeVec( CardBase->driverdatas ); 

  ahi_pci_exit();

  if (ExpansionBase)
    CloseLibrary( (struct Library*) ExpansionBase);

  if (UtilityBase)
    CloseLibrary( (struct Library*) UtilityBase);

  if (DOSBase)
    CloseLibrary( (struct Library*) DOSBase);
}

