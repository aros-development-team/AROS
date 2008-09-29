/*
    Copyright (C) 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <proto/exec.h>
#include <exec/lists.h>
#include <aros/startup.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "etask.h"

/*****************************************************************************

    NAME */

	pid_t waitpid(

/*  SYNOPSIS */
	pid_t pid,
	int *status,
	int options)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct ETask *et;
    APTR tid = (APTR) pid;
    pid_t ret = -1;
    int exchildno;

    D(bug("waitpid(%d, %p, %d)\n", pid, status, options));

    et = GetETask(FindTask(NULL));
    if(!et)
    {
	/* only ETasks are fertile */
	errno = ECHILD;
	return -1;
    }
    
    if (pid < -1 || pid == 0)
    {
	/* process groups not yet supported */
	errno = EINVAL;
	return -1;
    }

    if (pid == -1)
	tid = 0;

    if(tid != 0 && ChildStatus(tid) == CHILD_NOTFOUND)
    {
	/* error if there's no such child */
	errno = ECHILD;
        return -1;
    }

    if (options & ~WNOHANG)
    {
	/* option not yet supported */
	errno = EINVAL;
	return -1;
    }
    
    /* not very pretty, perhaps we need a function for counting dead 
       children? */
    ListLength(&et->et_TaskMsgPort.mp_MsgList, exchildno);
    if ((options & WNOHANG) && (
	(tid != 0 && ChildStatus(tid) != CHILD_EXITED) ||
	(tid == 0 && exchildno == 0)
    ))
    {
	/* no dead children atm */
	return 0;
    }

    et = (struct ETask *)ChildWait(tid);
    if (et != CHILD_NOTNEW)
    {
        if(status && IntETask(et)->iet_startup)
        {
            struct aros_startup *startup = IntETask(et)->iet_startup;
            *status = startup->as_startup_error;
        }
        ret = et->et_UniqueID;
        ChildFree(et->et_UniqueID);
    }
    else
        errno = ECHILD;
    
    return ret;
}
