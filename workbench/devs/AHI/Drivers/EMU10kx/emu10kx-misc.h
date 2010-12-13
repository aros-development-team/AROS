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

#ifndef AHI_Drivers_EMU10kx_emu10kx_misc_h
#define AHI_Drivers_EMU10kx_emu10kx_misc_h

#include <config.h>

#include <devices/ahi.h>
#include <DriverData.h>


struct EMU10kxData*
AllocDriverData( APTR               dev,
		 struct DriverBase* AHIsubBase );

void
FreeDriverData( struct EMU10kxData* dd,
		struct DriverBase*  AHIsubBase );

void
SaveMixerState( struct EMU10kxData* dd );

void
RestoreMixerState( struct EMU10kxData* dd );

void
UpdateMonitorMixer( struct EMU10kxData* dd );

Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits );

Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits );

ULONG
SamplerateToLinearPitch( ULONG samplingrate );

#endif /* AHI_Drivers_EMU10kx_emu10kx_misc_h */
