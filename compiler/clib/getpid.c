/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function remove()
    Lang: english
*/

#include <proto/exec.h>

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

    HISTORY
	29.07.2002 falemagn created

******************************************************************************/
{
    return (pid_t)FindTask(NULL);
} /* remove */

