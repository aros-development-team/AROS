/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <exec/tasks.h>
#include <proto/alib.h>

	void DeleteTask (

/*  SYNOPSIS */
	struct Task * task)

/*  FUNCTION
	Get rid of a task which was created by CreateTask().

    INPUTS
	task - The task which was created by CreateTask(). Must be
	    non-NULL.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemTask()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_GET_SYSBASE_OK
    RemTask (task);
} /* DeleteTask */

