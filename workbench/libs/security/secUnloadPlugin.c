/*
    Copyright � 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#include "security_intern.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */
	AROS_LH1(BOOL, secUnloadPlugin,

/*  SYNOPSIS */
	/* void */
	AROS_LHA(STRPTR, name, A0),

/*  LOCATION */
	struct Library *, SecurityBase, 51, Security)

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

    D(bug( DEBUG_NAME_STR "secUnloadPlugin()\n") );;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secUnloadPlugin */

