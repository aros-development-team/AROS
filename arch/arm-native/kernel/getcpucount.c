/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0(unsigned int, KrnGetCPUCount,
	 struct KernelBase *, KernelBase, 36, Kernel)
{
    AROS_LIBFUNC_INIT

    uint32_t count = 0, mask;

    for (mask = __arm_affinitymask; mask > 0 ; mask >> 1)
    {
        if (mask & 1)
            count++;
    }
    return count;

    AROS_LIBFUNC_EXIT
}
