/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <hardware/bcm2708_boot.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_intern.h"
#include "mmu.h"

void core_MMUUpdatePageTables(void)
{
    static pde_t *pde = (pde_t *)BOOTMEMADDR(bm_pde);
    unsigned int ttbr;

    /* Invalidate caches and TLBs */
#if defined(__AROSEXEC_SMP__)
    /* Broadcast variants; need ACTLR.SMP set on each CPU to be honoured. */
    asm volatile("mcr   p15, 0, %[r], c8, c3, 0" : : [r] "r" (0x0));   // TLBIALLIS
    asm volatile("mcr   p15, 0, %[r], c7, c1, 6" : : [r] "r" (0x0));   // BPIALLIS
    asm volatile("mcr   p15, 0, %[r], c7, c1, 0" : : [r] "r" (0x0));   // ICIALLUIS
#else
    asm volatile("mcr   p15, 0, %[r], c8, c7, 0" : : [r] "r" (0x0));   // TLBIALL
    asm volatile("mcr   p15, 0, %[r], c8, c6, 0" : : [r] "r" (0x0));   // DTLBIALL
    asm volatile("mcr   p15, 0, %[r], c8, c5, 0" : : [r] "r" (0x0));   // ITLBIALL
    asm volatile("mcr   p15, 0, %[r], c7, c5, 6" : : [r] "r" (0x0));   // BPIALL
    asm volatile("mcr   p15, 0, %[r], c7, c5, 0" : : [r] "r" (0x0));   // ICIALLU
#endif

    /* setup_ttbr1 */
    ttbr = (unsigned int)pde;
#if defined(__AROSEXEC_SMP__)
    /* Inner-shareable WB-WA table walks: S | RGN[0] | IRGN[0]. The
     * descriptors need their S bit too - the bootstrap sets that. */
    ttbr |= 0x4A;
#endif
    asm volatile("mcr   p15, 0, %[addr], c2, c0, 1" : : [addr] "r" (ttbr));
#if defined(__AROSEXEC_SMP__)
    /* TTBR0 (low 32MB - SysBase, scheduler lists) needs the same walk
     * attributes as the secondaries use, or the mismatch lets the boot
     * CPU read stale descriptors. The bootstrap leaves them clear. */
    asm volatile("mcr   p15, 0, %[addr], c2, c0, 0" : : [addr] "r" (ttbr));
    asm volatile("mcr   p15, 0, %[r], c7, c10, 4" : : [r] "r" (0)); /* dsb */
    asm volatile("mcr   p15, 0, %[r], c7, c5, 4" : : [r] "r" (0));  /* isb */
#endif
    /* setup_ttbrc - TTBR0 covers low 32MB, TTBR1 covers the rest */
    asm volatile("mcr   p15, 0, %[n], c2, c0, 2" : : [n] "r" (7));
}

void core_SetupMMU(struct TagItem *msg)
{
    register unsigned int control;

#if defined(__AROSEXEC_SMP__)
    /* SMP/snoop enable. Only Cortex-A7 has SMPEN at ACTLR bit 6; on A53
     * that bit is something else (SMPEN lives in EL3-only CPUECTLR), so
     * gate on MIDR. */
    register unsigned int midr, actlr;
    asm volatile("mrc p15, 0, %0, c0, c0, 0" : "=r" (midr));
    if ((midr & 0xfff0) == 0xc070)
    {
        asm volatile("mrc p15, 0, %0, c1, c0, 1" : "=r" (actlr));
        actlr |= (1 << 6);
        asm volatile("mcr p15, 0, %0, c1, c0, 1" : : "r" (actlr));
        asm volatile("dsb" ::: "memory");
        asm volatile("isb" ::: "memory");
    }
#endif

    core_MMUUpdatePageTables();

    /* Set the domain access control to all-supervisor */
    asm volatile("mcr   p15, 0, %[r], c3, c0, 0" : : [r] "r" (~0));

    /* Enable L1 caches (I-cache and D-cache) and MMU.*/
    asm volatile("mrc   p15, 0, %[control], c1, c0, 0" : [control] "=r" (control));
    control |= ( ENABLE_I_CACHE | ENABLE_D_CACHE | ENABLE_MMU );
    asm volatile ("mcr  p15, 0, %[r], c7, c10, 4" : : [r] "r" (0)); /* dsb */
    asm volatile ("mcr  p15, 0, %0, c1, c0, 0" : : "r" (control) : "cc" );
    asm volatile ("mcr  p15, 0, %[r], c7, c5, 4" : : [r] "r" (0)); /* isb */

    D(bug("[Kernel] core_SetupMMU: Done\n"));
}

void core_ProtPage(intptr_t addr, char p, char rw, char us)
{
    D(bug("[Kernel] Marking page 0x%p as read-only\n", addr));

    core_MMUUpdatePageTables();
}

void core_ProtKernelArea(intptr_t addr, intptr_t length, char p, char rw, char us)
{
    D(bug("[Kernel] Protecting area 0x%p - 0x%p\n", addr, addr + length - 1));
    while (length > 0)
    {
        core_ProtPage(addr, p, rw, us);
        addr += 4096;
        length -= 4096;
    }
}
