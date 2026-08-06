/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(intptr_t, KrnGetCPUAttr,

/*  SYNOPSIS */
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(uint32_t, cpu, D1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 71, Kernel)

/*  FUNCTION
        Get the value of a per-CPU attribute, for the given CPU.
        Currently defined attributes are:

          KATTR_CPULoad          [.G] (ULONG) - Load of the CPU, 0 - 0xffffffff.

          KATTR_CPUFrequencyKHz  [.G] (IPTR)  - Current core clock, in kHz.

    INPUTS
        id  - ID of the attribute to get
        cpu - CPU number, 0 .. KrnGetCPUCount() - 1

    RESULT
        Value of the attribute, or -1 when the attribute (or the CPU) is
        not known.

    NOTES
        This supersedes querying KrnGetSystemAttr() with the
        KATTR_CPULoad + n attribute range, which cannot scale beyond the
        range's size.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetSystemAttr()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return -1;

    AROS_LIBFUNC_EXIT
}
