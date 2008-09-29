/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function getpid().
*/

#include <proto/exec.h>
#include <assert.h>

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
    struct ETask *et;

    et = GetETask(FindTask(NULL));
    assert(et); 
    return (pid_t) et->et_UniqueID;
} /* getpid */
