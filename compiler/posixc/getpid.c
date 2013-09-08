/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function getpid().
*/

#include <proto/exec.h>
#include <assert.h>

#include "__vfork.h"
#include "__arosc_privdata.h"

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
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    struct ETask *et;

    if(aroscbase->acb_flags & PRETEND_CHILD)
    {
	struct vfork_data *udata = aroscbase->acb_vfork_data;
	et = GetETask(udata->child);
    }
    else
	et = GetETask(FindTask(NULL));
    assert(et); 
    return (pid_t) et->et_UniqueID;
} /* getpid */
