/*
    Copyright � 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#include "security_intern.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */
	AROS_LH0(UWORD, secgetuid,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct Library *, SecurityBase, 39, Security)

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

    D(bug( DEBUG_NAME_STR "secgetuid()\n") );;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secgetuid */

