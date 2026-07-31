/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Sv39 MMU bring-up for the opensbi-riscv64 target.

    Builds an identity map (VA == PA) so all existing pointers stay
    valid when translation is enabled:

      - RAM is mapped with 2MiB megapages (RW, global), except the
        2MiB regions covering the kernel image, which are mapped with
        4KiB pages carrying per-section W^X permissions (.text RX,
        .rodata R, data/bss RW).
      - Everything else (device space) is left unmapped; stray
        accesses fault into the trap handler. Devices get mapped as
        drivers appear (SBI calls do not need S-mode mappings).

    Page tables come from a small static pool inside the kernel image.
*/

#include <inttypes.h>

#include <asm/cpu.h>
#include <asm/riscv64/mmu.h>

#include "kernel_intern.h"

/* Linker script section boundaries */
extern char __text_start[], __text_end[];
extern char __rodata_start[], __rodata_end[];
extern char __kernel_end[];

typedef unsigned long pte_t;

#define PTPOOL_PAGES 16

static pte_t pt_pool[PTPOOL_PAGES][PT_ENTRIES]
        __attribute__((aligned(PAGE_SIZE)));
static unsigned int pt_pool_used;

static pte_t *pt_root;

static pte_t *krnPTAlloc(void)
{
    unsigned int i;
    pte_t *pt;

    if (pt_pool_used >= PTPOOL_PAGES)
    {
        krnSBIPutStr("[mmu] PANIC: page table pool exhausted!\n");
        for (;;)
            asm volatile("wfi");
    }

    pt = pt_pool[pt_pool_used++];
    for (i = 0; i < PT_ENTRIES; i++)
        pt[i] = 0;
    return pt;
}

/* Return the level-0 table for va, creating intermediate levels */
static pte_t *krnPTWalk(unsigned long va)
{
    pte_t *l2 = pt_root, *l1, *l0;
    unsigned int idx2 = SV39_VPN(va, 2), idx1 = SV39_VPN(va, 1);

    if (!(l2[idx2] & PTE_V))
    {
        l1 = krnPTAlloc();
        l2[idx2] = PA_TO_PTE(l1) | PTE_V;
    }
    l1 = (pte_t *)PTE_TO_PA(l2[idx2]);

    if (!(l1[idx1] & PTE_V))
    {
        l0 = krnPTAlloc();
        l1[idx1] = PA_TO_PTE(l0) | PTE_V;
    }
    else if (l1[idx1] & PTE_LEAF_MASK)
    {
        krnSBIPutStr("[mmu] PANIC: 4K mapping over an existing megapage!\n");
        for (;;)
            asm volatile("wfi");
    }
    return (pte_t *)PTE_TO_PA(l1[idx1]);
}

/* Map [pa, pa+size) at VA == PA with 4KiB pages */
static void krnMap4K(unsigned long pa, unsigned long size, unsigned long perms)
{
    unsigned long va;

    for (va = pa; va < pa + size; va += PAGE_SIZE)
    {
        pte_t *l0 = krnPTWalk(va);
        l0[SV39_VPN(va, 0)] = PA_TO_PTE(va) | perms | PTE_A | PTE_D |
                              PTE_G | PTE_V;
    }
}

/* Map a 2MiB-aligned region at VA == PA with megapages */
static void krnMapMega(unsigned long pa, unsigned long size, unsigned long perms)
{
    unsigned long va;

    for (va = pa; va < pa + size; va += MEGAPAGE_SIZE)
    {
        pte_t *l2 = pt_root, *l1;
        unsigned int idx2 = SV39_VPN(va, 2);

        if (!(l2[idx2] & PTE_V))
        {
            l1 = krnPTAlloc();
            l2[idx2] = PA_TO_PTE(l1) | PTE_V;
        }
        l1 = (pte_t *)PTE_TO_PA(l2[idx2]);
        l1[SV39_VPN(va, 1)] = PA_TO_PTE(va) | perms | PTE_A | PTE_D |
                              PTE_G | PTE_V;
    }
}

/* Map a 1GiB-aligned region at VA == PA with gigapages (root leaves) */
static void krnMapGiga(unsigned long pa, unsigned long size, unsigned long perms)
{
    unsigned long va;

    for (va = pa; va < pa + size; va += GIGAPAGE_SIZE)
        pt_root[SV39_VPN(va, 2)] = PA_TO_PTE(va) | perms | PTE_A | PTE_D |
                                   PTE_G | PTE_V;
}

/*
 * Re-map an already-mapped 4KiB-page range with new permissions. Used
 * for the boot modules, which are loaded into what the initial map
 * treats as plain RW data.
 *
 * TODO: modules are mapped RWX as a block; per-section W^X would need
 * the loader to place each section on its own page.
 */
void krnMMUSetPerms(IPTR lo, IPTR hi, unsigned long perms)
{
    unsigned long va;

    for (va = lo & ~(PAGE_SIZE - 1); va < hi; va += PAGE_SIZE)
    {
        pte_t *l0 = krnPTWalk(va);
        l0[SV39_VPN(va, 0)] = PA_TO_PTE(va) | perms | PTE_A | PTE_D |
                              PTE_G | PTE_V;
    }

    asm volatile("sfence.vma zero, zero" ::: "memory");
}

void krnInitMMU(struct krnFDTInfo *info)
{
    unsigned long kimg_lo = (unsigned long)__text_start & ~(MEGAPAGE_SIZE - 1);
    unsigned long kimg_hi = ((unsigned long)__kernel_end + MEGAPAGE_SIZE - 1)
                            & ~(MEGAPAGE_SIZE - 1);
    unsigned long mem_end = info->mem_base + info->mem_size;
    unsigned long pa;

    unsigned long kgiga_lo = (unsigned long)__text_start & ~(GIGAPAGE_SIZE - 1);

    pt_root = krnPTAlloc();

    /*
     * Bulk RAM is mapped with 1GiB gigapages straight out of the root
     * table - no second-level tables at all, so the page table pool
     * stays tiny no matter how much RAM is fitted (the Milk-V Titan
     * takes up to 64GiB; megapages alone would need one L1 table per
     * GiB and exhaust the pool).
     *
     * The gigabyte containing the kernel is the exception: it is mapped
     * with megapages, and the kernel's own megabytes with 4KiB pages,
     * so per-section W^X permissions can be applied.
     */
    for (pa = info->mem_base & ~(GIGAPAGE_SIZE - 1); pa < mem_end;
         pa += GIGAPAGE_SIZE)
    {
        if (pa == kgiga_lo)
            continue;
        krnMapGiga(pa, GIGAPAGE_SIZE, PTE_R | PTE_W);
    }

    /* The kernel's gigabyte, in megapages except the kernel image */
    for (pa = kgiga_lo; pa < kgiga_lo + GIGAPAGE_SIZE; pa += MEGAPAGE_SIZE)
    {
        if (pa >= kimg_lo && pa < kimg_hi)
            continue;
        krnMapMega(pa, MEGAPAGE_SIZE, PTE_R | PTE_W);
    }

    /* The kernel image, with per-section W^X permissions */
    krnMap4K((unsigned long)__text_start,
             (unsigned long)__text_end - (unsigned long)__text_start,
             PTE_R | PTE_X);
    krnMap4K((unsigned long)__rodata_start,
             (unsigned long)__rodata_end - (unsigned long)__rodata_start,
             PTE_R);
    krnMap4K((unsigned long)__rodata_end,
             kimg_hi - (unsigned long)__rodata_end,
             PTE_R | PTE_W);
    /* The gap between the megapage base and the kernel (OpenSBI's
       territory is below; anything in this range is fair RW RAM) */
    if (kimg_lo < (unsigned long)__text_start)
        krnMap4K(kimg_lo,
                 (unsigned long)__text_start - kimg_lo,
                 PTE_R | PTE_W);

    /* The DTB may sit outside the mapped RAM ranges rounding; it is
       inside RAM on qemu/OpenSBI setups, so nothing extra to do. */

    asm volatile("sfence.vma zero, zero" ::: "memory");
    csr_write(satp, SATP_MODE_SV39 | SATP_PPN(pt_root));
    asm volatile("sfence.vma zero, zero" ::: "memory");

    krnSBIPutStr("mmu:       Sv39 enabled, root table ");
    krnSBIPutHex((uint64_t)(uintptr_t)pt_root);
    krnSBIPutStr(" (");
    krnSBIPutDec(pt_pool_used);
    krnSBIPutStr(" tables)\n");
}
