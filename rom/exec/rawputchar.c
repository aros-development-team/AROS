/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Emit one character via raw IO
    Lang: english
*/
#include <unistd.h>

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(void, RawPutChar,

/*  SYNOPSIS */
	AROS_LHA(UBYTE, chr, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 86, Exec)

/*  FUNCTION
	Emits a single character.

    INPUTS
	chr - The character to emit

    RESULT
	None.

    NOTES
	This function is for very low level debugging only.

    EXAMPLE

    BUGS

    SEE ALSO
	RawIOInit(), RawPutChar(), RawMayGetChar()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Don't write 0 bytes */
    if (chr)
    {
	/* Write char to stderr */
	write (STDERR_FILENO, &chr, 1);

	/* Make sure it makes it to the user. Slow but safe. */
	fsync (STDERR_FILENO);
    }

    AROS_LIBFUNC_EXIT
} /* RawPutChar */
