/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1(void, KrnPutChar,

/*  SYNOPSIS */
	AROS_LHA(char, c, D0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 25, Kernel)

/*  FUNCTION
	Output a single character to low-level debug output stream

    INPUTS
	c - A character to output

    RESULT
	None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	During very early system startup this function can be called
	directly with KernelBase set to NULL. Architecture-specific
	implementations of this function need to take care of it.

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    krnPutC(c, KernelBase);

    AROS_LIBFUNC_EXIT
}
