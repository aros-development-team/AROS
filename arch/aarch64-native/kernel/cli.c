/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* See rom/kernel/cli.c for documentation */

AROS_LH0I(void, KrnCli,
    struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    unsigned long spsel;

    asm volatile("mrs %0, spsel" : "=r"(spsel));

    if (spsel & 1)
    {
        /* Kernel context (EL1h): mask IRQ and FIQ here, no system call */
        asm volatile("msr daifset, #3" ::: "memory");
    }
    else
    {
        /* A task: the system call masks both in the task's saved state */
        krnSysCall(SC_CLI);
    }

    AROS_LIBFUNC_EXIT
}
