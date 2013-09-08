/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function getpid().
*/

#include <proto/exec.h>
#include <assert.h>

#include "__vfork.h"
#include "__posixc_intbase.h"

/*****************************************************************************

    NAME */
#include <sys/types.h>
#include <unistd.h>

	pid_t getpid (

/*  SYNOPSIS */
	)

/*  FUNCTION
	Returns the process ID of the calling process

    RESULT
	The process ID of the calling process.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct ETask *et;

    if(PosixCBase->flags & PRETEND_CHILD)
    {
	struct vfork_data *udata = PosixCBase->vfork_data;
	et = GetETask(udata->child);
    }
    else
	et = GetETask(FindTask(NULL));
    assert(et); 
    return (pid_t) et->et_UniqueID;
} /* getpid */
