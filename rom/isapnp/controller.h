/* $Id: controller.h,v 1.6 2001/05/29 10:53:44 lcs Exp $ */

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

#ifndef	ISA_PNP_controller_h
#define ISA_PNP_controller_h

#include "CompilerSpecific.h"

#include <exec/types.h>

struct ISAPNPBase;

void ASMCALL
ISAC_SetMasterInt( REG( d0, BOOL               on ),
                   REG( a6, struct ISAPNPBase* res ) );

BOOL ASMCALL
ISAC_GetMasterInt( REG( a6, struct ISAPNPBase* res ) );


void ASMCALL
ISAC_SetWaitState( REG( d0, BOOL               on ),
                   REG( a6, struct ISAPNPBase* res ) );


BOOL ASMCALL
ISAC_GetWaitState( REG( a6, struct ISAPNPBase* res ) );


BOOL ASMCALL
ISAC_GetInterruptStatus( REG( d0, UBYTE              interrupt ),
                         REG( a6, struct ISAPNPBase* res ) );

UBYTE ASMCALL
ISAC_GetRegByte( REG( d0, UWORD              reg ),
                 REG( a6, struct ISAPNPBase* res ) );


void ASMCALL
ISAC_SetRegByte( REG( d0, UWORD              reg ),
                 REG( d1, UBYTE              value ),
                 REG( a6, struct ISAPNPBase* res ) );


UWORD ASMCALL
ISAC_GetRegWord( REG( d0, UWORD              reg ),
                 REG( a6, struct ISAPNPBase* res ) );


void ASMCALL
ISAC_SetRegWord( REG( d0, UWORD              reg ),
                 REG( d1, UWORD              value ),
                 REG( a6, struct ISAPNPBase* res ) );

ULONG ASMCALL
ISAC_GetRegLong( REG( d0, UWORD              reg ),
                 REG( a6, struct ISAPNPBase* res ) );


void ASMCALL
ISAC_SetRegLong( REG( d0, UWORD              reg ),
                 REG( d1, ULONG              value ),
                 REG( a6, struct ISAPNPBase* res ) );


UBYTE ASMCALL
ISAC_ReadByte( REG( d0, ULONG              address ),
               REG( a6, struct ISAPNPBase* res ) );


void ASMCALL
ISAC_WriteByte( REG( d0, ULONG              address ),
                REG( d1, UBYTE              value ),
                REG( a6, struct ISAPNPBase* res ) );


UWORD ASMCALL
ISAC_ReadWord( REG( d0, ULONG              address ),
               REG( a6, struct ISAPNPBase* res ) );


void ASMCALL
ISAC_WriteWord( REG( d0, ULONG              address ),
                REG( d1, UWORD              value ),
                REG( a6, struct ISAPNPBase* res ) );


ULONG ASMCALL
ISAC_ReadLong( REG( d0, ULONG              address ),
               REG( a6, struct ISAPNPBase* res ) );


void ASMCALL
ISAC_WriteLong( REG( d0, ULONG              address ),
                REG( d1, ULONG              value ),
                REG( a6, struct ISAPNPBase* res ) );


#endif /* ISA_PNP_controller_h */
