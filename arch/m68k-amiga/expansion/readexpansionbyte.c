/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "expansion_intern.h"

/*****************************************************************************

    NAME */
#include <clib/expansion_protos.h>

	AROS_LH2(UBYTE, ReadExpansionByte,

/*  SYNOPSIS */
	AROS_LHA(APTR , board, A0),
	AROS_LHA(ULONG, offset, D0),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 16, Expansion)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
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
	v = (p[offset] & 0xf0) | ((p[offset + loffset] & 0xf0) >> 4);
	return v;

    AROS_LIBFUNC_EXIT
} /* ReadExpansionByte */
