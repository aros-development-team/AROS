/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"
#include "security_task.h"

#include <libraries/mufs.h>

/*****************************************************************************

    NAME */
	AROS_LH1(int, secgetpgid,

/*  SYNOPSIS */
	/* (pid) */
	AROS_LHA(int, pid, D0),

/*  LOCATION */
	struct SecurityBase *, secBase, 46, Security)

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

    struct secTaskNode *tasknode;
    int rc = -1;
    ObtainSemaphore(&secBase->TaskOwnerSem);
    if (pid == 0)
            tasknode = FindTaskNode(secBase, FindTask(NULL));
    else
            tasknode = FindTaskNodePid(secBase, pid);
    if (tasknode->Session)
            rc = tasknode->Session->sid;
    else
            rc = 0;
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return rc;

    AROS_LIBFUNC_EXIT

} /* secgetpgid */

