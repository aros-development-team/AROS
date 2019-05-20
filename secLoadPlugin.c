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
	AROS_LH1(BOOL, secLoadPlugin,

/*  SYNOPSIS */
	/* (name) */
	AROS_LHA(STRPTR, name, A0),

/*  LOCATION */
	struct Library *, SecurityBase, 50, Security)

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

    D(bug( DEBUG_NAME_STR "secLoadPlugin()\n") );;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secLoadPlugin */

