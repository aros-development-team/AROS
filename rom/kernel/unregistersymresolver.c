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
        Returns 0 on success, or a negative value on error:
            -1 - resolver does not match the stored callback
            -3 - resolver registry is currently in use

    NOTES
        The resolver is typically implemented by debug.library and used by the
        kernel to produce symbolic backtraces when handling traps or exceptions.

        Unregistration never waits for an active backtrace reader. On all-pc
        targets it returns -3 while a reader lease still protects the resolver
        callback and its private pointer.

    EXAMPLE
        if (old_resolver && KrnUnregisterSymResolver(old_resolver) == 0)
            KrnBug("Symbol resolver removed.\n");

    BUGS
        None known.

    SEE ALSO
        KrnRegisterSymResolver(), KrnPrintBacktrace(), KrnBacktraceFromFrame()

    INTERNALS
        The resolver callback and private pointer are cleared as one registry
        pair. On all-pc, generation and reader-count state prevents the pair
        from changing while KrnPrintBacktrace() still uses its snapshot.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if defined(__i386__) || defined(__x86_64__)
    LONG result;

    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&KernelBase->kb_gResolverSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif

    if (KernelBase->kb_gResolver != resolver)
        result = -1;
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
            KernelBase->kb_gResolver = NULL;
            KernelBase->kb_gResolvPrivate = NULL;
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
    if (KernelBase->kb_gResolver != resolver)
        return -1;

    KernelBase->kb_gResolver = NULL;
    KernelBase->kb_gResolvPrivate = NULL;
    return 0;
#endif

    AROS_LIBFUNC_EXIT
}
