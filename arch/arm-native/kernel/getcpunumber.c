/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0(unsigned int, KrnGetCPUNumber,
	 struct KernelBase *, KernelBase, 37, Kernel)
{
    AROS_LIBFUNC_INIT

    uint32_t tmp;

    asm volatile (" mrc p15, 0, %0, c0, c0, 5 " : "=r" (tmp));

    if (tmp & (2 << 30))
    {
        return (tmp & 0x3);
    }

    // Uniprocessor System
    return 0;

    AROS_LIBFUNC_EXIT
}
