/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_intern.h"

extern int Kernel_13_KrnIsSuper(void);

static void protect_pages(void *address, uint32_t length, KRN_MapAttr flags)
{
    IPTR start;
    IPTR end;
    char present;
    char writable;
    char user;

    if (!length)
        return;

    start = (IPTR)address & ~(IPTR)(PAGE_SIZE - 1);
    end = ((IPTR)address + length + PAGE_SIZE - 1) & ~(IPTR)(PAGE_SIZE - 1);
    if (end <= start)
        return;

    present = (flags & (MAP_Readable | MAP_Writable | MAP_Executable)) ? 1 : 0;
    writable = (flags & MAP_Writable) ? 1 : 0;
    user = (flags & MAP_Supervisor) ? 0 : 1;

    core_ProtKernelArea(start, end - start, present, writable, user);
}

/* See rom/kernel/setprotection.c for documentation. */
AROS_LH3I(void, KrnSetProtection,
    AROS_LHA(void *, address, A0),
    AROS_LHA(uint32_t, length, D0),
    AROS_LHA(KRN_MapAttr, flags, D1),
    struct KernelBase *, KernelBase, 21, Kernel)
{
    AROS_LIBFUNC_INIT

    APTR ssp = NULL;

    if (!Kernel_13_KrnIsSuper()) {
        ssp = SuperState();
        if (!ssp)
            return;
    }

    protect_pages(address, length, flags);

    if (ssp)
        UserState(ssp);

    AROS_LIBFUNC_EXIT
}
