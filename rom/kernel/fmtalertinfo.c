/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

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
