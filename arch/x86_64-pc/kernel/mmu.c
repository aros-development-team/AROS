/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <asm/cpu.h>

#include "kernel_base.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "apic.h"

#define D(x)
#define DMMU(x)

void core_SetupMMU(struct KernBootPrivate *__KernBootPrivate)
{
    unsigned int i;
    struct PML4E *PML4;
    struct PDPE  *PDP;
    struct PDE2M *PDE;

    D(bug("[Kernel] core_SetupMMU: Re-creating the MMU pages for first 4GB area\n"));

    if (!__KernBootPrivate->PML4)
    {
	/*
	 * Allocate MMU pages and directories. Four PDE directories (PDE2M structures)
	 * are enough to map whole 4GB address space.
	 */
    	__KernBootPrivate->PML4 = krnAllocBootMemAligned(sizeof(struct PML4E) * 512, PAGE_SIZE);
    	__KernBootPrivate->PDP  = krnAllocBootMemAligned(sizeof(struct PDPE)  * 512, PAGE_SIZE);
    	__KernBootPrivate->PDE  = krnAllocBootMemAligned(sizeof(struct PDE2M) * 512 * 4, PAGE_SIZE);
    	__KernBootPrivate->PTE  = krnAllocBootMemAligned(sizeof(struct PTE)   * 512 * 32, PAGE_SIZE);

    	D(bug("[Kernel] Allocated PML4 0x%p, PDP 0x%p, PDE 0x%p PTE 0x%p\n", __KernBootPrivate->PML4, __KernBootPrivate->PDP, __KernBootPrivate->PDE, __KernBootPrivate->PTE));
    }

    PML4 = __KernBootPrivate->PML4;
    PDP  = __KernBootPrivate->PDP;
    PDE  = __KernBootPrivate->PDE;

    /* PML4 Entry - we need only the first out of 16 entries */
    PML4[0].p  = 1; /* present */
    PML4[0].rw = 1; /* read/write */
    PML4[0].us = 1; /* accessible for user */
    PML4[0].pwt= 0; /* write-through cache */
    PML4[0].pcd= 0; /* cache enabled */
    PML4[0].a  = 0; /* not yet accessed */
    PML4[0].mbz= 0; /* must be zero */
    PML4[0].base_low = (unsigned long)PDP >> 12;
    PML4[0].avl= 0;
    PML4[0].nx = 0;
    PML4[0].avail = 0;
    PML4[0].base_high = ((unsigned long)PDP >> 32) & 0x000FFFFF;

    /* PDP Entries. There are four of them used in order to define 2048 pages of 2MB each. */
    for (i = 0; i < 4; i++)
    {
        struct PDE2M *pdes = &PDE[512 * i];
        unsigned int j;

        /* Set the PDP entry up and point to the PDE table */
        PDP[i].p  = 1;
        PDP[i].rw = 1;
        PDP[i].us = 1;
        PDP[i].pwt= 0;
        PDP[i].pcd= 0;
        PDP[i].a  = 0;
        PDP[i].mbz= 0;
        PDP[i].base_low = (unsigned long)pdes >> 12;

        PDP[i].nx = 0;
        PDP[i].avail = 0;
        PDP[i].base_high = ((unsigned long)pdes >> 32) & 0x000FFFFF;

        for (j=0; j < 512; j++)
        {
            /* Set PDE entries - use 2MB memory pages, with full supervisor and user access */        
            unsigned long base = (i << 30) + (j << 21);

            pdes[j].p  = 1;
            pdes[j].rw = 1;
            pdes[j].us = 1;
            pdes[j].pwt= 0;  // 1
            pdes[j].pcd= 0;  // 1
            pdes[j].a  = 0;
            pdes[j].d  = 0;
            pdes[j].g  = 0;
            pdes[j].pat= 0;
            pdes[j].ps = 1;
            pdes[j].base_low = base >> 13;

            pdes[j].avail = 0;
            pdes[j].nx = 0;
            pdes[j].base_high = (base >> 32) & 0x000FFFFF;
        }
    }

    __KernBootPrivate->used_page = 0;

    D(bug("[Kernel] core_SetupMMU: Registering New PML4 @ 0x%p\n", __KernBootPrivate->PML4));
    wrcr(cr3, __KernBootPrivate->PML4);

    D(bug("[Kernel] core_SetupMMU: Done\n"));
}

void core_ProtPage(intptr_t addr, char p, char rw, char us)
{
    unsigned long pml4_off = (addr >> 39) & 0x1ff;
    unsigned long pdpe_off = (addr >> 30) & 0x1ff;
    unsigned long pde_off  = (addr >> 21) & 0x1ff;
    unsigned long pte_off  = (addr >> 12) & 0x1ff;

    struct PML4E *pml4 = __KernBootPrivate->PML4;
    struct PDPE  *pdpe = (struct PDPE *)((pml4[pml4_off].base_low << 12) | ((unsigned long)pml4[pml4_off].base_high << 32));
    struct PDE4K *pde  = (struct PDE4K *)((pdpe[pdpe_off].base_low << 12) | ((unsigned long)pdpe[pdpe_off].base_high << 32));
    struct PTE   *Pages4K = __KernBootPrivate->PTE;
    struct PTE   *pte;

    DMMU(bug("[Kernel] Marking page 0x%p as read-only\n", addr));

    if (pde[pde_off].ps)
    {
        /* work on local copy of the affected PDE */
        struct PDE4K tmp_pde = pde[pde_off]; 
        struct PDE2M *pde2 = (struct PDE2M *)pde;
        intptr_t base = (pde2[pde_off].base_low << 13) | ((unsigned long)pde2[pde_off].base_high << 32);
        int i;

        pte = &Pages4K[512 * __KernBootPrivate->used_page++];

        D(bug("[Kernel] The page for address 0x%p was a big one. Splitting it into 4K pages\n", addr));
        D(bug("[Kernel] Base=0x%p, pte=0x%p\n", base, pte));

        for (i = 0; i < 512; i++)
        {
            pte[i].p = 1;
            pte[i].rw        = pde2[pde_off].rw;
            pte[i].us        = pde2[pde_off].us;
            pte[i].pwt       = pde2[pde_off].pwt;
            pte[i].pcd       = pde2[pde_off].pcd;
            pte[i].base_low  = base >> 12;
            pte[i].base_high = (base >> 32) & 0x0FFFFF;

            base += PAGE_SIZE;
        }

        tmp_pde.ps = 0;
        tmp_pde.base_low = (intptr_t)pte >> 12;
        tmp_pde.base_high = ((intptr_t)pte >> 32) & 0x0FFFFF;

        pde[pde_off] = tmp_pde;
    }
            
    pte = (struct PTE *)((pde[pde_off].base_low << 12) | ((unsigned long)pde[pde_off].base_high << 32));

    pte[pte_off].rw = rw ? 1:0;
    pte[pte_off].us = us ? 1:0;
    pte[pte_off].p = p ? 1:0;
    asm volatile ("invlpg (%0)"::"r"(addr));   
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
