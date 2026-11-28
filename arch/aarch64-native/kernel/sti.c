/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* See rom/kernel/sti.c for documentation */

AROS_LH0I(void, KrnSti,
    struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    unsigned long spsel;

    asm volatile("mrs %0, spsel" : "=r"(spsel));

    if (spsel & 1)
    {
        /* Kernel context (EL1h): open IRQ and FIQ here, no system call */
        asm volatile("msr daifclr, #3" ::: "memory");
    }
    else
    {
        /* A task: the system call opens both in the task's saved state */
        krnSysCall(SC_STI);
    }

    AROS_LIBFUNC_EXIT
}
