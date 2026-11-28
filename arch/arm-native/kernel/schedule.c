/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: arm-native KrnSchedule.
*/

#include <aros/kernel.h>
#include <exec/execbase.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

/* The scheduling flags are per-CPU TLS */
#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#include <proto/kernel.h>

/* Called directly: it takes no base */
extern int AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)(void);

/* See rom/kernel/schedule.c for documentation */

AROS_LH0(void, KrnSchedule,
    struct KernelBase *, KernelBase, 6, Kernel)
{
    AROS_LIBFUNC_INIT

    if (AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)())
    {
        /*
         * Kernel context (an interrupt handler) does not trap and does
         * not switch tasks itself. It leaves the request pending: the
         * handler's exit path runs the scheduler when it returns to a
         * task (core_ExitInterrupt). The flag is also set by the FIQ
         * handlers, so it is changed with both interrupts masked.
         */
        unsigned int __if = EXEC_IRQFIQ_DISABLE();

        FLAG_SCHEDSWITCH_SET;
        EXEC_IRQFIQ_RESTORE(__if);
    }
    else
        krnSysCall(SC_SCHEDULE);

    AROS_LIBFUNC_EXIT
}
