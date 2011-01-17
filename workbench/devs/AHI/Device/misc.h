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

#ifndef ahi_misc_h
#define ahi_misc_h

#include <config.h>

#include <exec/lists.h>
#include <exec/nodes.h>

#include <stddef.h>

#include "ahi_def.h"


struct Node *
FindNode ( struct List *list,
           struct Node *node );

int
Fixed2Shift( Fixed f );

void
ReqA( const char* text, APTR args );

#define Req(a0, args...) \
        ({IPTR _args[] = { args }; ReqA((a0), (APTR)_args);})

char*
SprintfA( char *dst, const char *fmt, IPTR* args );

#define Sprintf(a0, a1, args...) \
        ({IPTR _args[] = { args }; SprintfA((a0), (a1), (IPTR*)_args);})



void
AHIInitSemaphore( struct SignalSemaphore* ss );

void
AHIObtainSemaphore( struct SignalSemaphore* ss );

void
AHIReleaseSemaphore( struct SignalSemaphore* ss );

LONG
AHIAttemptSemaphore( struct SignalSemaphore* ss );



APTR
AHIAllocVec( ULONG byteSize,
             ULONG requirements );

void
AHIFreeVec( APTR memoryBlock );

void*
AHILoadObject( const char* objname );

void
AHIUnloadObject( void* obj );

BOOL
AHIGetELFSymbol( const char* name,
                 void** ptr );


#if !defined( WORDS_BIGENDIAN )
void EndianSwap( size_t size, void* data );
#else
# define EndianSwap(s, x)
#endif

BOOL
PreTimerFunc( struct Hook*             hook,
	      struct AHIPrivAudioCtrl* audioctrl,
	      void*                    null );

void
PostTimerFunc( struct Hook*             hook,
	       struct AHIPrivAudioCtrl* audioctrl,
	       void*                    null );

BOOL
PreTimer( struct AHIPrivAudioCtrl* audioctrl );

void
PostTimer( struct AHIPrivAudioCtrl* audioctrl );

#endif /* ahi_misc_h */
