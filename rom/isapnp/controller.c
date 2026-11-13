/* $Id$ */

/*
     ISA-PnP -- A Plug And Play ISA software layer for AmigaOS.
     Copyright (C) 2001 Martin Blom <martin@blom.org>
     Copyright (C) 2009-2013 The AROS Development Team
     
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

AROS_LH1(void, ISAC_SetMasterInt,
         AROS_LHA(BOOL, on, D0),
         struct ISAPNPBase *, res, 1, ISAPNP)
{
    AROS_LIBFUNC_INIT

    /* On a PC we have no interrupt mapping so we don't have a 'master interrupt' */

    AROS_LIBFUNC_EXIT
}


AROS_LH0(BOOL, ISAC_GetMasterInt,
         struct ISAPNPBase *, res, 2, ISAPNP)
{
    AROS_LIBFUNC_INIT

    return TRUE;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(void, ISAC_SetWaitState,
         AROS_LHA(BOOL, on, D0),
         struct ISAPNPBase *, res, 3, ISAPNP)
{
    AROS_LIBFUNC_INIT

    /* On a PC we can't control it */

    AROS_LIBFUNC_EXIT
}


AROS_LH0(BOOL, ISAC_GetWaitState,
         struct ISAPNPBase *, res, 4, ISAPNP)
{
    AROS_LIBFUNC_INIT

    return TRUE;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(BOOL, ISAC_GetInterruptStatus,
         AROS_LHA(UBYTE, interrupt, D0),
         struct ISAPNPBase *, res, 5, ISAPNP)
{
    AROS_LIBFUNC_INIT

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

    AROS_LIBFUNC_EXIT
}


AROS_LH1(UBYTE, ISAC_GetRegByte,
         AROS_LHA(UWORD, reg, D0),
         struct ISAPNPBase *, res, 6, ISAPNP)
{
    AROS_LIBFUNC_INIT

    return inb(reg);

    AROS_LIBFUNC_EXIT
}


AROS_LH2(void, ISAC_SetRegByte,
         AROS_LHA(UWORD, reg, D0),
         AROS_LHA(UBYTE, value, D1),
         struct ISAPNPBase *, res, 7, ISAPNP)
{
    AROS_LIBFUNC_INIT

    outb(value, reg);

    AROS_LIBFUNC_EXIT
}


AROS_LH1(UWORD, ISAC_GetRegWord,
         AROS_LHA(UWORD, reg, D0),
         struct ISAPNPBase *, res, 8, ISAPNP)
{
    AROS_LIBFUNC_INIT

    return inw(reg);

    AROS_LIBFUNC_EXIT
}


AROS_LH2(void, ISAC_SetRegWord,
         AROS_LHA(UWORD, reg, D0),
         AROS_LHA(UWORD, value, D1),
         struct ISAPNPBase *, res, 9, ISAPNP)
{
    AROS_LIBFUNC_INIT

    outw(value, reg);

    AROS_LIBFUNC_EXIT
}


AROS_LH1(ULONG, ISAC_GetRegLong,
         AROS_LHA(UWORD, reg, D0),
         struct ISAPNPBase *, res, 10, ISAPNP)
{
    AROS_LIBFUNC_INIT

    return inl(reg);

    AROS_LIBFUNC_EXIT
}


AROS_LH2(void, ISAC_SetRegLong,
         AROS_LHA(UWORD, reg, D0),
         AROS_LHA(ULONG, value, D1),
         struct ISAPNPBase *, res, 11, ISAPNP)
{
    AROS_LIBFUNC_INIT

    outl(value, reg);

    AROS_LIBFUNC_EXIT
}


AROS_LH1(UBYTE, ISAC_ReadByte,
         AROS_LHA(ULONG, address, D0),
         struct ISAPNPBase *, res, 12, ISAPNP)
{
    AROS_LIBFUNC_INIT

    return *(UBYTE *)address;

    AROS_LIBFUNC_EXIT
}


AROS_LH2(void, ISAC_WriteByte,
         AROS_LHA(ULONG, address, D0),
         AROS_LHA(UBYTE, value, D1 ),
         struct ISAPNPBase *, res, 13, ISAPNP)
{
    AROS_LIBFUNC_INIT

    *(UBYTE *)address = value;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(UWORD, ISAC_ReadWord,
         AROS_LHA(ULONG, address, D0),
         struct ISAPNPBase *, res, 14, ISAPNP)
{
    AROS_LIBFUNC_INIT

    return *(UWORD *)address;

    AROS_LIBFUNC_EXIT
}


AROS_LH2(void, ISAC_WriteWord,
         AROS_LHA(ULONG, address, D0),
         AROS_LHA(UWORD, value, D1),
         struct ISAPNPBase *, res, 15, ISAPNP)
{
    AROS_LIBFUNC_INIT

    *(UWORD *)address = value;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(UWORD, ISAC_ReadLong,
         AROS_LHA(ULONG, address, D0),
         struct ISAPNPBase *, res, 16, ISAPNP)
{
    AROS_LIBFUNC_INIT

    return *(ULONG *)address;

    AROS_LIBFUNC_EXIT
}


AROS_LH2(void, ISAC_WriteLong,
         AROS_LHA(ULONG, address, D0),
         AROS_LHA(ULONG, value, D1 ),
         struct ISAPNPBase *, res, 17, ISAPNP)
{
    AROS_LIBFUNC_INIT

    *(ULONG *)address = value;

    AROS_LIBFUNC_EXIT
}
