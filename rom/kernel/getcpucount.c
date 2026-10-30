/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH0(unsigned int, KrnGetCPUCount,

/*  SYNOPSIS */

/*  LOCATION */
        struct KernelBase *, KernelBase, 40, Kernel)

/*  FUNCTION
        Return the number of processors running AROS

    INPUTS
        None

    RESULT
        Number of running CPUs in this system, at least 1. It grows while
        secondary CPUs are brought up, then stays fixed; CPUs the
        hardware describes but that are not running are not counted.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return 1;

    AROS_LIBFUNC_EXIT
}
