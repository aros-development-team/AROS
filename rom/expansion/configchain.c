/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "expansion_intern.h"

/*****************************************************************************

    NAME */
#include <clib/expansion_protos.h>

	AROS_LH1(void, ConfigChain,

/*  SYNOPSIS */
	AROS_LHA(APTR, baseAddr, A0),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 11, Expansion)

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

    /*
     * This function can actually be implemented only on classic Amiga(tm)
     * because only Amigas have Zorro bus
     */

    AROS_LIBFUNC_EXIT
} /* ConfigChain */
