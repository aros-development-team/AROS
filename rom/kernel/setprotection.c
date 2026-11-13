/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH3I(void, KrnSetProtection,

/*  SYNOPSIS */
        AROS_LHA(void *, address, A0),
        AROS_LHA(uint32_t, length, D0),
        AROS_LHA(KRN_MapAttr, flags, D1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 21, Kernel)

/*  FUNCTION
        Set the access permissions of a memory region.

    INPUTS
        address - Start of the region.
        length  - Length of the region in bytes.
        flags   - Requested access permissions, expressed as KRN_MapAttr
                  attributes.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}
