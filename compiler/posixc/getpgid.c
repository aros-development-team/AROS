/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>

#include <exec/tasks.h>
#include <proto/exec.h>

#include <assert.h>

#include "__vfork.h"
#include "__posixc_intbase.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	pid_t getpgid(

/*  SYNOPSIS */
	pid_t pid)

/*  FUNCTION
	Returns the process group ID for the specified process with ID 'pid'.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
  return (pid_t) 0;
}

