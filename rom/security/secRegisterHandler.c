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
	AROS_LH1(ULONG, secRegisterHandler,

/*  SYNOPSIS */
	/* void */
	AROS_LHA(struct plugin_ops *, ops, A0),

/*  LOCATION */
	struct Library *, SecurityBase, 47, Security)

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

    D(bug( DEBUG_NAME_STR "secRegisterHandler()\n") );;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secRegisterHandler */

