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

#ifndef ahi_devcommands_h
#define ahi_devcommands_h

#include "ahi_def.h"

void
_DevBeginIO( struct AHIRequest* ioreq,
	     struct AHIBase*    AHIBase );

ULONG
_DevAbortIO( struct AHIRequest* ioreq,
	     struct AHIBase*    AHIBase );

void
PerformIO ( struct AHIRequest *ioreq,
            struct AHIBase *AHIBase );

void
FeedReaders ( struct AHIDevUnit *iounit,
              struct AHIBase *AHIBase );

void
RethinkPlayers ( struct AHIDevUnit *iounit,
                 struct AHIBase *AHIBase );

void 
UpdateSilentPlayers ( struct AHIDevUnit *iounit,
                      struct AHIBase *AHIBase );

#endif /* ahi_devcommands_h */
