/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "expansion_intern.h"

#include <clib/expansion_protos.h>

/* See rom/expansion/readexpansionbyte.c for documentation */

AROS_LH2(UBYTE, ReadExpansionByte,
    AROS_LHA(APTR , board, A0),
    AROS_LHA(ULONG, offset, D0),
    struct ExpansionBase *, ExpansionBase, 16, Expansion)
{
    AROS_LIBFUNC_INIT

	UBYTE v;
	UWORD loffset;
	volatile UBYTE *p = (UBYTE*)board;

	offset *= 4;
	if (((ULONG)board) & 0xff000000)
		loffset = 0x100;
	else
		loffset = 0x002;
	v = (p[offset + loffset] & 0xf0) >> 4;
	v |= p[offset] & 0xf0;
	return v;

    AROS_LIBFUNC_EXIT
} /* ReadExpansionByte */
