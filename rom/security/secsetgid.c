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
	AROS_LH1(int, secsetgid,

/*  SYNOPSIS */
	/* (gid) */
	AROS_LHA(UWORD, gid, D0),

/*  LOCATION */
	struct SecurityBase *, secBase, 42, Security)

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
    if ((tasknode = FindTaskNode(secBase, FindTask(NULL))) || (tasknode = CreateOrphanTask(secBase, FindTask(NULL), DEFPROTECTION)))
    {
            if (tasknode->Owner){
                    if (tasknode->Owner->uid == secROOT_UID)
                    {
                            tasknode->RealGID = gid;
                            tasknode->SavedGID = gid;
                            tasknode->Owner->gid = gid;
                            rc = 0;
                    }
                    else if (tasknode->RealGID == gid || tasknode->SavedGID == gid)
                    {
                            tasknode->Owner->gid = gid;
                            rc = 0;
                    }
            }
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return rc;

    AROS_LIBFUNC_EXIT

} /* secsetgid */

