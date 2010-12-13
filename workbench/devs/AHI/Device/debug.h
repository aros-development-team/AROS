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

#ifndef ahi_debug_h
#define ahi_debug_h

#include <devices/ahi.h>
#include <proto/exec.h>


void
KPrintFArgs( UBYTE* fmt, 
             ULONG* args );

#ifndef __AMIGAOS4__
#define KPrintF( fmt, ... )        \
({                                 \
  ULONG _args[] = { __VA_ARGS__ }; \
  KPrintFArgs( (fmt), _args );     \
})
#else
#define KPrintF DebugPrintF
#endif

void
Debug_AllocAudioA( struct TagItem *tags );

void
Debug_FreeAudio( struct AHIPrivAudioCtrl *audioctrl );

void
Debug_KillAudio( void );

void
Debug_ControlAudioA( struct AHIPrivAudioCtrl *audioctrl, struct TagItem *tags );


void
Debug_SetVol( UWORD chan, Fixed vol, sposition pan, struct AHIPrivAudioCtrl *audioctrl, ULONG flags);

void
Debug_SetFreq( UWORD chan, ULONG freq, struct AHIPrivAudioCtrl *audioctrl, ULONG flags);

void
Debug_SetSound( UWORD chan, UWORD sound, ULONG offset, LONG length, struct AHIPrivAudioCtrl *audioctrl, ULONG flags);

void
Debug_SetEffect( ULONG *effect, struct AHIPrivAudioCtrl *audioctrl );

void
Debug_LoadSound( UWORD sound, ULONG type, APTR info, struct AHIPrivAudioCtrl *audioctrl );

void
Debug_UnloadSound( UWORD sound, struct AHIPrivAudioCtrl *audioctrl );

void
Debug_NextAudioID( ULONG id);

void
Debug_GetAudioAttrsA( ULONG id, struct AHIPrivAudioCtrl *audioctrl, struct TagItem *tags );

void
Debug_BestAudioIDA( struct TagItem *tags );

void
Debug_AllocAudioRequestA( struct TagItem *tags );

void
Debug_AudioRequestA( struct AHIAudioModeRequester *req, struct TagItem *tags );

void
Debug_FreeAudioRequest( struct AHIAudioModeRequester *req );

void
Debug_PlayA( struct AHIPrivAudioCtrl *audioctrl, struct TagItem *tags );

void
Debug_SampleFrameSize( ULONG sampletype);

void
Debug_AddAudioMode(struct TagItem *tags );

void
Debug_RemoveAudioMode( ULONG id);

void
Debug_LoadModeFile( STRPTR name);

#endif /* ahi_debug_h */
