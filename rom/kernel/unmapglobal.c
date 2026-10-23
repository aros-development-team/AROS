/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2I(int, KrnUnmapGlobal,

/*  SYNOPSIS */
        AROS_LHA(void *, virt, A0),
        AROS_LHA(uint32_t, length, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 17, Kernel)

/*  FUNCTION
        Unmap a region of virtual address space.

    INPUTS
        virt   - Starting virtual address of the region.
        length - Length of the region in bytes.

    RESULT
        Port specific. On x86_64, 0 on success and -1 on failure; on aarch64
        and riscv64, 0 on failure and a non-zero value on success. Callers
        that work across ports must not assume either; see KrnMapGlobal().

    NOTES
        The range is rounded out to whole pages; accessing it afterwards
        faults. Parts of the range that are not mapped are skipped.

        On x86_64 the local TLB is reloaded before the call returns and the
        other CPUs are asked to reload theirs. That request is asynchronous
        when the caller is inside Disable(), or when it was already in
        supervisor state, so on that path the other CPUs' TLBs are not
        updated before the call returns - the shootdown completes later.
        A 2 MiB mapping that is only partly unmapped is split into 4 KiB
        pages first; the page table this creates is not freed. A call that
        fails partway leaves the part of the range it had already processed
        unmapped.

        On architectures without an MMU, or where this is not implemented,
        the call does nothing and returns 0.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnMapGlobal()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}
