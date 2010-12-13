/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#ifndef ahi_sound_h
#define ahi_sound_h

#include "ahi_def.h"

ULONG
_AHI_SetVol ( UWORD                    channel,
	      Fixed                    volume,
	      sposition                pan,
	      struct AHIPrivAudioCtrl* audioctrl,
	      ULONG                    flags,
	      struct AHIBase*          AHIBase );


ULONG
_AHI_SetFreq ( UWORD                    channel,
	       ULONG                    freq,
	       struct AHIPrivAudioCtrl* audioctrl,
	       ULONG                    flags,
	       struct AHIBase*          AHIBase );


ULONG
_AHI_SetSound ( UWORD                    channel,
		UWORD                    sound,
		ULONG                    offset,
		LONG                     length,
		struct AHIPrivAudioCtrl* audioctrl,
		ULONG                    flags,
		struct AHIBase*          AHIBase );


ULONG
_AHI_SetEffect( ULONG*                   effect,
		struct AHIPrivAudioCtrl* audioctrl,
		struct AHIBase*          AHIBase );


ULONG
_AHI_LoadSound( UWORD                    sound,
		ULONG                    type,
		APTR                     info,
		struct AHIPrivAudioCtrl* audioctrl,
		struct AHIBase*          AHIBase );


ULONG
_AHI_UnloadSound( UWORD                    sound,
		  struct AHIPrivAudioCtrl* audioctrl,
		  struct AHIBase*          AHIBase );


ULONG
_AHI_PlayA( struct AHIPrivAudioCtrl* audioctrl,
	    struct TagItem*          tags,
	    struct AHIBase*          AHIBase );


ULONG
_AHI_SampleFrameSize( ULONG           sampletype,
		      struct AHIBase* AHIBase );

#endif /* ahi_sound_h */
