/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: KrnGetSystemAttr() - AArch64.

    The generic rom/kernel version returns -1 for KATTR_PeripheralBase, so the
    BCM2708 SoC drivers (USB/SD/i2c/mbox) that derive their MMIO base from it
    (__arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase)) would access a
    bogus address and fail to probe their hardware. Provide the arm-native
    behaviour here.
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
        return (intptr_t)"aarch64-raspi";

    case KATTR_PeripheralBase:
        return (intptr_t)__arm_arosintern.ARMI_PeripheralBase;

    case KATTR_AffinityMask:
        return (intptr_t)__arm_arosintern.ARMI_AffinityMask;

    default:
        return -1;
    }

    AROS_LIBFUNC_EXIT
}
