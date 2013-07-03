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

	AROS_LH2(BOOL, ConfigBoard,

/*  SYNOPSIS */
	AROS_LHA(APTR              , board, A0),
	AROS_LHA(struct ConfigDev *, configDev, A1),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 10, Expansion)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
        This function isn't implemented on all platforms.

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

    /* TODO: Write expansion/ConfigBoard() */
    aros_print_not_implemented ("ConfigBoard");
    
    return FALSE;

    AROS_LIBFUNC_EXIT
} /* ConfigBoard */
