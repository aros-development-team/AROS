/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: KrnTimeStamp() - architecture-specific monotonic timestamp.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>

#include "kernel_intern.h"

#include <proto/kernel.h>

/* See rom/kernel/timestamp.c for documentation. On arm-native this returns
 * the platform free-running timer in microseconds (BCM System Timer). */

AROS_LH0(UQUAD, KrnTimeStamp,
        struct KernelBase *, KernelBase, 64, Kernel)
{
    AROS_LIBFUNC_INIT

    if (__arm_arosintern.ARMI_GetTime)
        return (UQUAD)__arm_arosintern.ARMI_GetTime();

    return 0;

    AROS_LIBFUNC_EXIT
}
