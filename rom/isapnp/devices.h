/* $Id: devices.h,v 1.1 2001/05/10 14:07:50 lcs Exp $ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     
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

#ifndef	ISA_PNP_devices_h
#define ISA_PNP_devices_h

#include "CompilerSpecific.h"

#include <exec/types.h>

struct ISAPNPBase;
struct ISAPNP_Card;
struct ISAPNP_Device;

struct ISAPNP_Card* ASMCALL
ISAPNP_FindCard( REG( a0, struct ISAPNP_Card* last_card ), 
                 REG( d0, LONG                manufacturer ),
                 REG( d1, WORD                product ),
                 REG( d2, BYTE                revision ),
                 REG( d3, LONG                serial ),
                 REG( a6, struct ISAPNPBase*  res ) );


struct ISAPNP_Device* ASMCALL
ISAPNP_FindDevice( REG( a0, struct ISAPNP_Device* last_device ), 
                   REG( d0, LONG                  manufacturer ),
                   REG( d1, WORD                  product ),
                   REG( d2, BYTE                  revision ),
                   REG( a6, struct ISAPNPBase*    res ) );



APTR ASMCALL
ISAPNP_LockCardsA( REG( d0, ULONG                flags ),
                   REG( a0, struct ISAPNP_Card** cards ),
                   REG( a6, struct ISAPNPBase*   res ) );


void ASMCALL
ISAPNP_UnlockCards( REG( a0, APTR               card_lock_handle ),
                    REG( a6, struct ISAPNPBase* res ) );



APTR ASMCALL
ISAPNP_LockDevicesA( REG( d0, ULONG                  flags ),
                     REG( a0, struct ISAPNP_Device** devices ),
                     REG( a6, struct ISAPNPBase*     res ) );


void ASMCALL
ISAPNP_UnlockDevices( REG( a0, APTR               device_lock_handle ),
                      REG( a6, struct ISAPNPBase* res ) );

#endif /* ISA_PNP_devices_h */
