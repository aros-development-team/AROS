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

#ifndef ahi_warpup_h
#define ahi_warpup_h

#include <exec/interrupts.h>
#include <utility/hooks.h>

#include "ahi_def.h"

struct PowerPCContext
{
  volatile int              Command;
  volatile void*            Argument;

  int                       Active;
  struct Hook*              Hook;
  void*	                    Dst;
  void*                     XLock;

  struct AHIPrivAudioCtrl*  AudioCtrl;
  struct Library*           PowerPCBase;
  
  struct Interrupt*         MixInterrupt;
  void*                     MixBuffer;
};

#define PPCC_COM_NONE          0
#define PPCC_COM_ACK           1
#define PPCC_COM_START         2
#define PPCC_COM_CONTINUE      3
#define PPCC_COM_INIT          4
#define PPCC_COM_SOUNDFUNC     5
#define PPCC_COM_QUIT          6
#define PPCC_COM_DEBUG         7
#define PPCC_COM_FINISHED      8

BOOL
WarpUpInit( struct AHIPrivAudioCtrl* audioctrl );

void
WarpUpCallMixer( struct AHIPrivAudioCtrl* audioctrl,
                 void* dst );

void
WarpUpCallSoundHook( struct AHIPrivAudioCtrl *audioctrl,
                     void* arg );

void
WarpUpCleanUp( struct AHIPrivAudioCtrl* audioctrl );

#endif /* ahi_warpup_h */
