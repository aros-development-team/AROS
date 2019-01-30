/*
    Copyright � 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0(cpuid_t, KrnGetCPUCount,
	 struct KernelBase *, KernelBase, 40, Kernel)
{
    AROS_LIBFUNC_INIT

    cpuid_t count;

    count = __arm_arosintern.ARMI_AffinityMask
        - ((__arm_arosintern.ARMI_AffinityMask >> 1) & 0x55555555);
    count = (count & 0x33333333) + ((count >> 2) & 0x33333333);
    return (((count + (count >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;

    return count;

    AROS_LIBFUNC_EXIT
}
