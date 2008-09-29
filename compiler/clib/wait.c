/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <proto/exec.h>
#include <aros/startup.h>
#include <aros/debug.h>

#include "etask.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

pid_t wait(int *status)
{
    pid_t ret = -1;
    struct ETask *et;

    D(bug("wait()\n"));

    et = GetETask(FindTask(NULL));
    if(!et)
    {
	/* only ETasks are fertile */
	errno = ECHILD;
	return -1;
    }

    et = (struct ETask *)ChildWait(0);
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
