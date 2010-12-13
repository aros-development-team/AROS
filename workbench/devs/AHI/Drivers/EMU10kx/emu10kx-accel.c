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

#include <devices/ahi.h>
#include <libraries/ahi_sub.h>
#include <utility/hooks.h>

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
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

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
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

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
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

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
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

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
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

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
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  return AHIS_UNKNOWN;
}
