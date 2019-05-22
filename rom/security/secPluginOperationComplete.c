/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH2(void, secPluginOperationComplete,

/*  SYNOPSIS */
	/* (context, result) */
	AROS_LHA(APTR, context, A0),
	AROS_LHA(ULONG, result, D0),

/*  LOCATION */
	struct SecurityBase *, secBase, 49, Security)

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

    return;

    AROS_LIBFUNC_EXIT

} /* secPluginOperationComplete */

