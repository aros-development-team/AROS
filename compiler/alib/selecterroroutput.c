/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	BPTR SelectErrorOutput(

/*  SYNOPSIS */
	BPTR fh)

/*  FUNCTION
	Sets the current error stream returned by ErrorOutput() to a new
	value. Returns the old error stream.

    INPUTS
	fh - New error stream.

    RESULT
	Old error stream handle.

    NOTES
	This function is source-compatible with AmigaOS v4.
	
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    BPTR old;
    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);

    /* Nothing spectacular */
    old = me->pr_CES;
    me->pr_CES = fh;

    return old;
}
