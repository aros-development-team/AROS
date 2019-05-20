/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH2(BOOL, secSetProtection,

/*  SYNOPSIS */
	/* (name, mask) */
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(LONG, mask, D2),

/*  LOCATION */
	struct SecurityBase *, secBase, 15, Security)

/*  FUNCTION

    INPUTS


    RESULT


    NOTES


    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    return 0;

    AROS_LIBFUNC_EXIT

} /* secSetProtection */

