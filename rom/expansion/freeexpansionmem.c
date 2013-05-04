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

	AROS_LH2(void, FreeExpansionMem,

/*  SYNOPSIS */
	AROS_LHA(ULONG, startSlot, D0),
	AROS_LHA(ULONG, numSlots, D1),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 15, Expansion)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
        This function is unimplemented.

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* TODO: Write expansion/FreeExpansionMem() */
    aros_print_not_implemented ("FreeExpansionMem");

    AROS_LIBFUNC_EXIT
} /* FreeExpansionMem */
