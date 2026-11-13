/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(BOOL, KrnCPUInMask,
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 46, Kernel)
{
    AROS_LIBFUNC_INIT

    return core_APIC_CPUInMask(id, mask);

    AROS_LIBFUNC_EXIT
}
