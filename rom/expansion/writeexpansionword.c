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

	AROS_LH3(void, WriteExpansionWord,

/*  SYNOPSIS */
	AROS_LHA(APTR , board, A0),
	AROS_LHA(ULONG, offset, D0),
	AROS_LHA(ULONG, word, D1),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 27, Expansion)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
        This function isn't implemented on all platforms.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* TODO: Write expansion/WriteExpansionWord() */
    aros_print_not_implemented ("WriteExpansionWord");

    AROS_LIBFUNC_EXIT
} /* WriteExpansionWord */
