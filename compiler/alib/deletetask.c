/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
#include <exec/tasks.h>
#include <clib/aros_protos.h>

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
    RemTask (task);
} /* DeleteTask */

