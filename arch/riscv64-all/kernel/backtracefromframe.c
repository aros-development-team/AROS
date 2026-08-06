/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

/*
 * RISC-V AROS is built with -fomit-frame-pointer, so there is no frame
 * chain to walk. Instead the stack is scanned conservatively: every
 * word that the registered symbol resolver can attribute to a module is
 * reported as a possible return address. This overreports - a stale
 * code pointer left on the stack is indistinguishable from a live
 * return address - but every frame of the real call chain is in the
 * result, each one nameable.
 */
#define SCAN_WORDS  512

AROS_LH3(ULONG, KrnBacktraceFromFrame,
        AROS_LHA(APTR, frame_in, A0),
        AROS_LHA(APTR *, out_pcs, A1),
        AROS_LHA(ULONG, max_depth, D0),
        struct KernelBase *, KernelBase, 69, Kernel)
{
    AROS_LIBFUNC_INIT

    KrnSymResolver_t resolver = KernelBase->kb_gResolver;
    IPTR *sp = (IPTR *)((IPTR)frame_in & ~(IPTR)7);
    ULONG n = 0, i;

    if (!resolver || !sp)
        return 0;

    for (i = 0; i < SCAN_WORDS && n < max_depth; i++)
    {
        IPTR val = sp[i];
        struct KrnSymInfo info = {0};

        /* Return addresses are 2-byte aligned at minimum */
        if (!val || (val & 1))
            continue;

        if (resolver(KernelBase->kb_gResolvPrivate, (APTR)val, &info))
            out_pcs[n++] = (APTR)val;
    }

    return n;

    AROS_LIBFUNC_EXIT
}
