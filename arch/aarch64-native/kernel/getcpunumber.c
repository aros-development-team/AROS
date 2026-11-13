/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_cpu.h"

/* Tasks run at EL1t, so the TPIDR_EL1 read needs no syscall. */
AROS_LH0(cpuid_t, KrnGetCPUNumber,
         struct KernelBase *, KernelBase, 41, Kernel)
{
    AROS_LIBFUNC_INIT

    return (cpuid_t)GetCPUNumber();

    AROS_LIBFUNC_EXIT
}
