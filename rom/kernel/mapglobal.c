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

        AROS_LH4I(int, KrnMapGlobal,

/*  SYNOPSIS */
        AROS_LHA(void *, virt, A0),
        AROS_LHA(void *, phys, A1),
        AROS_LHA(uint32_t, length, D0),
        AROS_LHA(KRN_MapAttr, flags, D1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 16, Kernel)

/*  FUNCTION
        Map a physical memory region into the virtual address space.

    INPUTS
        virt   - Starting virtual address of the mapping.
        phys   - Starting physical address of the region.
        length - Length of the mapping in bytes.
        flags  - Mapping attributes specified by KRN_MapAttr.

    RESULT
        Port specific. On x86_64 and i386, 0 on success and -1 on failure; on
        aarch64 and riscv64, 0 on failure and a non-zero value on success.
        Callers that work across ports must not assume either; see BUGS.

    NOTES
        The range is rounded out to whole pages, and virt and phys must
        agree below the page size (4 KiB on most ports). Mapping over an
        existing mapping replaces it.

        On x86_64 the local TLB is reloaded before the call returns and the
        other CPUs are asked to reload theirs. That request is asynchronous
        when the caller is inside Disable(), or when it was already in
        supervisor state, so on that path the other CPUs' TLBs are not
        updated before the call returns - the shootdown completes later.

        On x86_64 page tables created by the call, including the 4 KiB table
        made when part of a 2 MiB mapping is replaced, are never freed: the
        cost is bounded at one table per 2 MiB region ever split.

        On x86_64 a call that fails partway leaves the part of the range it
        had already processed mapped.

        On architectures without an MMU, or where this is not implemented,
        the call does nothing and returns 0.

    EXAMPLE

    BUGS
        The return value convention is not consistent across ports: x86_64
        returns 0 on success, aarch64 and riscv64 return non-zero on success.
        A caller written for one convention misreads the other. Reconciling
        the ports is a separate change; until then, check the port you are
        building for.

    SEE ALSO
        KrnUnmapGlobal(), KrnVirtualToPhysical(), KrnSetProtection()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}
