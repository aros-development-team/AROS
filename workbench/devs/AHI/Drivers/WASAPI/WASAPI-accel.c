
#include <aros/debug.h>

#include <devices/ahi.h>
#include <libraries/ahi_sub.h>

#include "DriverData.h"

/******************************************************************************
** AHIsub_SetVol **************************************************************
******************************************************************************/

ULONG
_AHIsub_SetVol( UWORD                   channel,
        Fixed                   volume,
        sposition               pan,
        struct AHIAudioCtrlDrv* AudioCtrl,
        ULONG                   flags,
        struct DriverBase*      AHIsubBase )
{
#if (0)
  struct WASAPIBase* WASAPIBase = (struct WASAPIBase*) AHIsubBase;
#endif
  D(bug("[WASAPI] %s(%d)\n", __func__, volume));

  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_SetFreq *************************************************************
******************************************************************************/

ULONG
_AHIsub_SetFreq( UWORD                   channel,
         ULONG                   freq,
         struct AHIAudioCtrlDrv* AudioCtrl,
         ULONG                   flags,
         struct DriverBase*      AHIsubBase )
{
#if (0)
  struct WASAPIBase* WASAPIBase = (struct WASAPIBase*) AHIsubBase;
#endif
  D(bug("[WASAPI] %s(%d)\n", __func__, freq));

  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_SetSound ************************************************************
******************************************************************************/

ULONG
_AHIsub_SetSound( UWORD                   channel,
          UWORD                   sound,
          ULONG                   offset,
          LONG                    length,
          struct AHIAudioCtrlDrv* AudioCtrl,
          ULONG                   flags,
          struct DriverBase*      AHIsubBase )
{
  D(bug("[WASAPI] %s()\n", __func__));

  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_SetEffect ***********************************************************
******************************************************************************/

ULONG
_AHIsub_SetEffect( APTR                    effect,
           struct AHIAudioCtrlDrv* AudioCtrl,
           struct DriverBase*      AHIsubBase )
{
  D(bug("[WASAPI] %s()\n", __func__));

  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_LoadSound ***********************************************************
******************************************************************************/

ULONG
_AHIsub_LoadSound( UWORD                   sound,
           ULONG                   type,
           APTR                    info,
           struct AHIAudioCtrlDrv* AudioCtrl,
           struct DriverBase*      AHIsubBase )
{
  D(bug("[WASAPI] %s()\n", __func__));

  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_UnloadSound *********************************************************
******************************************************************************/

ULONG
_AHIsub_UnloadSound( UWORD                   sound,
             struct AHIAudioCtrlDrv* AudioCtrl,
             struct DriverBase*      AHIsubBase )
{
  D(bug("[WASAPI] %s()\n", __func__));

  return AHIS_UNKNOWN;
}
