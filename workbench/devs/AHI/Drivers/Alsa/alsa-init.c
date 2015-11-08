
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

  ALSA_Cleanup();

  CloseLibrary( (struct Library*) DOSBase );
}
