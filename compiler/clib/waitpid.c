/*
    Copyright (C) 2004, The AROS Development Team. All rights reserved.
    $Id: wait.c 21894 2008-01-11 09:02:07Z agreppin $
*/

#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

pid_t waitpid(pid_t pid, int *status, int options)
{
    struct ETask *et;
    APTR tid = (APTR)pid;
    pid_t ret = -1;

    if (pid < 0)
	tid = 0;

    if (options & ~WNOHANG)
    {
	/* option not yet supported */
	errno = EINVAL;
    }
    else if (options & WNOHANG)
    {
	switch (ChildStatus(tid))
	{
	case CHILD_EXITED:
	    ret = 0;
	    break;
	default:
	    errno = ECHILD;
	    break;
	}
    }
    else
    {
	et = (struct ETask *)ChildWait(tid);
	if (et)
	{
	    ChildFree(et->et_UniqueID);
	    ret = 0;
	}
	else
	    errno = ECHILD;
    }
    
    return ret;
}
