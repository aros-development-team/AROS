/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/security.h>
#include <stdio.h>

#include "security_intern.h"
#include "security_task.h"

/*****************************************************************************

    NAME */
	AROS_LH0(BOOL, secFSRendezVous,

/*  SYNOPSIS */

/*  LOCATION */
	struct SecurityBase *, secBase, 18, Security)

/*  FUNCTION
	Freeze a task or process

    INPUTS


    RESULT


    NOTES
	This function may be called by root only!

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL res = FALSE;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    Forbid();
    if (secBase->Server && secBase->ConsistencySig) {
            Signal((struct Task *)secBase->Server, 1<<secBase->ConsistencySig);
            res = TRUE;
    }
    Permit();

    return(res);

    AROS_LIBFUNC_EXIT

} /* secContextLocate */

