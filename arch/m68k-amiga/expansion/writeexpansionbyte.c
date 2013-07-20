/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "expansion_intern.h"

#include <clib/expansion_protos.h>

/* See rom/expansion/writeexpansionbyte.c for documentation */

AROS_LH3(void, WriteExpansionByte,
    AROS_LHA(APTR , board, A0),
    AROS_LHA(ULONG, offset, D0),
    AROS_LHA(ULONG, byte, D1),
    struct ExpansionBase *, ExpansionBase, 19, Expansion)
{
    AROS_LIBFUNC_INIT

	UWORD loffset;
	volatile UBYTE *p = (UBYTE*)board;

	offset *= 4;
	if (((ULONG)board) & 0xff000000)
		loffset = 0x100;
	else
		loffset = 0x002;
	p[offset + loffset] = byte << 4;
	p[offset] = byte;

    AROS_LIBFUNC_EXIT
} /* WriteExpansionByte */
