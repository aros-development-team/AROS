/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/alib.h>

	BPTR ErrorOutput(

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Returns the current error stream or 0 if there is no current
	input stream.

    INPUTS

    RESULT
	Error stream handle.

    NOTES
	This function is source-compatible with AmigaOS v4.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Nothing spectacular */
    return me->pr_CES;
}
