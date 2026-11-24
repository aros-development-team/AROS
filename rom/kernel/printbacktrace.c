/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/atomic.h>
#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/* Acquire a stable resolver/private snapshot without blocking a trap reader. */
static BOOL krnResolverAcquire(struct KernelBase *KernelBase,
                               KrnSymResolver_t *resolver, APTR *priv)
{
#if defined(__i386__) || defined(__x86_64__)
    ULONG generation = KernelBase->kb_gResolverGeneration;

    if (generation & 1)
        return FALSE;

    AROS_ATOMIC_INC(KernelBase->kb_gResolverReaders);
    if ((KernelBase->kb_gResolverGeneration != generation) ||
        (KernelBase->kb_gResolverGeneration & 1))
    {
        AROS_ATOMIC_DEC(KernelBase->kb_gResolverReaders);
        return FALSE;
    }
#endif

    *resolver = KernelBase->kb_gResolver;
    *priv = KernelBase->kb_gResolvPrivate;
    return TRUE;
}

static VOID krnResolverRelease(struct KernelBase *KernelBase)
{
#if defined(__i386__) || defined(__x86_64__)
    AROS_ATOMIC_DEC(KernelBase->kb_gResolverReaders);
#else
    (void)KernelBase;
#endif
}

/* Internal: pretty-print one PC using resolver if present */
static VOID krnBacktraceSingle(APTR priv, APTR pc, KrnSymResolver_t resolver)
{
    struct KrnSymInfo info = {0};
    LONG resolved;

    if (!resolver) {
        bug("[Kernel]  %p\n", pc);
        return;
    }

    resolved = resolver(priv, pc, &info);
    if (resolved) {
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
            /* No symbol - the segment offset is what addr2line needs */
            bug("[Kernel]  %p  (%s%s%s+0x%lx)\n",
                pc,
                info.mod_name,
                info.seg_name ? ":" : "",
                info.seg_name ? (char *)info.seg_name : "",
                info.seg_start ? (ULONG)((IPTR)pc - (IPTR)info.seg_start) : 0);
        } else {
            bug("[Kernel]  %p\n", pc);
        }
    } else {
        bug("[Kernel]  %p\n", pc);
    }

    if (info.release)
        info.release(info.release_cookie);
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
        On all-pc, the resolver callback and private pointer are snapshotted
        under a non-blocking generation/reader lease. The lease spans the
        complete backtrace so unregister cannot invalidate either pointer
        while a frame is being resolved. A collision falls back to raw
        addresses. Trap readers never acquire the registry writer spinlock.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    KrnSymResolver_t resolver = NULL;
    APTR priv = NULL;
    BOOL resolverReader = krnResolverAcquire(KernelBase, &resolver, &priv);

    bug("%sBacktrace (%lu frames):\n",
        prefix ? (char *)prefix : "[Kernel] ", (ULONG)depth);

    for (ULONG i = 0; i < depth; ++i)
        krnBacktraceSingle(priv, pcs[i], resolverReader ? resolver : NULL);

    if (resolverReader)
        krnResolverRelease(KernelBase);
    AROS_LIBFUNC_EXIT
}
