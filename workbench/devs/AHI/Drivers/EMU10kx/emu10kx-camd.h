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

#ifndef AHI_Drivers_EMU10kx_emu10kx_camd_h
#define AHI_Drivers_EMU10kx_emu10kx_camd_h

#include <config.h>
#include <exec/semaphores.h>

#define EMU10KX_CAMD_SEMAPHORE "emu10kx.camd"

/* This is the global, public data structure used to communicate with
 * the CAMD part of emu10kx.audio. Find it, lock it, use it and then
 * release it. */
   
struct EMU10kxCamd
{
    struct SignalSemaphore  Semaphore;
    UWORD                   Cards;
    UWORD                   Version;
    UWORD                   Revision;
    struct Hook             OpenPortFunc;
    struct Hook             ClosePortFunc;
    struct Hook             ActivateXmitFunc;
};


/* Message for OpenPortFunc */
struct OpenMessage
{
    LONG          PortNum;
    ULONG         V40Mode;
    struct Hook*  TransmitFunc;
    struct Hook*  ReceiveFunc;
};

/* Message for ClosePortFunc */
struct CloseMessage
{
    LONG PortNum;
};

/* Message for ActivateXmitFunc */
struct ActivateMessage
{
    LONG PortNum;
};

/* Message for ReceiveFunc */
struct ReceiveMessage
{
    ULONG InputByte;
};


struct EMU10kxBase;

ULONG
OpenCAMDPort( struct Hook*        hook,
	      struct EMU10kxBase* EMU10kxBase,
	      struct OpenMessage* msg );

VOID
CloseCAMDPort( struct Hook*         hook,
	       struct EMU10kxBase*  EMU10kxBase,
	       struct CloseMessage* msg );

VOID
ActivateCAMDXmit( struct Hook*            hook,
		  struct EMU10kxBase*     EMU10kxBase,
		  struct ActivateMessage* msg );

#endif /* AHI_Drivers_EMU10kx_emu10kx_camd_h */

