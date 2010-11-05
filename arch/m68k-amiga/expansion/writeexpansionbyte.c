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

	AROS_LH3(void, WriteExpansionByte,

/*  SYNOPSIS */
	AROS_LHA(APTR , board, A0),
	AROS_LHA(ULONG, offset, D0),
	AROS_LHA(ULONG, byte, D1),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 19, Expansion)

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

	volatile UBYTE *p = (UBYTE*)board;

	offset *= 4;
	p[offset + 2] = byte << 4;
	p[offset] = byte;

    AROS_LIBFUNC_EXIT
} /* WriteExpansionByte */
