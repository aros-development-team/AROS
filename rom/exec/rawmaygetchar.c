/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Check if there is a character on the raw console
    Lang: english
*/

#include <proto/kernel.h>

#include "exec_intern.h"

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>

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
	RawIOInit(), RawPutChar()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return KrnMayGetChar();

    AROS_LIBFUNC_EXIT
} /* RawMayGetChar */
