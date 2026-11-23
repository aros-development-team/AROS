/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* See rom/kernel/schedule.c for documentation */

AROS_LH0(void, KrnSchedule,
    struct KernelBase *, KernelBase, 6, Kernel)
{
    AROS_LIBFUNC_INIT

    unsigned long spsel;

    asm volatile("mrs %0, spsel" : "=r"(spsel));

    if (spsel & 1)
    {
        /*
         * Kernel context (EL1h): no system call from here. The switch is
         * left pending, and the interrupt exit that returns to a task
         * carries it out (core_ExitInterrupt).
         */
        TLS_PTR_GET()->ScheduleFlags |= TLSSF_Switch;
    }
    else
    {
        krnSysCall(SC_SCHEDULE);
    }

    AROS_LIBFUNC_EXIT
}
