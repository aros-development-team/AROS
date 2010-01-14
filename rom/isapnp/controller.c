/* $Id: controller.c,v 1.7 2001/05/29 10:53:44 lcs Exp $ */

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

#include <asm/io.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>

#include <proto/expansion.h>

#include "controller.h"
#include "isapnp_private.h"

/******************************************************************************
*** Controller functions ******************************************************
******************************************************************************/

void ASMCALL
ISAC_SetMasterInt( REG( d0, BOOL               on ),
                   REG( a6, struct ISAPNPBase* res ) )
{
    /* On a PC we have no interrupt mapping so we don't have a 'master interrupt' */
}


BOOL ASMCALL
ISAC_GetMasterInt( REG( a6, struct ISAPNPBase* res ) )
{
    return TRUE;
}


void ASMCALL
ISAC_SetWaitState( REG( d0, BOOL               on ),
                   REG( a6, struct ISAPNPBase* res ) )
{
    /* On a PC we can't control it */
}


BOOL ASMCALL
ISAC_GetWaitState( REG( a6, struct ISAPNPBase* res ) )
{
    return TRUE;
}


BOOL ASMCALL
ISAC_GetInterruptStatus( REG( d0, UBYTE              interrupt ),
                         REG( a6, struct ISAPNPBase* res ) )
{
    bug("[ISAPNP] ISAC_GetInterruptStatus() is not implemented\n");
/*UWORD* reg1 = (UWORD*)( res->m_Base + 0x18000 );

  switch( interrupt )
  {
    case 3:
      return ( ReadWord( reg1 ) & 4 ) != 0;

    case 4:
      return ( ReadWord( reg1 ) & 8 ) != 0;

    case 5:
      return ( ReadWord( reg1 ) & 16 ) != 0;

    case 6:
      return ( ReadWord( reg1 ) & 32 ) != 0;

    case 7:
      return ( ReadWord( reg1 ) & 64 ) != 0;

    case 9:
      return ( ReadWord( reg1 ) & 128 ) != 0;

    case 10:
      return ( ReadWord( reg1 ) & 256 ) != 0;

    case 11:
      return ( ReadWord( reg1 ) & 512 ) != 0;

    case 12:
      return ( ReadWord( reg1 ) & 1024 ) != 0;

    case 14:
      return ( ReadWord( reg1 ) & 2048 ) != 0;

    case 15:
      return ( ReadWord( reg1 ) & 4096 ) != 0;
  }
*/
  return FALSE;
}


UBYTE ASMCALL
ISAC_GetRegByte( REG( d0, UWORD              reg ),
                 REG( a6, struct ISAPNPBase* res ) )
{
    return inb(reg);
}


void ASMCALL
ISAC_SetRegByte( REG( d0, UWORD              reg ),
                 REG( d1, UBYTE              value ),
                 REG( a6, struct ISAPNPBase* res ) )
{
    outb(value, reg);
}


UWORD ASMCALL
ISAC_GetRegWord( REG( d0, UWORD              reg ),
                 REG( a6, struct ISAPNPBase* res ) )
{
    return inw(reg);
}


void ASMCALL
ISAC_SetRegWord( REG( d0, UWORD              reg ),
                 REG( d1, UWORD              value ),
                 REG( a6, struct ISAPNPBase* res ) )
{
    outw(value, reg);
}


ULONG ASMCALL
ISAC_GetRegLong( REG( d0, UWORD              reg ),
                 REG( a6, struct ISAPNPBase* res ) )
{
    return inl(reg);
}


void ASMCALL
ISAC_SetRegLong( REG( d0, UWORD              reg ),
                 REG( d1, ULONG              value ),
                 REG( a6, struct ISAPNPBase* res ) )
{
    outl(value, reg);
}


UBYTE ASMCALL
ISAC_ReadByte( REG( d0, ULONG              address ),
               REG( a6, struct ISAPNPBase* res ) )
{
    return *(UBYTE *)address;
}


void ASMCALL
ISAC_WriteByte( REG( d0, ULONG              address ),
                REG( d1, UBYTE              value ),
                REG( a6, struct ISAPNPBase* res ) )
{
    *(UBYTE *)address = value;
}


UWORD ASMCALL
ISAC_ReadWord( REG( d0, ULONG              address ),
               REG( a6, struct ISAPNPBase* res ) )
{
    return *(UWORD *)address;
}


void ASMCALL
ISAC_WriteWord( REG( d0, ULONG              address ),
                REG( d1, UWORD              value ),
                REG( a6, struct ISAPNPBase* res ) )
{
    *(UWORD *)address = value;
}


ULONG ASMCALL
ISAC_ReadLong( REG( d0, ULONG              address ),
               REG( a6, struct ISAPNPBase* res ) )
{
    return *(ULONG *)address;
}


void ASMCALL
ISAC_WriteLong( REG( d0, ULONG              address ),
                REG( d1, ULONG              value ),
                REG( a6, struct ISAPNPBase* res ) )
{
    *(ULONG *)address = value;
}
