/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: mmu.c
    Lang: english
 */

#include <stdint.h>
#include <hardware/bcm2708_boot.h>
#include "mmu.h"
#include "boot.h"

#define DMMU(x) x

void mmu_init()
{
    static pde_t *pde = (pde_t *)BOOTMEMADDR(bm_pde);
    int i;

    for (i = 0; i < 4096; i++)
    {
#if 0
        pde[i].raw = i << 20;
        pde[i].section.type = PDE_TYPE_SECTION;
        pde[i].section.b = 1;
        pde[i].section.c = 1;
        pde[i].section.ap = 3;
        pde[i].section.apx = 0;
        pde[i].section.tex = 1;
#else
        pde[i].raw = 0;
#endif
    }
}

void mmu_load()
{
    uint32_t tmp;

    static pde_t *pde = (pde_t *)BOOTMEMADDR(bm_pde);

    arm_flush_cache((uint32_t)pde, 16384);

    /* Write page_dir address to ttbr0 */
    asm volatile ("mcr p15, 0, %0, c2, c0, 0"::"r"(pde));
    /* Write ttbr control N = 0 (use only ttbr0) */
    asm volatile ("mcr p15, 0, %0, c2, c0, 2"::"r"(0));

    /* Set domains - Dom0 is usable, rest is disabled */
    asm volatile ("mrc p15, 0, %0, c3, c0, 0":"=r"(tmp));
    DMMU(kprintf("[BOOT] Domain access control register: %08x\n", tmp));
    asm volatile ("mcr p15, 0, %0, c3, c0, 0"::"r"(0x00000001));

    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(tmp));
    DMMU(kprintf("[BOOT] control register %08x\n", tmp));
    tmp |= 1;           /* Enable MMU */
    tmp |= 1 << 23;     /* v6 page tables, subpages disabled */
    asm volatile ("mcr  p15, 0, %[r], c7, c10, 4" : : [r] "r" (0)); /* dsb */
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(tmp));
    asm volatile ("mcr  p15, 0, %[r], c7, c5, 4" : : [r] "r" (0)); /* isb */
    DMMU(kprintf("[BOOT] mmu up\n"));
}

void mmu_unmap_section(uint32_t virt, uint32_t length)
{
    static pde_t *pde = (pde_t *)BOOTMEMADDR(bm_pde);

    uint32_t start = virt & ~(1024*1024-1);
    uint32_t end = (start + length) & ~(1024*1024-1);

    start >>= 20;
    end >>= 20;

    while (start < end)
    {
        pde[start].raw = 0;
        start++;
    }
}

void mmu_map_section(uint32_t phys, uint32_t virt, uint32_t length, int b, int c, int ap, int tex)
{
    static pde_t *pde = (pde_t *)BOOTMEMADDR(bm_pde);

    uint32_t start = virt & ~(1024*1024-1);
    uint32_t end = (start + length) & ~(1024*1024-1);

    DMMU(kprintf("[BOOT] MMU map %p:%p->%p:%p, b=%d, c=%d, ap=%x, tex=%x\n",
            phys, phys+length-1, virt, virt+length-1, b, c, ap, tex));

    int count = (end - start) >> 20;
    int i = start >> 20;
    phys >>= 20;

    while(count--)
    {
        pde_t s;

        s.raw = PDE_TYPE_SECTION | (c << 3) | (b << 2);
        s.raw |= (ap & 3) << 10;
        s.raw |= (tex) << 12;
        s.raw |= ((ap >> 2) & 1) << 15;
        s.raw |= phys << 20;
#if 0
        s.section.type = PDE_TYPE_SECTION;
        s.section.b = b;
        s.section.c = c;
        s.section.ap = ap & 3;
        s.section.apx = (ap >> 2) & 1;
        s.section.tex = tex;
        s.section.base_address = phys;
#endif

        pde[i] = s;
        DMMU(kprintf("[BOOT] pde[%04d] = %08x\n", i, s.raw));

        phys++;
        i++;
    }
}
