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

#include <devices/ahi.h>
#include <libraries/ahi_sub.h>

#include "library.h"

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
    D(bug("[SB128]: %s()\n", __PRETTY_FUNCTION__));

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
    D(bug("[SB128]: %s()\n", __PRETTY_FUNCTION__));

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
    D(bug("[SB128]: %s()\n", __PRETTY_FUNCTION__));

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
    D(bug("[SB128]: %s()\n", __PRETTY_FUNCTION__));

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
    D(bug("[SB128]: %s()\n", __PRETTY_FUNCTION__));

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
    D(bug("[SB128]: %s()\n", __PRETTY_FUNCTION__));

  return AHIS_UNKNOWN;
}
