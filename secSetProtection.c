/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#include "security_intern.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */
	AROS_LH2(BOOL, secSetProtection,

/*  SYNOPSIS */
	/* (name, mask) */
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(LONG, mask, D2),

/*  LOCATION */
	struct Library *, SecurityBase, 15, Security)

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

    D(bug( DEBUG_NAME_STR "secSetProtection()\n") );;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secSetProtection */

