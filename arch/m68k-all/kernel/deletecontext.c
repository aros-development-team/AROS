/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Free a variable-sized m68k CPU context.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH1(void, KrnDeleteContext,
         AROS_LHA(void *, context, A0),
         struct KernelBase *, KernelBase, 19, Kernel)
{
    AROS_LIBFUNC_INIT

    FreeVec(context);

    AROS_LIBFUNC_EXIT
}
