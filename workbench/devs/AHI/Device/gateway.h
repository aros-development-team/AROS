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

#ifndef ahi_gateway_h
#define ahi_gateway_h

#include <config.h>

void m68k_IndexToFrequency( void );
void m68k_DevProc( void );


#if defined( __AROS__ ) && !defined( __mc68000__ )

BOOL m68k_PreTimer( void *);
void m68k_PostTimer( void *);

#define HookEntryPreserveAllRegs HookEntry
#define PreTimerPreserveAllRegs  m68k_PreTimer
#define PostTimerPreserveAllRegs m68k_PostTimer

#else

/* Special hook entry points */
void HookEntryPreserveAllRegs( void );

/* (Pre|Post)Timer entry points */
BOOL PreTimerPreserveAllRegs( void * );
void PostTimerPreserveAllRegs( void * );

#endif /* defined( __AROS__ ) && !defined( __mc68000__ ) */

#ifdef __AMIGAOS4__
ULONG HookEntry( struct Hook* h, void* o, void* m );
#endif

#endif /* ahi_gateway_h */
