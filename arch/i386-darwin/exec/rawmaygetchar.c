/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Check if there is a character on the raw console
    Lang: english
*/

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>
#include <proto/arossupport.h>

	AROS_LH0(LONG, RawMayGetChar,

/*  LOCATION */
	struct ExecBase *, SysBase, 85, Exec)

/*  FUNCTION
	Check if there is a character on the raw console.

    INPUTS
	None.

    RESULT
	The character or -1 if there was none.

    NOTES
	This function is for very low level debugging only.

    EXAMPLE

    BUGS

    SEE ALSO
	RawIOInit(), RawPutChar(), RawMayGetChar()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
//is this ever used? maybe for "kernel debugging?
    return -1;
    AROS_LIBFUNC_EXIT
} /* RawMayGetChar */
