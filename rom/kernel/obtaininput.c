/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0I(int, KrnObtainInput,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 33, Kernel)

/*  FUNCTION
	Take over low-level debug input hardware and initialize the input

    INPUTS
	None

    RESULT
	Nonzero for success, zero for failure (for example there's no input channel)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * The implementation of this function is entirely architecture-specific (at least for now).
     * In fact this is a placeholder for an advanced low-level I/O subsystem, which will allow
     * various hardware drivers (like ethernet) to provide their own debug I/O channels.
     * This will allow to perform remote debugging and read boot logs on machines without serial
     * ports, for example over Ethernet/USB/whatever.
     */
    return 0;

    AROS_LIBFUNC_EXIT
}
