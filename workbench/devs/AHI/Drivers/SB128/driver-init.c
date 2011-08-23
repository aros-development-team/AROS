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

#include <config.h>

#include <exec/memory.h>

#undef __USE_INLINE__
#include <proto/expansion.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <devices/timer.h>

#include "library_card.h"
#include "version.h"
#include "misc.h"
#include "regs.h"
#include "DriverData.h"


struct Library*    SysBase;
struct Library*    DOSBase;
struct DriverBase* AHIsubBase;

struct Library*             ExpansionBase = NULL;
struct ExpansionIFace*      IExpansion = NULL;
struct UtilityIFace*        IUtility = NULL;
struct AHIsubIFace*         IAHIsub = NULL;
struct MMUIFace*            IMMU = NULL;
struct PCIIFace*            IPCI = NULL;

#define CARD_STRING "SB128"


/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* ahisubbase )
{
  struct CardBase* CardBase = (struct CardBase*) ahisubbase;
  struct PCIDevice   *dev;
  int                 card_no;

  UWORD cards[] =
  {
    0x1274, 0x5000,
    0x1274, 0x1371,
    0x1274, 0x5880,
    0x1102, 0x8938,
    PCI_ANY_ID, PCI_ANY_ID
  };

  CardBase->cards_found = 0;
  CardBase->driverdatas = 0;
  AHIsubBase = ahisubbase;
  
  DOSBase  = IExec->OpenLibrary( DOSNAME, 37 );

  if( DOSBase == NULL )
  {
    Req( "Unable to open 'dos.library' version 37.\n" );
    return FALSE;
  }

  if ((IDOS = (struct DOSIFace *) IExec->GetInterface((struct Library *) DOSBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open IDOS interface!\n");
       return FALSE;
  }

  ExpansionBase = IExec->OpenLibrary( "expansion.library", 50 );
  if( ExpansionBase == NULL )
  {
    Req( "Unable to open 'expansion.library' version 50.\n" );
    return FALSE;
  }
  if ((IExpansion = (struct ExpansionIFace *) IExec->GetInterface((struct Library *) ExpansionBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open IExpansion interface!\n");
       return FALSE;
  }

  if ((IPCI = (struct PCIIFace *) IExec->GetInterface((struct Library *) ExpansionBase, "pci", 1, NULL)) == NULL)
  {
       Req("Couldn't open IPCI interface!\n");
       return FALSE;
  }
  
  if ((IAHIsub = (struct AHIsubIFace *) IExec->GetInterface((struct Library *) AHIsubBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open IAHIsub interface!\n");
       return FALSE;
  }
  
  if ((IUtility = (struct UtilityIFace *) IExec->GetInterface((struct Library *) UtilityBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open IUtility interface!\n");
       return FALSE;
  }

  if ((IMMU = (struct MMUIFace *) IExec->GetInterface((struct Library *) SysBase, "mmu", 1, NULL)) == NULL)
  {
       Req("Couldn't open IMMU interface!\n");
       return FALSE;
  }

  /* Timer Device */
  
/*  replymp = (struct MsgPort *) IExec->CreatePort(NULL, 0);
  if (!replymp)
  {
    IExec->DebugPrintF("SB128: Couldn't create ReplyPort!\n");
    return FALSE;
  }*/

  /*TimerIO = (struct timerequest *)IExec->CreateIORequest(replymp, sizeof(struct timerequest));

  if (TimerIO == NULL)
  {
    IExec->DebugPrintF("Out of memory.\n");
    return FALSE;
  }
  
  if (IExec->OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)TimerIO, 0) != 0)
  {
    IExec->DebugPrintF("Unable to open 'timer.device'.\n");
    return FALSE;
  }
  else
    TimerBase = (struct Device *)TimerIO->tr_node.io_Device;*/
  
  IExec->InitSemaphore( &CardBase->semaphore );


  /*** Count cards ***********************************************************/

  dev = NULL;

  if ( (dev = IPCI->FindDeviceTags( FDT_CandidateList, cards,
				  TAG_DONE ) ) != NULL )
  {
    ++CardBase->cards_found;
    IExec->DebugPrintF("%s found! :-)\n", CARD_STRING);
  }
  
  /* Fail if no hardware is present (prevents the audio modes from being added to
     the database if the driver cannot be used). */

  if( CardBase->cards_found == 0 )
  {
    IExec->DebugPrintF("No %s found! :-(\n", CARD_STRING);
    Req( "No card present.\n" );
    return FALSE;
  }

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

  CardBase->driverdatas = IExec->AllocVec( sizeof( *CardBase->driverdatas ) *
				       CardBase->cards_found,
				       MEMF_PUBLIC );

  if( CardBase->driverdatas == NULL )
  {
    Req( "Out of memory." );
    return FALSE;
  }

  card_no = 0;

  if( ( dev = IPCI->FindDeviceTags( FDT_CandidateList, cards, 
				  TAG_DONE ) ) != NULL )
  {
    CardBase->driverdatas[ card_no ] = AllocDriverData( dev, AHIsubBase );
    ++card_no;    
  }

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
#if 0
  if( CardBase->camd.Semaphore.ss_Link.ln_Name != NULL )
  {
    IExec->ObtainSemaphore( &CardBase->camd.Semaphore );
    IExec->RemSemaphore( &CardBase->camd.Semaphore );
    IExec->ReleaseSemaphore( &CardBase->camd.Semaphore );
  }
#endif
  
  for( i = 0; i < CardBase->cards_found; ++i )
  {
    if (CardBase->driverdatas)
    {
      CardBase->driverdatas[ i ]->pci_dev->OutLong(CardBase->driverdatas[ i ]->iobase + SB128_SCON, 0); 
      FreeDriverData( CardBase->driverdatas[ i ], AHIsubBase );
    }
  }

  if (CardBase->driverdatas)
    IExec->FreeVec( CardBase->driverdatas );
  
  /*if (TimerIO)
  {
    IExec->CloseDevice((struct IORequest *)TimerIO);
    IExec->DeleteIORequest((struct IORequest *)TimerIO);
  }*/
  
  /*if (replymp)
    IExec->DeletePort(replymp);*/
 
  if (IMMU)
    IExec->DropInterface( (struct Interface *) IMMU);
  
  if (IUtility) 
    IExec->DropInterface( (struct Interface *) IUtility);
  
  if (IExpansion)
    IExec->DropInterface( (struct Interface *) IExpansion);

  if (IPCI)
    IExec->DropInterface( (struct Interface *) IPCI);

  if (IAHIsub)
    IExec->DropInterface( (struct Interface *) IAHIsub);

  if (IDOS)
    IExec->DropInterface( (struct Interface *) IDOS);

  if (ExpansionBase)
    IExec->CloseLibrary( (struct Library*) ExpansionBase );
  
  if (UtilityBase)
    IExec->CloseLibrary( (struct Library*) UtilityBase );
  
  if (DOSBase)
    IExec->CloseLibrary( (struct Library*) DOSBase );
}
