/* $Id: pnp_structs.h,v 1.3 2001/05/05 13:11:21 lcs Exp $ */

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

#ifndef	ISA_PNP_pnp_structs_h
#define ISA_PNP_pnp_structs_h

#include "CompilerSpecific.h"

#include <exec/types.h>

struct ISAPNPBase;

struct ISAPNP_Card;
struct ISAPNP_Device;
struct ISAPNP_ResourceGroup;
struct ISAPNP_Resource;

struct ISAPNP_Card* ASMCALL
ISAPNP_AllocCard( REG( a6, struct ISAPNPBase* res ) );

void ASMCALL
ISAPNP_FreeCard( REG( a0, struct ISAPNP_Card* card ),
                 REG( a6, struct ISAPNPBase*  res ) );


struct ISAPNP_Device* ASMCALL
ISAPNP_AllocDevice( REG( a6, struct ISAPNPBase* res ) );

void ASMCALL
ISAPNP_FreeDevice( REG( a0, struct ISAPNP_Device* dev ),
                   REG( a6, struct ISAPNPBase*    res ) );


struct ISAPNP_ResourceGroup* ASMCALL
ISAPNP_AllocResourceGroup( REG( d0, UBYTE              pri ),
                           REG( a6, struct ISAPNPBase* res ) );

void ASMCALL
ISAPNP_FreeResourceGroup( REG( a0, struct ISAPNP_ResourceGroup* rg ),
                          REG( a6, struct ISAPNPBase*           res ) );


struct ISAPNP_Resource* ASMCALL
ISAPNP_AllocResource( REG( d0, UBYTE              type ),
                      REG( a6, struct ISAPNPBase* res ) );

void ASMCALL
ISAPNP_FreeResource( REG( a0, struct ISAPNP_Resource* r ),
                     REG( a6, struct ISAPNPBase*      res ) );


#endif /* ISA_PNP_pnp_structs_h */
