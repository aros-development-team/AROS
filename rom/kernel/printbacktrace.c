/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/* Internal: pretty-print one PC using resolver if present */
static VOID krnBacktraceSingle(APTR priv, APTR pc, KrnSymResolver_t resolver)
{
    if (!resolver) {
        bug("[Kernel]  %p\n", pc);
        return;
    }

    struct KrnSymInfo info = {0};
    if (resolver(priv, pc, &info)) {
        IPTR off = 0;
        if (info.sym_start)
            off = (IPTR)pc - (IPTR)info.sym_start;

        if (info.sym_name) {
            bug("[Kernel]  %p  %s+0x%lx (%s%s%s)\n",
                pc,
                info.sym_name, (ULONG)off,
                info.mod_name ? (char *)info.mod_name : "",
                info.seg_name ? ":" : "",
                info.seg_name ? (char *)info.seg_name : "");
        } else if (info.mod_name) {
            bug("[Kernel]  %p  (%s%s%s)\n",
                pc,
                info.mod_name,
                info.seg_name ? ":" : "",
                info.seg_name ? (char *)info.seg_name : "");
        } else {
            bug("[Kernel]  %p\n", pc);
        }
    } else {
        bug("[Kernel]  %p\n", pc);
    }
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
        The resolver is stored in a global kernel variable and accessed without
        locking. This function does not perform symbol resolution itself, but
        delegates to the resolver when available.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Read once; ok if resolver changes during print. */
    KrnSymResolver_t resolver = KernelBase->kb_gResolver;
    if (resolver) {
        bug("%sBacktrace (%lu frames):\n",
            prefix ? (char *)prefix : "[Kernel] ", (ULONG)depth);

        for (ULONG i = 0; i < depth; ++i)
            krnBacktraceSingle(KernelBase->kb_gResolvPrivate, pcs[i], resolver);
    }
    AROS_LIBFUNC_EXIT
}
