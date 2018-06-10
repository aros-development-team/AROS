
#include <aros/debug.h>
#include <config.h>

#include "library.h"
#include "DriverData.h"

#include "alsa-bridge/alsa.h"

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* AHIsubBase )
{
  struct AlsaBase* AlsaBase = (struct AlsaBase*) AHIsubBase;

  D(bug("[Alsa]: DriverInit()\n"));

  AlsaBase->dosbase = (struct DosLibrary *)OpenLibrary( DOSNAME, 37 );

  if( AlsaBase->dosbase == NULL )
  {
    Req( "Unable to open 'dos.library' version 37.\n" );
    return FALSE;
  }

  if (!ALSA_Init())
      return FALSE;

  ALSA_MixerInit(&AlsaBase->al_MixerHandle, &AlsaBase->al_MixerElem,
          &AlsaBase->al_MinVolume, &AlsaBase->al_MaxVolume);

  D(bug("[Alsa]: DriverInit() completed\n"));

  return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct AlsaBase* AlsaBase = (struct AlsaBase*) AHIsubBase;

  if (AlsaBase->al_MixerHandle)
    ALSA_MixerCleanup(AlsaBase->al_MixerHandle);

  ALSA_Cleanup();

  CloseLibrary( (struct Library*) DOSBase );
}
