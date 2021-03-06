/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.
*/

/*
 * Include fcntl.h before AROS includes, because __unused as a macro in AROS,
 * causing conflicts with __unused being a structure member name in Linux bits/stat.h.
 */
#include <fcntl.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

#ifndef _POSIX_C_SOURCE
/* On Darwin this definition is required by unistd.h
 * (which is marked as deprecated without the flag)
  */
#define _POSIX_C_SOURCE 200112L
#endif
#include <unistd.h>

AROS_LH0(int, KrnObtainInput,
          struct KernelBase *, KernelBase, 33, Kernel)
{
    AROS_LIBFUNC_INIT

    int res;

    /* Set our STDERR to non-blocking mode for RawMayGetChar() to work */
    res = KernelBase->kb_PlatformData->iface->fcntl(STDERR_FILENO, F_GETFL);
    AROS_HOST_BARRIER
    res = KernelBase->kb_PlatformData->iface->fcntl(STDERR_FILENO, F_SETFL, res|O_NONBLOCK);
    AROS_HOST_BARRIER

    return (res == -1) ? 0 : 1;

    AROS_LIBFUNC_EXIT
}
