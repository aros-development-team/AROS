/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: KrnBacktraceFromFrame() - AArch64 frame-pointer stack walk.
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
    /*
     * AAPCS64 frame record: at x29 (the frame pointer) lie two doublewords --
     * [0] the caller's saved frame pointer, [1] the saved return address (x30).
     * Identical in shape to the x86_64 rbp chain, so the walk is the same.
     */
    IPTR *fp = (IPTR *)frame_in;

    while (fp && n < max_depth) {
        IPTR saved_fp = fp[0];
        IPTR ret      = fp[1];

        if (!ret)
            break;

        out_pcs[n++] = (APTR)ret;

        /* Monotonicity & 16-byte alignment to avoid loops/corruption */
        if (saved_fp <= (IPTR)fp || (saved_fp & 0xF))
            break;

        fp = (IPTR *)saved_fp;
    }

    return n;

    AROS_LIBFUNC_EXIT
}
