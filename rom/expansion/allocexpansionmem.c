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

	AROS_LH2(APTR, AllocExpansionMem,

/*  SYNOPSIS */
	AROS_LHA(ULONG, numSlots, D0),
	AROS_LHA(ULONG, slotAlign, D1),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 9, Expansion)

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

#warning TODO: Write expansion/AllocExpansionMem()
    aros_print_not_implemented ("AllocExpansionMem");
    return 0;

    AROS_LIBFUNC_EXIT
} /* AllocExpansionMem */
