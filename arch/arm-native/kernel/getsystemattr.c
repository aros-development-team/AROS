/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include "kernel_intern.h"

#include <proto/kernel.h>

AROS_LH1(intptr_t, KrnGetSystemAttr,
    AROS_LHA(uint32_t, id, D0),
    struct KernelBase *, KernelBase, 29, Kernel)
{
    AROS_LIBFUNC_INIT

    switch (id)
    {
    case KATTR_Architecture:
        return (intptr_t)"arm-raspi";

    case KATTR_PeripheralBase:
        return (intptr_t)__arm_arosintern.ARMI_PeripheralBase;

    case KATTR_AffinityMask:
        return (intptr_t)__arm_arosintern.ARMI_AffinityMask;

    default:
        return -1;
    }

    AROS_LIBFUNC_EXIT
}
