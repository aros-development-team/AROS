/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

/* See rom/kernel/issuper.c for documentation */

AROS_LH0I(int, KrnIsSuper,
    struct KernelBase *, KernelBase, 13, Kernel)
{
    AROS_LIBFUNC_INIT

    unsigned long spsel;

    /* Tasks run at EL1t (SPSel=0); handlers and SuperState() at EL1h */
    asm volatile("mrs %0, spsel" : "=r"(spsel));

    return spsel & 1;

    AROS_LIBFUNC_EXIT
}
