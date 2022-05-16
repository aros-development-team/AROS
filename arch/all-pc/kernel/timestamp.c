/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH0(UQUAD, KrnTimeStamp,
        struct KernelBase *, KernelBase, 64, Kernel)
{
    AROS_LIBFUNC_INIT

    return RDTSC();

    AROS_LIBFUNC_EXIT
}
