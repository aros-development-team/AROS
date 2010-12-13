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

#ifndef ahi_effectinit_h
#define ahi_effectinit_h

#include <config.h>

#include "ahi_def.h"

BOOL 
update_MasterVolume ( struct AHIPrivAudioCtrl *audioctrl );

BOOL
update_DSPEcho ( struct AHIEffDSPEcho *echo,
                 struct AHIPrivAudioCtrl *audioctrl );

void
free_DSPEcho ( struct AHIPrivAudioCtrl *audioctrl );

BOOL 
update_DSPMask ( struct AHIEffDSPMask *mask,
                 struct AHIPrivAudioCtrl *audioctrl );

void
clear_DSPMask ( struct AHIPrivAudioCtrl *audioctrl );

#endif /* ahi_effectinit_h */
