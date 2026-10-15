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

        AROS_LH1(LONG, KrnUnregisterSymResolver,

/*  SYNOPSIS */
        AROS_LHA(KrnSymResolver_t, resolver, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 68, Kernel)

/*  FUNCTION
        Unregisters the previously installed kernel symbol resolver callback.

        The resolver function is used by the kernel to translate instruction
        addresses into module and symbol information when printing diagnostic
        or crash reports. Only one resolver can be active at a time.

    INPUTS
        resolver - The resolver callback previously registered with
                   KrnRegisterSymResolver(). If this value does not match
                   the currently active resolver, no action is taken.

    RESULT
        Returns 0 if the supplied resolver matches the currently registered
        resolver and is unregistered, -1 if it does not match, or -3 if
        the resolver registry is currently in use.

    NOTES
        The resolver is typically implemented by debug.library and used by the
        kernel to produce symbolic backtraces when handling traps or exceptions.

        The function never waits for an active resolver reader. If called
        from a resolver that is currently in use on a guarded target, it
        returns -3; the caller may retry after the callback has returned.

    EXAMPLE
        if (KrnUnregisterSymResolver(old_resolver) == 0)
            KrnBug("Symbol resolver removed.\n");

    BUGS
        None known.

    SEE ALSO
        KrnRegisterSymResolver(), KrnPrintBacktrace(), KrnBacktraceFromFrame()

    INTERNALS
        The resolver/private pair is removed under the same non-blocking
        registry guard used by diagnostic readers.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!KrnSpinTryLock(&KernelBase->kb_gResolverSpinLock, SPINLOCK_MODE_WRITE))
        return -3;

    if (KernelBase->kb_gResolver != resolver)
    {
        KrnSpinUnLock(&KernelBase->kb_gResolverSpinLock);
        return -1;
    }

    KernelBase->kb_gResolver = NULL;
    KernelBase->kb_gResolvPrivate = NULL;

    KrnSpinUnLock(&KernelBase->kb_gResolverSpinLock);
    return 0;

    AROS_LIBFUNC_EXIT
}
