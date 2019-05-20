/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH2(struct secPrivGroupInfo *, secPasswd,

/*  SYNOPSIS */
	/* (oldpwd, newpwd) */
	AROS_LHA(STRPTR, oldpwd, A0),
	AROS_LHA(STRPTR, newpwd, A1),

/*  LOCATION */
	struct SecurityBase *, secBase, 8, Security)

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

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secPasswd */

