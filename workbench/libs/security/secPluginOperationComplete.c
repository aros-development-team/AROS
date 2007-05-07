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
	AROS_LH2(void, secPluginOperationComplete,

/*  SYNOPSIS */
	/* (context, result) */
	AROS_LHA(APTR, context, A0),
	AROS_LHA(ULONG, result, D0),

/*  LOCATION */
	struct Library *, SecurityBase, 49, Security)

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

    D(bug( DEBUG_NAME_STR "secPluginOperationComplete()\n") );;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* secPluginOperationComplete */

