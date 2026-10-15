/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <proto/kernel.h>

/* Internal: pretty-print one PC using resolver if present */
static VOID krnBacktraceSingle(struct KernelBase *KernelBase, APTR pc)
{
    struct KrnSymInfo info = {0};
    KrnSymResolver_t resolver;
    APTR priv;
    LONG resolved = 0;

    /*
     * Trap paths must not wait behind resolver registration changes.
     * If the pair is being updated, print the raw address instead.
     */
    if (!KrnSpinTryLock(&KernelBase->kb_gResolverSpinLock, SPINLOCK_MODE_READ))
    {
        bug("[Kernel]  %p\n", pc);
        return;
    }

    resolver = KernelBase->kb_gResolver;
    priv = KernelBase->kb_gResolvPrivate;

    if (resolver)
        resolved = resolver(priv, pc, &info);

    if (resolved)
    {
        IPTR off = 0;

        if (info.sym_start)
            off = (IPTR)pc - (IPTR)info.sym_start;

        if (info.sym_name)
        {
            bug("[Kernel]  %p  %s+0x%lx (%s%s%s)\n",
                pc,
                info.sym_name, (ULONG)off,
                info.mod_name ? (char *)info.mod_name : "",
                info.seg_name ? ":" : "",
                info.seg_name ? (char *)info.seg_name : "");
        }
        else if (info.mod_name)
        {
            bug("[Kernel]  %p  (%s%s%s+0x%lx)\n",
                pc,
                info.mod_name,
                info.seg_name ? ":" : "",
                info.seg_name ? (char *)info.seg_name : "",
                info.seg_start ? (ULONG)((IPTR)pc - (IPTR)info.seg_start) : 0);
        }
        else
        {
            bug("[Kernel]  %p\n", pc);
        }
    }
    else
    {
        bug("[Kernel]  %p\n", pc);
    }

    if (info.release)
        info.release(info.release_cookie);

    KrnSpinUnLock(&KernelBase->kb_gResolverSpinLock);
}

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH3(void, KrnPrintBacktrace,

/*  SYNOPSIS */
        AROS_LHA(const STRPTR, prefix, A0),
        AROS_LHA(APTR *, pcs, A1),
        AROS_LHA(ULONG, depth, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 70, Kernel)

/*  FUNCTION
        Prints a formatted stack backtrace to the kernel debug output. Each
        entry in the provided address list is optionally resolved to a module
        and symbol name using the currently registered symbol resolver (if any).

        The output includes one line per program counter, showing the address
        and, when available, the function or symbol name with offset.

    INPUTS
        prefix - Optional string prefix printed before the "Backtrace" header.
                 If NULL, "[Kernel] " is used.
        pcs    - Pointer to an array of program counter values representing the
                 captured call stack.
        depth  - Number of valid entries in the pcs array.

    RESULT
        None

    NOTES
        The function relies on an optional symbol resolver that can be
        registered with KrnRegisterSymResolver(). If no resolver is present,
        only raw addresses are printed.

        This function is safe to call from exception and trap context.

    EXAMPLE
        APTR pcs[64];
        ULONG depth = KrnBacktraceFromFrame((APTR)__builtin_frame_address(0), pcs, 64);
        KrnPrintBacktrace("[Crash] ", pcs, depth);

    BUGS
        None known.

    SEE ALSO
        KrnBacktraceFromFrame(), KrnRegisterSymResolver(), KrnUnregisterSymResolver()

    INTERNALS
        The resolver/private pair is consumed under a non-blocking read guard.
        If a resolver returns a result lease, it is released only after the
        returned pointers have been consumed.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    bug("%sBacktrace (%lu frames):\n",
        prefix ? (char *)prefix : "[Kernel] ", (ULONG)depth);

    for (ULONG i = 0; i < depth; ++i)
        krnBacktraceSingle(KernelBase, pcs[i]);
    AROS_LIBFUNC_EXIT
}
