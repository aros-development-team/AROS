/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/arm/cpucontext.h>

#include <kernel_base.h>

#include <proto/kernel.h>

/* See rom/kernel/issuper.c for documentation */

AROS_LH0I(int, KrnIsSuper,
    struct KernelBase *, KernelBase, 13, Kernel)
{
    AROS_LIBFUNC_INIT

    register unsigned int reg;
    asm volatile("mrs %[reg], cpsr" : [reg] "=r" (reg) );

    return !(((reg & CPUMODE_MASK) == CPUMODE_USER) || ((reg & CPUMODE_MASK) == CPUMODE_SYSTEM));

    AROS_LIBFUNC_EXIT
}
