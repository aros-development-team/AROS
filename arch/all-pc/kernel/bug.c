/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <stdarg.h>
#include <stdio.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <proto/kernel.h>

AROS_LH2(int, KrnBug,
        AROS_LHA(const char *, format, A0),
        AROS_LHA(va_list, args, A1),
        struct KernelBase *, KernelBase, 12, Kernel)
{
    AROS_LIBFUNC_INIT

    int retval;
#if defined(DEBUG_USEATOMIC)
    unsigned long flags = 0;

    if (_arosdebuglock & 1)
    {
        __save_flags(flags);
        __cli();
    }
#endif

    retval = krnBug(format, args, KernelBase);

#if defined(DEBUG_USEATOMIC)
    if (_arosdebuglock & 1)
    {
        __restore_flags(flags);
    }
#endif
    return retval;

    AROS_LIBFUNC_EXIT
}
