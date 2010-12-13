/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2005 Martin Blom <martin@blom.org>

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>


#include "library.h"
#include "version.h"
#include "emu10kx-misc.h"
#include "pci_wrapper.h"

/* We use global library bases instead of storing them in DriverBase, since
   I don't want to modify the original sources more than necessary. */

struct DriverBase* AHIsubBase = NULL;
struct ExecBase*   SysBase    = NULL;
struct DosLibrary* DOSBase    = NULL;

#ifdef __AMIGAOS4__
struct DOSIFace*   IDOS       = NULL;
struct ExecIFace*  IExec      = NULL;
struct MMUIFace*   IMMU       = NULL;
#endif


#include "8010.h"




/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* ahisubbase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) ahisubbase;
  APTR	    	      dev;
  int                 card_no;

  /*** Libraries etc. ********************************************************/

  AHIsubBase = ahisubbase;
  
  DOSBase  = (struct DosLibrary*) OpenLibrary( DOSNAME, 37 );

  if( DOSBase == NULL )
  {
    Req( "Unable to open 'dos.library' version 37.\n" );
    return FALSE;
  }

#ifdef __AMIGAOS4__
  if ((IDOS = (struct DOSIFace *) GetInterface((struct Library *) DOSBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open IDOS interface!\n");
       return FALSE;
  }

/*   if ((IAHIsub = (struct AHIsubIFace *) GetInterface((struct Library *) AHIsubBase, "main", 1, NULL)) == NULL) */
/*   { */
/*        Req("Couldn't open IAHIsub interface!\n"); */
/*        return FALSE; */
/*   } */
  
  if ((IMMU = (struct MMUIFace *) GetInterface((struct Library *) SysBase, "mmu", 1, NULL)) == NULL)
  {
       Req("Couldn't open IMMU interface!\n");
       return FALSE;
  }

/*   if ((IUtility = (struct UtilityIFace *) GetInterface((struct Library *) UtilityBase, "main", 1, NULL)) == NULL) */
/*   { */
/*        Req("Couldn't open IUtility interface!\n"); */
/*        return FALSE; */
/*   } */
#endif

  if (!ahi_pci_init(ahisubbase))
  {
       return FALSE;
  }
  
  InitSemaphore( &EMU10kxBase->semaphore );


  /*** Count cards ***********************************************************/

  EMU10kxBase->cards_found = 0;
  dev = NULL;

  // Search for Live! cards
  while( ( dev = ahi_pci_find_device( PCI_VENDOR_ID_CREATIVE,
				      PCI_DEVICE_ID_CREATIVE_EMU10K1,
				      dev ) ) != NULL )
  {
    ++EMU10kxBase->cards_found;
  }
  
  // Search for Audigy cards
  while( ( dev = ahi_pci_find_device( PCI_VENDOR_ID_CREATIVE,
				      PCI_DEVICE_ID_CREATIVE_AUDIGY,
				      dev ) ) != NULL )
  {
    ++EMU10kxBase->cards_found;
  }
  
  // Fail if no hardware (prevents the audio modes form being added to
  // the database if the driver cannot be used).

  if( EMU10kxBase->cards_found == 0 )
  {
    return FALSE;
  }

  /*** CAMD ******************************************************************/
  
  InitSemaphore( &EMU10kxBase->camd.Semaphore );
  EMU10kxBase->camd.Semaphore.ss_Link.ln_Pri  = 0;
  EMU10kxBase->camd.Semaphore.ss_Link.ln_Name = EMU10KX_CAMD_SEMAPHORE;
  
  EMU10kxBase->camd.Cards    = EMU10kxBase->cards_found;
  EMU10kxBase->camd.Version  = VERSION;
  EMU10kxBase->camd.Revision = REVISION;

#ifdef __AMIGAOS4__
  EMU10kxBase->camd.OpenPortFunc.h_Entry    = OpenCAMDPort;
  EMU10kxBase->camd.OpenPortFunc.h_SubEntry = NULL;
  EMU10kxBase->camd.OpenPortFunc.h_Data     = NULL;

  EMU10kxBase->camd.ClosePortFunc.h_Entry    = (HOOKFUNC) CloseCAMDPort;
  EMU10kxBase->camd.ClosePortFunc.h_SubEntry = NULL;
  EMU10kxBase->camd.ClosePortFunc.h_Data     = NULL;

  EMU10kxBase->camd.ActivateXmitFunc.h_Entry    = (HOOKFUNC) ActivateCAMDXmit;
  EMU10kxBase->camd.ActivateXmitFunc.h_SubEntry = NULL;
  EMU10kxBase->camd.ActivateXmitFunc.h_Data     = NULL;
  
#else
  
  EMU10kxBase->camd.OpenPortFunc.h_Entry    = HookEntry;
  EMU10kxBase->camd.OpenPortFunc.h_SubEntry = OpenCAMDPort;
  EMU10kxBase->camd.OpenPortFunc.h_Data     = NULL;

  EMU10kxBase->camd.ClosePortFunc.h_Entry    = HookEntry;
  EMU10kxBase->camd.ClosePortFunc.h_SubEntry = (HOOKFUNC) CloseCAMDPort;
  EMU10kxBase->camd.ClosePortFunc.h_Data     = NULL;

  EMU10kxBase->camd.ActivateXmitFunc.h_Entry    = HookEntry;
  EMU10kxBase->camd.ActivateXmitFunc.h_SubEntry = (HOOKFUNC) ActivateCAMDXmit;
  EMU10kxBase->camd.ActivateXmitFunc.h_Data     = NULL;
#endif
  
  AddSemaphore( &EMU10kxBase->camd.Semaphore );

  /*** AC97 Mixer ************************************************************/
  
  InitSemaphore( &EMU10kxBase->ac97.Semaphore );
  EMU10kxBase->ac97.Semaphore.ss_Link.ln_Pri  = 0;
  EMU10kxBase->ac97.Semaphore.ss_Link.ln_Name = EMU10KX_AC97_SEMAPHORE;
  
  EMU10kxBase->ac97.Cards    = EMU10kxBase->cards_found;
  EMU10kxBase->ac97.Version  = VERSION;
  EMU10kxBase->ac97.Revision = REVISION;

#ifdef __AMIGAOS4__
  EMU10kxBase->ac97.GetFunc.h_Entry    = AC97GetFunc;
  EMU10kxBase->ac97.GetFunc.h_SubEntry = NULL;
  EMU10kxBase->ac97.GetFunc.h_Data     = NULL;

  EMU10kxBase->ac97.SetFunc.h_Entry    = (HOOKFUNC) AC97SetFunc;
  EMU10kxBase->ac97.SetFunc.h_SubEntry = NULL;
  EMU10kxBase->ac97.SetFunc.h_Data     = NULL;

#else

  EMU10kxBase->ac97.GetFunc.h_Entry    = HookEntry;
  EMU10kxBase->ac97.GetFunc.h_SubEntry = AC97GetFunc;
  EMU10kxBase->ac97.GetFunc.h_Data     = NULL;

  EMU10kxBase->ac97.SetFunc.h_Entry    = HookEntry;
  EMU10kxBase->ac97.SetFunc.h_SubEntry = (HOOKFUNC) AC97SetFunc;
  EMU10kxBase->ac97.SetFunc.h_Data     = NULL;
#endif

  AddSemaphore( &EMU10kxBase->ac97.Semaphore );
  
  /*** Allocate and init all cards *******************************************/

  EMU10kxBase->driverdatas = AllocVec( sizeof( *EMU10kxBase->driverdatas ) *
				       EMU10kxBase->cards_found,
				       MEMF_PUBLIC );

  if( EMU10kxBase->driverdatas == NULL )
  {
    Req( "Out of memory." );
    return FALSE;
  }

  card_no = 0;
  
  // Live! cards ... 
  while( ( dev = ahi_pci_find_device( PCI_VENDOR_ID_CREATIVE,
				      PCI_DEVICE_ID_CREATIVE_EMU10K1,
				      dev ) ) != NULL )
  {
    // AOS4: dev->Lock(PCI_LOCK_EXCLUSIVE); tbd
    EMU10kxBase->driverdatas[ card_no ] = AllocDriverData( dev, AHIsubBase );
    ++card_no;
  }
  
  // Audigy cards ...
  while( ( dev = ahi_pci_find_device( PCI_VENDOR_ID_CREATIVE,
				      PCI_DEVICE_ID_CREATIVE_AUDIGY,
				      dev ) ) != NULL )
  {
    // AOS4: dev->Lock(PCI_LOCK_EXCLUSIVE); tbd
    EMU10kxBase->driverdatas[ card_no ] = AllocDriverData( dev, AHIsubBase );
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
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;
  int i;

  if( EMU10kxBase->camd.Semaphore.ss_Link.ln_Name != NULL )
  {
    ObtainSemaphore( &EMU10kxBase->camd.Semaphore );
    RemSemaphore( &EMU10kxBase->camd.Semaphore );
    ReleaseSemaphore( &EMU10kxBase->camd.Semaphore );
  }

  if( EMU10kxBase->ac97.Semaphore.ss_Link.ln_Name != NULL )
  {
    ObtainSemaphore( &EMU10kxBase->ac97.Semaphore );
    RemSemaphore( &EMU10kxBase->ac97.Semaphore );
    ReleaseSemaphore( &EMU10kxBase->ac97.Semaphore );
  }

  for( i = 0; i < EMU10kxBase->cards_found; ++i )
  {
    // Kill'em all
    emu10k1_irq_disable( &EMU10kxBase->driverdatas[ i ]->card, ~0UL );
//			 INTE_MIDIRXENABLE | INTE_MIDITXENABLE);
    
    FreeDriverData( EMU10kxBase->driverdatas[ i ], AHIsubBase );
  }

  FreeVec( EMU10kxBase->driverdatas ); 
    
#ifdef __AMIGAOS4__
  DropInterface((struct Interface *) IDOS);
  DropInterface((struct Interface *) IMMU);
#endif

  ahi_pci_exit();

  CloseLibrary( (struct Library*) DOSBase );
}
