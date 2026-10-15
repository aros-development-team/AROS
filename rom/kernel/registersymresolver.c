/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(LONG, KrnRegisterSymResolver,

/*  SYNOPSIS */
        AROS_LHA(KrnSymResolver_t, resolver, A0),
        AROS_LHA(APTR, priv, A1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 67, Kernel)

/*  FUNCTION
        Registers a global kernel symbol resolver callback used for decoding
        instruction addresses into symbolic names.

        The resolver is used by the kernel when printing diagnostic backtraces
        or exception dumps. Only one resolver can be active at any time. The
        function fails if a resolver is already registered.
            -3 - resolver registry is busy

    INPUTS
        resolver - Pointer to the resolver callback function. This function will
                   be called by the kernel with an instruction address and must
                   fill a KrnSymResolve structure with module and symbol
                   information.
        priv     - Optional user data pointer passed unmodified to the resolver
                   each time it is invoked.

    RESULT
        Returns 0 on success, or a negative value on error:
            -1 — resolver argument was NULL
            -2 — a resolver is already registered

    NOTES
        This API allows an external component (typically debug.library) to
        provide symbol resolution for kernel stack traces. Once registered,
        the resolver will be used automatically by KrnPrintBacktrace() and
        the kernel crash handler.

        The function is safe to call from user or supervisor context.
        On guarded targets it never waits for an active resolver reader;
        -3 is returned instead and the caller may retry later.

    EXAMPLE
        static LONG MyResolver(APTR addr, struct KrnSymResolve *out, APTR priv)
        {
            return DecodeLocationA(addr, priv);
        }

        if (KrnRegisterSymResolver(MyResolver, DebugBase) == 0)
            KrnBug("Symbol resolver registered.\n");

    BUGS
        Only one resolver can be registered globally. There is currently no
        queuing or stacking mechanism.

    SEE ALSO
        KrnUnregisterSymResolver(), KrnPrintBacktrace(), KrnBacktraceFromFrame()

    INTERNALS
        The resolver/private pair is published under a non-blocking kernel
        spinlock guard. Diagnostic readers use the matching read guard.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!resolver)
        return -1;

    if (!KrnSpinTryLock(&KernelBase->kb_gResolverSpinLock, SPINLOCK_MODE_WRITE))
        return -3;

    if (KernelBase->kb_gResolver != NULL)
    {
        KrnSpinUnLock(&KernelBase->kb_gResolverSpinLock);
        return -2; /* already installed */
    }

    /* Publish the private context before making the callback visible. */
    KernelBase->kb_gResolvPrivate = priv;
    KernelBase->kb_gResolver = resolver;

    KrnSpinUnLock(&KernelBase->kb_gResolverSpinLock);
    return 0;

    AROS_LIBFUNC_EXIT
}
