/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <proto/expansion.h>
#include <proto/exec.h>
#include <proto/dos.h>

#ifdef __amigaos4__
#include "library_card.h"
struct Library*   SysBase;
#elif __MORPHOS__
struct ExecBase*   SysBase;
#include "library_mos.h"
#else
struct DosLibrary* DOSBase;
#include "library.h"
#endif
#include "version.h"
#include "misc.h"
#include "regs.h"
#include "DriverData.h"


struct DriverBase* AHIsubBase;

#include "pci_wrapper.h"



//
struct Library*             ExpansionBase = NULL;

#ifdef __amigaos4__
struct ExpansionIFace*      IExpansion = NULL;
struct UtilityIFace*        IUtility = NULL;
struct AHIsubIFace*         IAHIsub = NULL;
struct PCIIFace*            IPCI = NULL;
struct MMUIFace*            IMMU          = NULL;
#endif

#define VENDOR_ID 0x1412
#define DEVICE_ID 0x1724
#define CARD_STRING "Envy24HT"

#ifdef __AROS__
#include <proto/stdc.h>
struct StdCBase *StdCBase = NULL;
#endif

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL DriverInit( struct DriverBase* ahisubbase )
{
    struct CardBase* CardBase = (struct CardBase*) ahisubbase;
    struct PCIDevice   *dev;
    int                 card_no;

    DEBUGPRINTF("\n%s DriverInit\n", CARD_STRING);

    AHIsubBase = ahisubbase;

#ifdef __AROS__
    StdCBase = (struct StdCBase *)OpenLibrary((CONST_STRPTR)"stdc.library", 0);
    if (!StdCBase)
    {
        return FALSE;
    }
#endif

    if (!ahi_pci_init(ahisubbase))
    {
        return FALSE;
    }

    /*** Count cards ***********************************************************/

    CardBase->cards_found = 0;
    dev = NULL;

    if ( (dev = ahi_pci_find_device(VENDOR_ID, DEVICE_ID, 0) ))
    {
        ++CardBase->cards_found;
        DEBUGPRINTF("%s found! :-)\n", CARD_STRING);
    }

    // Fail if no hardware is present (prevents the audio modes from being added to
    // the database if the driver cannot be used).

    if( CardBase->cards_found == 0 )
    {
        DEBUGPRINTF("No %s found! :-(\n", CARD_STRING);
        //Req( "No card present.\n" );
        return FALSE;
    }

    /*if (dev->Lock(EXCLUSIVE_LOCK) == FALSE)
    {
         DEBUGPRINTF("Envy24HT: Couldn't lock the device\n");
         return FALSE;
    }*/

  /*** CAMD ******************************************************************/
#if 0
  IExec->InitSemaphore( &CardBase->camd.Semaphore );
  CardBase->camd.Semaphore.ss_Link.ln_Pri  = 0;

  CardBase->camd.Semaphore.ss_Link.ln_Name = Card_CAMD_SEMAPHORE;
  IExec->AddSemaphore( &CardBase->camd.Semaphore );
  
  CardBase->camd.Cards    = CardBase->cards_found;
  CardBase->camd.Version  = VERSION;
  CardBase->camd.Revision = REVISION;


  CardBase->camd.OpenPortFunc.h_Entry    = OpenCAMDPort;
  CardBase->camd.OpenPortFunc.h_SubEntry = NULL;
  CardBase->camd.OpenPortFunc.h_Data     = NULL;

  CardBase->camd.ClosePortFunc.h_Entry    = (HOOKFUNC) CloseCAMDPort;
  CardBase->camd.ClosePortFunc.h_SubEntry = NULL;
  CardBase->camd.ClosePortFunc.h_Data     = NULL;

  CardBase->camd.ActivateXmitFunc.h_Entry    = (HOOKFUNC) ActivateCAMDXmit;
  CardBase->camd.ActivateXmitFunc.h_SubEntry = NULL;
  CardBase->camd.ActivateXmitFunc.h_Data     = NULL;
#endif
  

    /*** Allocate and init all cards *******************************************/

    CardBase->driverdatas = ALLOCVEC(sizeof(*CardBase->driverdatas) *
			                         CardBase->cards_found,
			                         MEMF_PUBLIC);

    if( CardBase->driverdatas == NULL )
    {
        Req( "Out of memory." );
        return FALSE;
    }

    card_no = 0;

    if( ( dev = ahi_pci_find_device(VENDOR_ID, DEVICE_ID, 0) ) != NULL )
    {
        CardBase->driverdatas[ card_no ] = AllocDriverData( dev, AHIsubBase );
        ++card_no;
    }


    //DEBUGPRINTF("Envy24HT exit\n");
    return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct CardBase* CardBase = (struct CardBase*) AHIsubBase;
  struct CardData *card;
  int i;
#if 0
  if( CardBase->camd.Semaphore.ss_Link.ln_Name != NULL )
  {
    OBTAINSEMAPHORE( &CardBase->camd.Semaphore );
    IExec->RemSemaphore( &CardBase->camd.Semaphore );
    RELEASESEMAPHORE( &CardBase->camd.Semaphore );
  }
#endif
  for( i = 0; i < CardBase->cards_found; ++i )
  {
    card = CardBase->driverdatas[ i ];
    OUTWORD(card->iobase + CCS_INTR_MASK, ~0);
    //CardBase->driverdatas[i]->pci_dev->Unlock();
    FreeDriverData( card, AHIsubBase );
  }

  FREEVEC( CardBase->driverdatas ); 

  ahi_pci_exit();

#ifdef __AROS__
  if (StdCBase)
  {
    CloseLibrary((struct Library *)StdCBase);
    StdCBase = NULL;
  }
#endif
}
