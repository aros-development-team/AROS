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

#define PTPOOL_PAGES 128

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

#define PTE_PERM_MASK   (PTE_R | PTE_W | PTE_X | PTE_U | PTE_G | PTE_A | PTE_D)

/*
 * Replace a leaf entry with a table of smaller leaves covering exactly
 * the same memory with the same permissions. This lets a fine-grained
 * mapping be laid over a region already covered by a large page,
 * whatever order the mappings are made in.
 */
static pte_t *krnPTSplit(pte_t *entry, unsigned long pagesize)
{
    unsigned long perms = *entry & PTE_PERM_MASK;
    unsigned long base = PTE_TO_PA(*entry);
    pte_t *tbl = krnPTAlloc();
    unsigned int i;

    for (i = 0; i < PT_ENTRIES; i++)
        tbl[i] = PA_TO_PTE(base + (unsigned long)i * pagesize) | perms | PTE_V;

    *entry = PA_TO_PTE(tbl) | PTE_V;
    return tbl;
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
    else if (l2[idx2] & PTE_LEAF_MASK)
        krnPTSplit(&l2[idx2], MEGAPAGE_SIZE);   /* gigapage -> megapages */
    l1 = (pte_t *)PTE_TO_PA(l2[idx2]);

    if (!(l1[idx1] & PTE_V))
    {
        l0 = krnPTAlloc();
        l1[idx1] = PA_TO_PTE(l0) | PTE_V;
    }
    else if (l1[idx1] & PTE_LEAF_MASK)
        krnPTSplit(&l1[idx1], PAGE_SIZE);       /* megapage -> 4K pages */

    return (pte_t *)PTE_TO_PA(l1[idx1]);
}

/* The level-1 table covering a VA, splitting a gigapage if one is in
   the way. Used when a mapping is big enough to want megapages. */
static pte_t *krnPTWalkL1(unsigned long va)
{
    pte_t *l2 = pt_root;
    unsigned int idx2 = SV39_VPN(va, 2);

    if (!(l2[idx2] & PTE_V))
    {
        pte_t *l1 = krnPTAlloc();

        l2[idx2] = PA_TO_PTE(l1) | PTE_V;
    }
    else if (l2[idx2] & PTE_LEAF_MASK)
        krnPTSplit(&l2[idx2], MEGAPAGE_SIZE);

    return (pte_t *)PTE_TO_PA(l2[idx2]);
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

/*
 * Map [pa, pa+size) at an arbitrary virtual address, 4KiB at a time.
 * This is what kernel.resource's KrnMapGlobal() is built on, so it is
 * how a driver reaches device registers - the boot-time mapping only
 * covers RAM.
 *
 * Returns 0 if the range could not be mapped. Runs after the MMU is
 * live, so the TLB is flushed for the range on the way out.
 */
int krnMMUMapPages(unsigned long va, unsigned long pa, unsigned long size,
                   unsigned long perms)
{
    unsigned long off;

    if (!pt_root || !size)
        return 0;

    /* Whole pages, from the start of the page each address falls in */
    size += pa & (PAGE_SIZE - 1);
    va   &= ~(PAGE_SIZE - 1);
    pa   &= ~(PAGE_SIZE - 1);
    size  = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    /*
     * Use megapages wherever both ends line up and there is enough
     * left. A PCIe ECAM window is 256MiB; mapping that a page at a
     * time would need 128 page tables of its own and exhaust the pool.
     */
    for (off = 0; off < size; )
    {
        unsigned long v = va + off, p = pa + off;

        if (!((v | p) & (MEGAPAGE_SIZE - 1)) &&
            (size - off) >= MEGAPAGE_SIZE)
        {
            pte_t *l1 = krnPTWalkL1(v);

            if (!l1)
                return 0;
            l1[SV39_VPN(v, 1)] = PA_TO_PTE(p) | perms | PTE_A | PTE_D |
                                 PTE_G | PTE_V;
            off += MEGAPAGE_SIZE;
        }
        else
        {
            pte_t *l0 = krnPTWalk(v);

            if (!l0)
                return 0;
            l0[SV39_VPN(v, 0)] = PA_TO_PTE(p) | perms | PTE_A | PTE_D |
                                 PTE_G | PTE_V;
            off += PAGE_SIZE;
        }
    }

    asm volatile("sfence.vma zero, zero" ::: "memory");
    return 1;
}

/* Drop a mapping made by krnMMUMapPages() */
int krnMMUUnmapPages(unsigned long va, unsigned long size)
{
    unsigned long off;

    if (!pt_root || !size)
        return 0;

    size += va & (PAGE_SIZE - 1);
    va   &= ~(PAGE_SIZE - 1);
    size  = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    for (off = 0; off < size; off += PAGE_SIZE)
    {
        pte_t *l0 = krnPTWalk(va + off);

        if (l0)
            l0[SV39_VPN(va + off, 0)] = 0;
    }

    asm volatile("sfence.vma zero, zero" ::: "memory");
    return 1;
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
        else if (l2[idx2] & PTE_LEAF_MASK)
            krnPTSplit(&l2[idx2], MEGAPAGE_SIZE);
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

/*
 * Map one physical region identity, gigapage where it lines up and
 * megapage for the remainder. Used for every bank the firmware
 * reports, and for anything handed to us outside them.
 */
void krnMMUMapRegion(IPTR base, IPTR size, unsigned long perms)
{
    IPTR pa  = base & ~(MEGAPAGE_SIZE - 1);
    IPTR end = (base + size + MEGAPAGE_SIZE - 1) & ~(MEGAPAGE_SIZE - 1);

    while (pa < end)
    {
        pte_t root = pt_root[SV39_VPN(pa, 2)];

        /*
         * Already covered by a gigapage. Descending into a leaf entry
         * would treat the mapped memory itself as a page table, so
         * step over it.
         */
        if ((root & PTE_V) && (root & PTE_LEAF_MASK))
        {
            pa = (pa + GIGAPAGE_SIZE) & ~(GIGAPAGE_SIZE - 1);
            continue;
        }

        if (!(pa & (GIGAPAGE_SIZE - 1)) && (end - pa) >= GIGAPAGE_SIZE &&
            !(root & PTE_V))
        {
            krnMapGiga(pa, GIGAPAGE_SIZE, perms);
            pa += GIGAPAGE_SIZE;
        }
        else
        {
            krnMapMega(pa, MEGAPAGE_SIZE, perms);
            pa += MEGAPAGE_SIZE;
        }
    }
}

/* Set by the EFI stub (see efi_stub.c) */
extern unsigned long __efi_nfwregions;
extern unsigned long __efi_fwregions[][2];
extern unsigned long __efi_rsdp;

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
     * stays tiny no matter how much RAM is fitted: megapages alone
     * would need one L1 table per GiB and exhaust the pool.
     *
     * Allocatable RAM is mapped RWX: AROS has a single address space
     * and runs code out of ordinary allocations (LoadSeg()ed segments,
     * library trampolines), so it can not be marked no-execute.
     *
     * The gigabyte containing the kernel is the exception: it is mapped
     * with megapages, and the kernel image itself with 4KiB pages, so
     * the kickstart keeps per-section W^X.
     */
    for (pa = info->mem_base & ~(GIGAPAGE_SIZE - 1); pa < mem_end;
         pa += GIGAPAGE_SIZE)
    {
        if (pa == kgiga_lo)
            continue;
        krnMapGiga(pa, GIGAPAGE_SIZE, PTE_R | PTE_W | PTE_X);
    }

    /*
     * Any further banks the firmware reported. They need not be
     * adjacent to the main one, and the boot package can land in any
     * of them.
     */
    {
        unsigned int r;

        for (r = 0; r < info->nregions; r++)
        {
            if (info->regions[r].base == info->mem_base)
                continue;
            krnMMUMapRegion(info->regions[r].base, info->regions[r].size,
                            PTE_R | PTE_W | PTE_X);
        }
    }

    /* The kernel's gigabyte, in megapages except the kernel image */
    for (pa = kgiga_lo; pa < kgiga_lo + GIGAPAGE_SIZE; pa += MEGAPAGE_SIZE)
    {
        if (pa >= kimg_lo && pa < kimg_hi)
            continue;
        krnMapMega(pa, MEGAPAGE_SIZE, PTE_R | PTE_W | PTE_X);
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
             PTE_R | PTE_W | PTE_X);
    /* The gap between the megapage base and the kernel (OpenSBI's
       territory is below; anything in this range is fair RW RAM) */
    if (kimg_lo < (unsigned long)__text_start)
        krnMap4K(kimg_lo,
                 (unsigned long)__text_start - kimg_lo,
                 PTE_R | PTE_W | PTE_X);

    /*
     * Memory the firmware kept for itself: EFI runtime services and the
     * ACPI tables. It is not allocatable, but acpica.library reads the
     * tables straight out of it, so it has to be readable. Mapped RW
     * and no-execute - nothing here is ever run.
     *
     * Only worth doing when there are ACPI tables to read. The regions
     * are scattered over the whole address space, so mapping them costs
     * a lot of page tables - and a device-tree machine never touches
     * them.
     */
    if (__efi_rsdp)
    {
        unsigned int r;

        krnSBIPutStr("mmu:       mapping ");
        krnSBIPutDec(__efi_nfwregions);
        krnSBIPutStr(" firmware region(s)\n");

        for (r = 0; r < __efi_nfwregions; r++)
            krnMMUMapRegion(__efi_fwregions[r][0], __efi_fwregions[r][1],
                            PTE_R | PTE_W);
    }

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
