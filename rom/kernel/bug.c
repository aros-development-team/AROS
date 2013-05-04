/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <stdarg.h>
#include <stdio.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(int, KrnBug,

/*  SYNOPSIS */
	AROS_LHA(const char *, format, A0),
        AROS_LHA(va_list, args, A1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 12, Kernel)

/*  FUNCTION
	Output a formatted string to low-level debug output stream.

	The function supports the same set of formatting specifiers
	as standard C printf() function.

    INPUTS
	format - A format string
	args   - A list of arguments

    RESULT
	Number of succesfully printed characters

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

    return krnBug(format, args, KernelBase);

    AROS_LIBFUNC_EXIT
}
