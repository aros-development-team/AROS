/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_cpu.h"
#include "kernel_intern.h"
#include "kernel_syscall.h"

/*
 * Caller's mode matters: MRC to MPIDR (c0,c0,5) is privileged on
 * ARMv7-A and undefined in user mode. The previous implementation
 * always took a SC_SUPERSTATE trip, which has the side effect of
 * leaving the caller in SYSTEM mode and then forcibly cps'ing back
 * to USER - silently flipping SVC callers to USER and breaking any
 * subsequent privileged ops (inline GetCPUNumber, etc).
 *
 * Branch on the actual mode: in USER, SWI to a dedicated handler
 * (SC_GETCPUNUMBER) that reads MPIDR in SVC and returns the value.
 * In any privileged mode, do the MRC inline - no mode change.
 */
AROS_LH0(cpuid_t, KrnGetCPUNumber,
         struct KernelBase *, KernelBase, 41, Kernel)
{
    AROS_LIBFUNC_INIT

    uint32_t cpsr;

    asm volatile ("mrs %0, cpsr" : "=r" (cpsr));

    if ((cpsr & 0x1f) == 0x10)
    {
        register cpuid_t ret asm("r0");
        asm volatile ("swi %[swi_no]"
            : "=r" (ret)
            : [swi_no] "I" (SC_GETCPUNUMBER)
            : "lr", "memory");
        return ret;
    }

    return (cpuid_t)GetCPUNumber();

    AROS_LIBFUNC_EXIT
}
