/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#include "security_intern.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */
	AROS_LH2(int, secsetreuid,

/*  SYNOPSIS */
	/* (ruid, euid) */
	AROS_LHA(int, ruid, D0),
	AROS_LHA(int, euid, D1),

/*  LOCATION */
	struct Library *, SecurityBase, 45, Security)

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
    AROS_LIBBASE_EXT_DECL(struct Library *, SecurityBase)

    D(bug( DEBUG_NAME_STR "secsetreuid()\n") );;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secsetreuid */

