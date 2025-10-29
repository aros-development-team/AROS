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
        Returns non-zero (TRUE) if the resolver was successfully unregistered,
        or zero (FALSE) if no resolver was active or the supplied resolver did
        not match the currently registered one.

    NOTES
        The resolver is typically implemented by debug.library and used by the
        kernel to produce symbolic backtraces when handling traps or exceptions.

        The function performs no locking and is safe to call from within the
        resolver itself, or from kernel exception context.

    EXAMPLE
        if (KrnUnregisterSymResolver(old_resolver))
            KrnBug("Symbol resolver removed.\n");

    BUGS
        None known.

    SEE ALSO
        KrnRegisterSymResolver(), KrnPrintBacktrace(), KrnBacktraceFromFrame()

    INTERNALS
        The kernel maintains a single global pointer to the resolver function.
        This function simply clears the pointer if it matches the one provided.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (KernelBase->kb_gResolver != resolver) {
        return -1;
    }

    KernelBase->kb_gResolver = NULL;
    KernelBase->kb_gResolvPrivate = NULL;

    return 0;

    AROS_LIBFUNC_EXIT
}
