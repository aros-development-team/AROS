/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize raw IO
    Lang: english
*/

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, RawIOInit,

/*  LOCATION */
	struct ExecBase *, SysBase, 84, Exec)

/*  FUNCTION
	This is a private function. It initializes raw IO. After you
	have called this function, you can use RawMayGetChar() and
	RawPutChar().

    INPUTS
	None.

    RESULT
	None.

    NOTES
	This function is for very low level debugging only.

    EXAMPLE

    BUGS

    SEE ALSO
	RawPutChar(), RawMayGetChar()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

    return;
    AROS_LIBFUNC_EXIT
} /* RawIOInit */
