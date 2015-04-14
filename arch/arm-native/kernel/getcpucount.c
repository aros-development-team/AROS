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

    uint32_t count;

     count = __arm_affinitymask - ((__arm_affinitymask >> 1) & 0x55555555);
     count = (count & 0x33333333) + ((count >> 2) & 0x33333333);
     return (((count + (count >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;

    return count;

    AROS_LIBFUNC_EXIT
}
