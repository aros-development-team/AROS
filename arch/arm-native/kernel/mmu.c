/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
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

    /* Invalidate caches */
    asm volatile("mcr   p15, 0, %[r], c8, c7, 0" : : [r] "r" (0x0));   //Invalidate entire unified TLB
    asm volatile("mcr   p15, 0, %[r], c8, c6, 0" : : [r] "r" (0x0));   //Invalidate entire data TLB
    asm volatile("mcr   p15, 0, %[r], c8, c5, 0" : : [r] "r" (0x0));   //Invalidate entire instruction TLB
    asm volatile("mcr   p15, 0, %[r], c7, c5, 6" : : [r] "r" (0x0));   //Invalidate entire branch prediction array
    asm volatile("mcr   p15, 0, %[r], c7, c5, 0" : : [r] "r" (0x0));   //Invalidate icache

    /* setup_ttbr0/1 */
    asm volatile("mcr   p15, 0, %[addr], c2, c0, 1" : : [addr] "r" (pde));
    /* setup_ttbrc */
    asm volatile("mcr   p15, 0, %[n], c2, c0, 2" : : [n] "r" (7));
}

void core_SetupMMU(struct TagItem *msg)
{
    register unsigned int control;

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
