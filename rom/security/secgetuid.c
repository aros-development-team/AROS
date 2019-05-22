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
	AROS_LH0(UWORD, secgetuid,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct SecurityBase *, secBase, 39, Security)

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
    UWORD rc = secNOBODY_UID;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((tasknode = FindTaskNode(secBase, FindTask(NULL))) || (tasknode = CreateOrphanTask(secBase, FindTask(NULL), DEFPROTECTION)))
            rc = tasknode->RealUID;
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return rc;

    AROS_LIBFUNC_EXIT

} /* secgetuid */

