/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Hosted (unix/darwin) implementation of KrnAllocPages().

    The generic rom/kernel/allocpages.c serves pages out of the MMU page
    database (mm_AllocPages). The hosted ports never populate that database,
    so the generic function is a silent no-op that returns NULL. Here we back
    KrnAllocPages()/KrnFreePages() with the host's mmap()/munmap() instead.

    W^X note (Apple Silicon): a page may not be writable and executable at the
    same time. We therefore always hand back *writable* memory so the caller can
    populate it, and leave the transition to executable to a subsequent
    KrnSetProtection() call (host mprotect). With the process holding the
    com.apple.security.cs.allow-unsigned-executable-memory entitlement, that
    R/W -> R/X flip is permitted without MAP_JIT.
*/

#include <aros/libcall.h>
#include <aros/debug.h>

#include <inttypes.h>

#include <exec/memory.h>
#include <kernel_base.h>
#include <kernel_intern.h>

#include <sys/mman.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH3(void *, KrnAllocPages,

/*  SYNOPSIS */
        AROS_LHA(void *,     addr,   A0),
        AROS_LHA(uintptr_t,  length, D0),
        AROS_LHA(uint32_t,   flags,  D1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 27, Kernel)

/*  FUNCTION
        Allocate page-granular memory from the host. The region is always
        mapped readable/writable; pass it to KrnSetProtection() afterwards to
        make it executable or read-only.

    INPUTS
        addr   - Preferred start address, or NULL to let the host choose.
        length - Size in bytes (rounded up to the host page size).
        flags  - exec.library/AllocMem() style MEMF_* flags. Honoured for
                 intent only; the mapping itself is always R/W (see W^X note).

    RESULT
        Start address of the allocated region, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct KernelInterface *iface = KernelBase->kb_PlatformData->iface;
    IPTR    pageSize = KernelBase->kb_PageSize ? KernelBase->kb_PageSize : 4096;
    size_t  len      = (length + pageSize - 1) & ~(pageSize - 1);
    int     prot     = PROT_READ | PROT_WRITE;
    void   *map;

    /*
     * Always map R/W. Apple Silicon refuses a writable+executable mapping even
     * with the disable-executable-page-protection entitlement, so executable
     * callers populate the region R/W and then flip it to R/X with
     * KrnSetProtection() once the code is in place (see CreateSegList(),
     * internalloadseg_elf.c). (void)flags keeps the signature contract.
     */
    (void)flags;

    /* Darwin does not define MAP_ANONYMOUS, only MAP_ANON. */
    map = iface->mmap(addr, len, prot, MAP_PRIVATE | MAP_ANON, -1, 0);
    AROS_HOST_BARRIER

    if (map == MAP_FAILED)
    {
        int err = *iface->__error();
        AROS_HOST_BARRIER
        D(bug("[KrnAllocPages] FAILED: addr=%p len=%u flags=0x%x errno=%d\n",
              addr, (unsigned)len, (unsigned)flags, err));
        return NULL;
    }

    D(bug("[KrnAllocPages] addr=%p len=%u (req %u) flags=0x%x -> %p (R/W)\n",
          addr, (unsigned)len, (unsigned)length, (unsigned)flags, map));

    return map;

    AROS_LIBFUNC_EXIT
}
