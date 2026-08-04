/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

extern ULONG m68k_VoluntarySwitch(void);

/* See rom/kernel/switch.c for documentation */

AROS_LH0(void, KrnSwitch,
    struct KernelBase *, KernelBase, 5, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * Exec/Wait() has already queued the outgoing task.  Use the dedicated
     * 68000 voluntary-switch entry instead of the general scheduler-policy
     * path used by preemption and explicit rescheduling.
     */
    Supervisor(m68k_VoluntarySwitch);

    AROS_LIBFUNC_EXIT
}
