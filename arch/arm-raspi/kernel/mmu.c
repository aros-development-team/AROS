/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_intern.h"
#include "mmu.h"

unsigned int pagetable[4096]	__attribute__ ((aligned (16384)));
unsigned int pagetable0[32]	__attribute__ ((aligned (16384)));

void core_SetupMMU(void)
{
    unsigned int page;
    unsigned int pt_addr = (unsigned int) &pagetable;
    unsigned int pt0_addr = (unsigned int) &pagetable0;
    register unsigned int control;

    D(bug("[Kernel] core_SetupMMU: Creating MMU pagetable[0] entries for 4GB address space\n"));

    for(page = 0; page < 4096; page ++)
    {
        unsigned int pageflags = PAGE_TRANSLATIONFAULT;
        if (page > 64)
        {
            pageflags = (page << 20) | PAGE_FL_S_BIT | PAGE_SECTION;
            if ((page < (BCM_PHYSBASE >> 20)) || (page > ((BCM_PHYSBASE + 0x2000000) >> 20)))
                pageflags |= PAGE_C_BIT;
        }
        pagetable[page] = pageflags;
    }

    D(bug("[Kernel] core_SetupMMU: Creating MMU pagetable[1] entries for 64MB address space\n"));
    for(page = 0; page < 64; page++)
    {
            pagetable0[page] = (page << 20) | PAGE_FL_S_BIT | PAGE_C_BIT | PAGE_SECTION;
    }

    /* Invalidate caches */
    asm volatile("mcr     p15, 0, %[r], c8, c7, 0" : : [r] "r" (0x0));   //Invalidate entire unified TLB
    asm volatile("mcr     p15, 0, %[r], c8, c6, 0" : : [r] "r" (0x0));   //Invalidate entire data TLB
    asm volatile("mcr     p15, 0, %[r], c8, c5, 0" : : [r] "r" (0x0));   //Invalidate entire instruction TLB
    asm volatile("mcr     p15, 0, %[r], c7, c5, 6" : : [r] "r" (0x0));   //Invalidate entire branch prediction array
    asm volatile("mcr     p15, 0, %[r], c7, c5, 0" : : [r] "r" (0x0));   //Invalidate icache

    /* setup_ttbr0/1 */
    asm volatile("mcr p15, 0, %[addr], c2, c0, 0" : : [addr] "r" (pt0_addr));
    asm volatile("mcr p15, 0, %[addr], c2, c0, 1" : : [addr] "r" (pt_addr));
    /* setup_ttbrc */
    asm volatile("mcr p15, 0, %[n], c2, c0, 2" : : [n] "r" (7));

    /* Set the domain access control to all-supervisor */
    asm volatile("mcr p15, 0, %[r], c3, c0, 0" : : [r] "r" (~0));

    /* Enable L1 caches (I-cache and D-cache) and MMU.*/
    asm volatile("mrc p15, 0, %[control], c1, c0, 0" : [control] "=r" (control));
    control |= ( ENABLE_I_CACHE | ENABLE_D_CACHE | ENABLE_MMU );
    asm volatile ("mcr p15, 0, %[r], c7, c10, 4" : : [r] "r" (0)); /* dsb */
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r" (control) : "cc" );
    asm volatile ("mcr p15, 0, %[r], c7, c5, 4" : : [r] "r" (0)); /* isb */

    D(bug("[Kernel] core_SetupMMU: Done\n"));
}

void core_ProtPage(intptr_t addr, char p, char rw, char us)
{
    D(bug("[Kernel] Marking page 0x%p as read-only\n", addr));
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
