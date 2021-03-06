/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.
*/

#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

#include <signal.h>

AROS_LH0(void, KrnCause,
          struct KernelBase *, KernelBase, 3, Kernel)
{
    AROS_LIBFUNC_INIT

    KernelBase->kb_PlatformData->iface->raise(SIGUSR2);
    AROS_HOST_BARRIER

    AROS_LIBFUNC_EXIT
}
