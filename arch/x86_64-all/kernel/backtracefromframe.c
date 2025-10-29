/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH3(ULONG, KrnBacktraceFromFrame,
        AROS_LHA(APTR, frame_in, A0),
        AROS_LHA(APTR *, out_pcs, A1),
        AROS_LHA(ULONG, max_depth, D0),
        struct KernelBase *, KernelBase, 69, Kernel)
{
    AROS_LIBFUNC_INIT

    ULONG n = 0;
    IPTR *rbp = (IPTR *)frame_in;

    while (rbp && n < max_depth) {
        IPTR saved_rbp = rbp[0];
        IPTR ret       = rbp[1];

        if (!ret)
            break;

        out_pcs[n++] = (APTR)ret;

        /* Monotonicity & alignment to avoid loops/corruption */
        if (saved_rbp <= (IPTR)rbp || (saved_rbp & 0xF))
            break;

        rbp = (IPTR *)saved_rbp;
    }

    return n;

    AROS_LIBFUNC_EXIT
}
