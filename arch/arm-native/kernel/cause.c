/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: arm-native KrnCause.
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* Called directly: it takes no base */
extern int AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)(void);

/* See rom/kernel/cause.c for documentation */

AROS_LH0I(void, KrnCause,
    struct KernelBase *, KernelBase, 3, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * Kernel context (an interrupt handler) does not trap. The software
     * interrupt its caller has flagged in SysBase->SysFlags is run by the
     * handler's exit path when it returns to a task (core_ExitInterrupt),
     * or by the Enable() of the task that had interrupts off.
     */
    if (!AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)())
        krnSysCall(SC_CAUSE);

    AROS_LIBFUNC_EXIT
}
