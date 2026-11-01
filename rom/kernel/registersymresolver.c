/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/atomic.h>
#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

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

    INPUTS
        resolver - Pointer to the resolver callback function. This function will
                   be called by the kernel with an instruction address and must
                   fill a KrnSymResolve structure with module and symbol
                   information.
        priv     - Optional user data pointer passed unmodified to the resolver
                   each time it is invoked.

    RESULT
        Returns 0 on success, or a negative value on error:
            -1 - resolver argument was NULL
            -2 - a resolver is already registered
            -3 - resolver registry is currently in use

    NOTES
        This API allows an external component (typically debug.library) to
        provide symbol resolution for kernel stack traces. Once registered,
        the resolver will be used automatically by KrnPrintBacktrace() and
        the kernel crash handler.

        Registration never waits for an active backtrace reader. On all-pc
        targets it returns -3 when the resolver pair cannot be changed without
        crossing a live reader lease.

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
        The resolver and private pointer form one registry pair. On all-pc,
        a generation plus reader count protects pair lifetime. SMP writers
        additionally serialize through a writer-only kernel spinlock; trap
        readers never acquire that spinlock.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!resolver)
        return -1;

#if defined(__i386__) || defined(__x86_64__)
    LONG result;

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&KernelBase->kb_gResolverSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif

    if (KernelBase->kb_gResolver != NULL)
        result = -2;
    else
    {
        AROS_ATOMIC_INC(KernelBase->kb_gResolverGeneration);
        if (KernelBase->kb_gResolverReaders != 0)
        {
            AROS_ATOMIC_INC(KernelBase->kb_gResolverGeneration);
            result = -3;
        }
        else
        {
            KernelBase->kb_gResolver = resolver;
            KernelBase->kb_gResolvPrivate = priv;
            AROS_ATOMIC_INC(KernelBase->kb_gResolverGeneration);
            result = 0;
        }
    }

#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&KernelBase->kb_gResolverSpinLock);
#endif
    Enable();
    return result;
#else
    if (KernelBase->kb_gResolver != NULL)
        return -2;

    KernelBase->kb_gResolver = resolver;
    KernelBase->kb_gResolvPrivate = priv;
    return 0;
#endif

    AROS_LIBFUNC_EXIT
}
