/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: KrnMapGlobal() for the opensbi-riscv64 target.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include <asm/riscv64/mmu.h>

#include <kernel_base.h>
#include "kernel_intern.h"

#include <proto/kernel.h>

AROS_LH4I(int, KrnMapGlobal,
        AROS_LHA(void *, virt, A0),
        AROS_LHA(void *, phys, A1),
        AROS_LHA(uint32_t, length, D0),
        AROS_LHA(KRN_MapAttr, flags, D1),
        struct KernelBase *, KernelBase, 16, Kernel)
{
    AROS_LIBFUNC_INIT

    unsigned long perms = 0;

    D(bug("[Kernel] KrnMapGlobal(virt=%p, phys=%p, len=%08x, flags=%04x)\n",
          virt, phys, length, flags));

    if (flags & MAP_Readable)
        perms |= PTE_R;
    if (flags & MAP_Writable)
        perms |= PTE_R | PTE_W;
    if (flags & MAP_Executable)
        perms |= PTE_R | PTE_X;

    /*
     * A mapping with none of those set would be a valid PTE with no
     * access bits, which on RISC-V means a pointer to the next level -
     * not a leaf. Refuse rather than corrupt the tables.
     */
    if (!perms)
        return 0;

    /*
     * MAP_CacheInhibit and MAP_WriteThrough have nothing to act on
     * here. Base Sv39 carries no cacheability bits in the PTE (that is
     * the Svpbmt extension, which this hardware does not implement);
     * whether a region is cached follows from its physical memory
     * attributes, and MMIO is uncached by construction.
     */

    return krnMMUMapPages((unsigned long)virt, (unsigned long)phys,
                          length, perms);

    AROS_LIBFUNC_EXIT
}
