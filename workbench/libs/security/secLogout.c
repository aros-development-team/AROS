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
	AROS_LH0(ULONG, secLogout,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct Library *, SecurityBase, 5, Security)

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

    D(bug( DEBUG_NAME_STR "secLogout()\n") );;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secLogout */

