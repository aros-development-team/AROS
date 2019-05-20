/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"
#include "security_task.h"

/*****************************************************************************

    NAME */
	AROS_LH1(struct secExtOwner *, secGetTaskExtOwner,

/*  SYNOPSIS */
	/* (task) */
	AROS_LHA(struct Task *, task, D0),

/*  LOCATION */
	struct SecurityBase *, secBase, 21, Security)

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

    if (!task)
        task = FindTask(NULL);

    return(GetTaskExtOwner(secBase, task));

    AROS_LIBFUNC_EXIT

} /* secGetTaskExtOwner */

