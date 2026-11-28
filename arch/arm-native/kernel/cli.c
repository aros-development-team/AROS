/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* Called directly: it takes no base */
extern int AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)(void);

/* See rom/kernel/cli.c for documentation */

AROS_LH0I(void, KrnCli,
    struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * Both IRQ and FIQ are masked, in either case: the inter-CPU IPI is
     * an FIQ. Kernel context (an interrupt handler, the boot code) masks
     * them directly and does not trap; what it masks stays masked until
     * the handler returns, because Enable() opens nothing in supervisor
     * mode. A task runs unprivileged, where cpsid does nothing, and asks
     * with a system call.
     */
    if (AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)())
        asm volatile("cpsid if" ::: "memory");
    else
        krnSysCall(SC_CLI);

    AROS_LIBFUNC_EXIT
}
