/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Emit one character via raw IO
    Lang: english
*/

#include <aros/asmcall.h>
#include <aros/libcall.h>

#include "exec_intern.h"
#include <proto/exec.h>

/* FIXME: Cross-library call! */
AROS_LD1(void, KrnPutChar,
    	 AROS_LDA(char, chr, D0),
         void *, KernelBase, 25, Kernel);

/****************************************************************************

    NAME */

	AROS_LH1(void, RawPutChar,

/*  SYNOPSIS */
	AROS_LHA(UBYTE, chr, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 86, Exec)

/*  FUNCTION
	Emits a single character into low-level debug output stream

    INPUTS
	chr - The character to emit

    RESULT
	None.

    NOTES
	This function is for very low level debugging only.

	Zero bytes are ignored by this function.

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
    	AROS_CALL1(void, AROS_SLIB_ENTRY(KrnPutChar, Kernel),
    		   AROS_LHA(char, chr, D0),
		   void *, KernelBase);

    AROS_LIBFUNC_EXIT
} /* RawPutChar */
