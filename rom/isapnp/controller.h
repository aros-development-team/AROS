/* $Id$ */

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

AROS_LD1(void, ISAC_SetMasterInt,
         AROS_LHA(BOOL, on, D0),
         struct ISAPNPBase *, res, 1, ISAPNP);

AROS_LD0(BOOL, ISAC_GetMasterInt,
         struct ISAPNPBase *, res, 2, ISAPNP);

AROS_LD1(void, ISAC_SetWaitState,
         AROS_LHA(BOOL, on, D0),
         struct ISAPNPBase *, res, 3, ISAPNP);

AROS_LD0(BOOL, ISAC_GetWaitState,
         struct ISAPNPBase *, res, 4, ISAPNP);

AROS_LD1(BOOL, ISAC_GetInterruptStatus,
         AROS_LHA(UBYTE, interrupt, D0),
         struct ISAPNPBase *, res, 5, ISAPNP);

AROS_LD1(UBYTE, ISAC_GetRegByte,
         AROS_LHA(UWORD, reg, D0),
         struct ISAPNPBase *, res, 6, ISAPNP);

AROS_LD2(void, ISAC_SetRegByte,
         AROS_LHA(UWORD, reg, D0),
         AROS_LHA(UBYTE, value, D1),
         struct ISAPNPBase *, res, 7, ISAPNP);

AROS_LD1(UWORD, ISAC_GetRegWord,
         AROS_LHA(UWORD, reg, D0),
         struct ISAPNPBase *, res, 8, ISAPNP);

AROS_LD2(void, ISAC_SetRegWord,
         AROS_LHA(UWORD, reg, D0),
         AROS_LHA(UWORD, value, D1),
         struct ISAPNPBase *, res, 9, ISAPNP);

AROS_LD1(ULONG, ISAC_GetRegLong,
         AROS_LHA(UWORD, reg, D0),
         struct ISAPNPBase *, res, 10, ISAPNP);

AROS_LD2(void, ISAC_SetRegLong,
         AROS_LHA(UWORD, reg, D0),
         AROS_LHA(ULONG, value, D1),
         struct ISAPNPBase *, res, 11, ISAPNP);

AROS_LD1(UBYTE, ISAC_ReadByte,
         AROS_LHA(ULONG, address, D0),
         struct ISAPNPBase *, res, 12, ISAPNP);

AROS_LD2(void, ISAC_WriteByte,
         AROS_LHA(ULONG, address, D0),
         AROS_LHA(UBYTE, value, D1 ),
         struct ISAPNPBase *, res, 13, ISAPNP);

AROS_LD1(UWORD, ISAC_ReadWord,
         AROS_LHA(ULONG, address, D0),
         struct ISAPNPBase *, res, 14, ISAPNP);

AROS_LD2(void, ISAC_WriteWord,
         AROS_LHA(ULONG, address, D0),
         AROS_LHA(UWORD, value, D1),
         struct ISAPNPBase *, res, 15, ISAPNP);

AROS_LD1(UWORD, ISAC_ReadLong,
         AROS_LHA(ULONG, address, D0),
         struct ISAPNPBase *, res, 16, ISAPNP);

AROS_LD2(void, ISAC_WriteLong,
         AROS_LHA(ULONG, address, D0),
         AROS_LHA(ULONG, value, D1 ),
         struct ISAPNPBase *, res, 17, ISAPNP);

static inline UBYTE ISAC_GetRegByte(UWORD reg, struct ISAPNPBase *res)
{
        return AROS_LC1(UBYTE, ISAC_GetRegByte,
                        AROS_LHA(UWORD, reg, D0),
                        struct ISAPNPBase *, res, 6, ISAPNP);
}

static inline void ISAC_SetRegByte(UWORD reg, UBYTE value, struct ISAPNPBase *res)
{
        return AROS_LC2(void, ISAC_SetRegByte,
                        AROS_LHA(UWORD, reg, D0),
                        AROS_LHA(UBYTE, value, D1),
                        struct ISAPNPBase *, res, 7, ISAPNP);
}

#endif /* ISA_PNP_controller_h */
