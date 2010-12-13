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

#ifndef AHI_Drivers_EMU10kx_emu10kx_interrupt_h
#define AHI_Drivers_EMU10kx_emu10kx_interrupt_h

#include <config.h>

#include "DriverData.h"

#ifdef __AMIGAOS4__
ULONG
EMU10kxInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct EMU10kxData* dd );

void
PlaybackInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct EMU10kxData* dd );

void
RecordInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct EMU10kxData* dd );
#else

ULONG
EMU10kxInterrupt( struct EMU10kxData* dd );

void
PlaybackInterrupt( struct EMU10kxData* dd );

void
RecordInterrupt( struct EMU10kxData* dd );

#endif

#endif /* AHI_Drivers_EMU10kx_emu10kx_interrupt_h */
