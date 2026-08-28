/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: System attributes for the opensbi-riscv64 target.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/platformtimer.h>

#include <kernel_base.h>

#include <proto/kernel.h>

#include "kernel_intern.h"

extern struct KrnPlatformTimer __platform_timer;

AROS_LH1(intptr_t, KrnGetSystemAttr,
          AROS_LHA(uint32_t, id, D0),
          struct KernelBase *, KernelBase, 29, Kernel)
{
    AROS_LIBFUNC_INIT

    switch (id)
    {
    case KATTR_Architecture:
        return (intptr_t)AROS_ARCHITECTURE;

    case KATTR_ClockSource:
        if (KernelBase->kb_ClockSource)
            return (intptr_t)KernelBase->kb_ClockSource;
        return -1;

    case KATTR_PlatformTimer:
        return (intptr_t)&__platform_timer;
    }

    return -1;

    AROS_LIBFUNC_EXIT
}
