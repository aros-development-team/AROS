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
	AROS_LH1(BOOL, secFreeze,

/*  SYNOPSIS */
	/* (task) */
	AROS_LHA(struct Task *, task, D0),

/*  LOCATION */
	struct SecurityBase *, secBase, 31, Security)

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
    struct secExtOwner *xowner;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    xowner = GetTaskExtOwner(secBase, FindTask(NULL));
    if (task && (task != FindTask(NULL)) && 
                    (task != (struct Task*)secBase->Server) &&
             (secGetRelationshipA(xowner, 0, NULL) & secRelF_ROOT_UID)) {
        Disable();
        switch (task->tc_Node.ln_Type) {
            case NT_TASK:
            case NT_PROCESS:
                    if (task->tc_State < 7) {
                        Remove((struct Node*)task);
                        AddHead((struct List *)&secBase->Frozen, (struct Node*)task);
                        task->tc_State += 7;
                        res = TRUE;
                    }
                    break;
        }
        Enable();
    }
    secFreeExtOwner(xowner);
    return(res);

    AROS_LIBFUNC_EXIT

} /* secFreeze */

