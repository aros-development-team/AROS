/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>

#include <signal.h>

#include "kernel_base.h"
#include "kernel_intern.h"

#include <proto/kernel.h>

/* See rom/kernel/cli.c for documentation */

AROS_LH0(void, KrnCli,
    struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    /* This check is needed for early atomics on ARM <v6, which are Disable()/Enable()-based */
    if (KernelBase->kb_PlatformData->iface)
    {
        KernelBase->kb_PlatformData->iface->sigprocmask(SIG_BLOCK, &KernelBase->kb_PlatformData->sig_int_mask, NULL);
        AROS_HOST_BARRIER
    }

    AROS_LIBFUNC_EXIT
}
