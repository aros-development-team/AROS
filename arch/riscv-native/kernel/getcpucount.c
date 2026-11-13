/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Number of CPUs running AROS.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <kernel_base.h>

#include "kernel_riscv.h"

#include <proto/kernel.h>

/* See rom/kernel/getcpucount.c for documentation */

AROS_LH0(unsigned int, KrnGetCPUCount,
         struct KernelBase *, KernelBase, 40, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * Harts that are online, not the ones the firmware describes: a
     * hart that never started can not run anything, and callers size
     * per-CPU resources with this (e.g. nvme queues). It only grows
     * while secondaries come up, then stays fixed.
     */
    return __riscv_cpu_count;

    AROS_LIBFUNC_EXIT
}
