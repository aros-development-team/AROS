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

#ifndef ahi_mixer_h
#define ahi_mixer_h

#include <config.h>

#include "ahi_def.h"
#include "addroutines.h"


BOOL
InitMixroutine ( struct AHIPrivAudioCtrl *audioctrl );


void
CleanUpMixroutine ( struct AHIPrivAudioCtrl *audioctrl );


void
SelectAddRoutine ( Fixed     VolumeLeft,
                   Fixed     VolumeRight,
                   ULONG     SampleType,
                   struct    AHIPrivAudioCtrl *audioctrl,
                   LONG     *ScaleLeft,
                   LONG     *ScaleRight,
                   ADDFUNC **AddRoutine );


void
MixerFunc( struct Hook*             hook,
           struct AHIPrivAudioCtrl* audioctrl,
           void*                    dst );


void
Mix( struct Hook*             unused_Hook, 
     struct AHIPrivAudioCtrl* audioctrl,
     void*                    dst );

void
DoMasterVolume ( void *buffer,
                 struct AHIPrivAudioCtrl *audioctrl );


void
DoOutputBuffer ( void *buffer,
                 struct AHIPrivAudioCtrl *audioctrl );


void
DoChannelInfo ( struct AHIPrivAudioCtrl *audioctrl );


LONG
CalcSamples ( Fixed64 Add,
              ULONG   Type,
              Fixed64 LastOffset,
              Fixed64 Offset );

#endif /* ahi_mixer_h */
