/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/tasks.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include "kernel_riscv.h"
#include "kernel_cpu.h"
#if defined(__AROSEXEC_SMP__)
#include "kernel_ipi.h"
#endif

#include <proto/kernel.h>

/* See rom/kernel/schedulecpu.c for documentation */

AROS_LH1(void, KrnScheduleCPU,
        AROS_LHA(void *, cpu_mask, A0),
        struct KernelBase *, KernelBase, 47, Kernel)
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    ULONG mask[RISCV_CPUMASK_WORDS];

    /*
     * The sentinels resolve against the CPUs that are actually online.
     * The calling CPU is posted to like any other, as on aarch64: the
     * software interrupt stays pending while it runs Disable()d, so the
     * reschedule happens once that is lifted rather than right here.
     */
    krnRVResolveMask(cpu_mask, krnRVCPUNumber(), mask);

    core_SendIPI(IPI_SCHEDULE, mask);
#else
    (void)cpu_mask;

    krnSysCall(SC_SCHEDULE);
#endif

    AROS_LIBFUNC_EXIT
}
