/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(void, KrnFmtAlertInfo,

/*  SYNOPSIS */
        AROS_LHA(STRPTR *, TemplatePtr, A0),
        AROS_LHA(IPTR *, ParamPtr, A1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 65, Kernel)

/*  FUNCTION
        Supply architecture-specific details for an alert message.

    INPUTS
        TemplatePtr - Pointer to storage for an alert format string.
        ParamPtr    - Pointer to storage for its format parameters.

    RESULT

    NOTES
        The generic implementation leaves both outputs unchanged.
        The all-pc implementation may replace the template and two
        parameter entries when an interrupt context is available.
        ParamPtr must provide space for at least two IPTR values.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnDisplayAlert()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * The implementation of this function may be architecture-specific.
     * by default this is a no-op.
     */

    return;

    AROS_LIBFUNC_EXIT
}
