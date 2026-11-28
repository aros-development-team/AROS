/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* Called directly: it takes no base */
extern int AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)(void);

/* See rom/kernel/sti.c for documentation */

AROS_LH0I(void, KrnSti,
    struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * The counterpart of KrnCli(): IRQ and FIQ both. Kernel context (the
     * software interrupt dispatcher between two handlers) opens them
     * directly and does not trap; a task asks with a system call.
     */
    if (AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)())
        asm volatile("cpsie if" ::: "memory");
    else
        krnSysCall(SC_STI);

    AROS_LIBFUNC_EXIT
}
