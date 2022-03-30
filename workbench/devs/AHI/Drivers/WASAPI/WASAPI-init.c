
#include <aros/debug.h>
#include <config.h>

#include "library.h"
#include "DriverData.h"

#include "WASAPI-bridge/WASAPI.h"

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* AHIsubBase )
{
  struct WASAPIBase* WASAPIBase = (struct WASAPIBase*) AHIsubBase;

  D(bug("[WASAPI] %s()\n", __func__));

  WASAPIBase->dosbase = (struct DosLibrary *)OpenLibrary( DOSNAME, 37 );

  if( WASAPIBase->dosbase == NULL )
  {
    Req( "Unable to open 'dos.library' version 37.\n" );
    return FALSE;
  }

  if (!WASAPI_Init())
      return FALSE;

  WASAPI_MixerInit(&WASAPIBase->al_MixerHandle, &WASAPIBase->al_MixerVolCtrl,
          &WASAPIBase->al_MinVolume, &WASAPIBase->al_MaxVolume);

  D(bug("[WASAPI] %s: completed\n", __func__));

  return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct WASAPIBase* WASAPIBase = (struct WASAPIBase*) AHIsubBase;

  if (WASAPIBase->al_MixerHandle)
    WASAPI_MixerCleanup(WASAPIBase->al_MixerHandle);

  WASAPI_Cleanup();

  CloseLibrary( (struct Library*) DOSBase );
}
