/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    AArch64 MMU management for the kernel.
    Uses 4KB granule with 2MB block descriptors (level 2).
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

/*
 * AArch64 MMU descriptor bits
 */
#define MMU_DESC_VALID      (1UL << 0)
#define MMU_DESC_TABLE      (1UL << 1)  /* For level 0/1: points to next level table */
#define MMU_DESC_BLOCK      (0UL << 1)  /* For level 1/2: block descriptor */
#define MMU_DESC_PAGE       (1UL << 1)  /* For level 3: page descriptor */

/* Lower attributes */
#define MMU_ATTR_IDX(n)     ((uint64_t)(n) << 2)
#define MMU_ATTR_NS         (1UL << 5)
#define MMU_ATTR_AP_RW      (0UL << 6)
#define MMU_ATTR_AP_RO      (2UL << 6)
#define MMU_ATTR_AP_EL0     (1UL << 6)
#define MMU_ATTR_SH_NONE    (0UL << 8)
#define MMU_ATTR_SH_OUTER   (2UL << 8)
#define MMU_ATTR_SH_INNER   (3UL << 8)
#define MMU_ATTR_AF         (1UL << 10)
#define MMU_ATTR_nG         (1UL << 11)

/* Upper attributes */
#define MMU_ATTR_PXN        (1UL << 53)
#define MMU_ATTR_UXN        (1UL << 54)

/* MAIR attribute indices (must match boot MMU setup) */
#define MAIR_IDX_DEVICE     0   /* Device-nGnRnE */
#define MAIR_IDX_NORMAL_WB  1   /* Normal, Write-Back Cacheable */
#define MAIR_IDX_NORMAL_NC  2   /* Normal, Non-Cacheable */

/* MAIR register value */
#define MAIR_VALUE  ( \
    (0x00UL << (MAIR_IDX_DEVICE * 8))    | \
    (0xFFUL << (MAIR_IDX_NORMAL_WB * 8)) | \
    (0x44UL << (MAIR_IDX_NORMAL_NC * 8))   \
)

/* TCR_EL1 configuration: 4KB granule, 35-bit VA (T0SZ=29), 36-bit PA.
   Must match the boot MMU setup. */
#define TCR_VALUE ( \
    (29UL << 0)      | /* T0SZ = 29 (32GB VA space) */ \
    (0UL  << 7)      | /* EPD0 = 0 (TTBR0 enabled) */ \
    (1UL  << 8)      | /* IRGN0 = Write-Back */ \
    (1UL  << 10)     | /* ORGN0 = Write-Back */ \
    (3UL  << 12)     | /* SH0 = Inner Shareable */ \
    (0UL  << 14)     | /* TG0 = 4KB granule */ \
    (1UL  << 23)     | /* EPD1 = 1 (TTBR1 disabled) */ \
    (1UL  << 31)       /* RES1 */ \
)

/* Translation levels for a 4KB granule. The VA size is read from TCR_EL1
   at runtime, since boot picks it. */
#define MMU_L1_SHIFT        30
#define MMU_L2_SHIFT        21
#define MMU_L3_SHIFT        12
#define MMU_L1_SIZE         (1UL << MMU_L1_SHIFT)
#define MMU_L2_SIZE         (1UL << MMU_L2_SHIFT)
#define MMU_L3_SIZE         (1UL << MMU_L3_SHIFT)
#define MMU_TBL_ENTRIES     512

/* Descriptor address field, and the attribute bits carried across a split */
#define MMU_DESC_OA         0x0000fffffffff000UL
#define MMU_DESC_ATTRS      (~(MMU_DESC_OA | 3UL))


void core_MMUUpdatePageTables(void)
{
    /* Invalidate all TLBs */
    asm volatile("tlbi vmalle1" ::: "memory");
    asm volatile("dsb sy" ::: "memory");
    asm volatile("isb" ::: "memory");
}

void core_SetupMMU(struct TagItem *msg)
{
    uint64_t sctlr;

    core_MMUUpdatePageTables();

    /* Enable L1 caches (I-cache and D-cache) and MMU */
    asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr |= (ENABLE_I_CACHE | ENABLE_D_CACHE | ENABLE_MMU);
    asm volatile("dsb sy" ::: "memory");
    asm volatile("msr sctlr_el1, %0" : : "r"(sctlr));
    asm volatile("isb" ::: "memory");

    D(bug("[Kernel] core_SetupMMU: Done\n"));
}

/* How much VA the walker looks at, from TCR_EL1's T0SZ */
static unsigned int mmu_va_bits(void)
{
    uint64_t tcr;

    asm volatile("mrs %0, tcr_el1" : "=r"(tcr));

    return 64 - (unsigned int)(tcr & 0x3f);
}

/*
 * The live level 1 table. The boot tables are identity mapped, so the
 * address out of TTBR0_EL1 can be walked as it stands. A VA wider than
 * 39 bits starts at level 0 instead - refuse rather than walk it wrong.
 */
static uint64_t *mmu_root(void)
{
    uint64_t ttbr0;

    if (mmu_va_bits() > 39)
        return NULL;

    asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0));

    return (uint64_t *)(uintptr_t)(ttbr0 & MMU_DESC_OA);
}

/* Entries in the level 1 table - one per gigabyte of reachable VA */
static uint64_t mmu_l1_entries(void)
{
    return 1UL << (mmu_va_bits() - MMU_L1_SHIFT);
}

static uint64_t *mmu_alloc_table(void)
{
    IPTR raw;

    /* AllocMem does not align, so take a page extra and align inside it */
    raw = (IPTR)AllocMem(MMU_L3_SIZE * 2, MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw)
        return NULL;

    return (uint64_t *)((raw + MMU_L3_SIZE - 1) & ~(IPTR)(MMU_L3_SIZE - 1));
}

/*
 * The next-level table for a descriptor, created if absent. A block in
 * the way is split into equivalent smaller entries, so a mapping made
 * inside it leaves the rest of the block as it was.
 */
static uint64_t *mmu_next(uint64_t *entry, uint64_t childsize, uint64_t childtype)
{
    uint64_t desc = *entry;
    uint64_t *tbl;
    unsigned int i;

    if ((desc & MMU_DESC_VALID) && (desc & MMU_DESC_TABLE))
        return (uint64_t *)(uintptr_t)(desc & MMU_DESC_OA);

    tbl = mmu_alloc_table();
    if (!tbl)
        return NULL;

    if (desc & MMU_DESC_VALID)
    {
        for (i = 0; i < MMU_TBL_ENTRIES; i++)
            tbl[i] = ((desc & MMU_DESC_OA) + (uint64_t)i * childsize) |
                     (desc & MMU_DESC_ATTRS) | childtype | MMU_DESC_VALID;
    }

    *entry = (uint64_t)(uintptr_t)tbl | MMU_DESC_TABLE | MMU_DESC_VALID;

    return tbl;
}

/*
 * Publish edited descriptors and drop stale TLB entries. The walk is
 * cacheable inner shareable, so the stores only need ordering.
 */
static void mmu_commit(void)
{
    asm volatile("dsb ishst" ::: "memory");
    asm volatile("tlbi vmalle1is" ::: "memory");
    asm volatile("dsb ish" ::: "memory");
    asm volatile("isb" ::: "memory");
}

/*
 * Map [pa, pa+size) at va, 2MB blocks where both ends line up and 4KB
 * pages otherwise. Returns 0 on failure.
 */
static int mmu_map_range(uint64_t va, uint64_t pa, uint64_t size, uint64_t attr)
{
    uint64_t off;

    /* Round out to whole pages */
    size += pa & (MMU_L3_SIZE - 1);
    va   &= ~(MMU_L3_SIZE - 1);
    pa   &= ~(MMU_L3_SIZE - 1);
    size  = (size + MMU_L3_SIZE - 1) & ~(MMU_L3_SIZE - 1);

    /* Outside what TCR_EL1's T0SZ lets the walker reach */
    if ((va + size) > (1UL << mmu_va_bits()))
        return 0;

    for (off = 0; off < size; )
    {
        uint64_t v = va + off, p = pa + off;
        uint64_t *l1 = mmu_root();
        uint64_t *l2, *l3, *slot;

        if (!l1)
            return 0;

        l2 = mmu_next(&l1[(v >> MMU_L1_SHIFT) & (mmu_l1_entries() - 1)],
                      MMU_L2_SIZE, MMU_DESC_BLOCK);
        if (!l2)
            return 0;

        slot = &l2[(v >> MMU_L2_SHIFT) & (MMU_TBL_ENTRIES - 1)];

        /* A 2MB block, unless finer mappings already live inside it */
        if (!((v | p) & (MMU_L2_SIZE - 1)) && (size - off) >= MMU_L2_SIZE &&
            !((*slot & MMU_DESC_VALID) && (*slot & MMU_DESC_TABLE)))
        {
            *slot = p | attr | MMU_DESC_BLOCK | MMU_DESC_VALID;
            off += MMU_L2_SIZE;
            continue;
        }

        l3 = mmu_next(slot, MMU_L3_SIZE, MMU_DESC_PAGE);
        if (!l3)
            return 0;

        l3[(v >> MMU_L3_SHIFT) & (MMU_TBL_ENTRIES - 1)] =
            p | attr | MMU_DESC_PAGE | MMU_DESC_VALID;
        off += MMU_L3_SIZE;
    }

    mmu_commit();

    return 1;
}

/*
 * Drop the mappings for [va, va+size). A partly covered block is split
 * first, so neighbours survive. Emptied table pages are not freed.
 */
static int mmu_unmap_range(uint64_t va, uint64_t size)
{
    uint64_t off;

    size += va & (MMU_L3_SIZE - 1);
    va   &= ~(MMU_L3_SIZE - 1);
    size  = (size + MMU_L3_SIZE - 1) & ~(MMU_L3_SIZE - 1);

    if ((va + size) > (1UL << mmu_va_bits()))
        return 0;

    for (off = 0; off < size; )
    {
        uint64_t v = va + off;
        uint64_t *l1 = mmu_root();
        uint64_t *l1e, *l2, *l3, *slot;

        if (!l1)
            return 0;

        l1e = &l1[(v >> MMU_L1_SHIFT) & (mmu_l1_entries() - 1)];

        /* Nothing mapped in this gigabyte at all */
        if (!(*l1e & MMU_DESC_VALID))
        {
            off += MMU_L1_SIZE - (v & (MMU_L1_SIZE - 1));
            continue;
        }

        l2 = mmu_next(l1e, MMU_L2_SIZE, MMU_DESC_BLOCK);
        if (!l2)
            return 0;

        slot = &l2[(v >> MMU_L2_SHIFT) & (MMU_TBL_ENTRIES - 1)];

        if (!(*slot & MMU_DESC_VALID))
        {
            off += MMU_L2_SIZE - (v & (MMU_L2_SIZE - 1));
            continue;
        }

        /* A whole 2MB block goes in one store */
        if (!(v & (MMU_L2_SIZE - 1)) && (size - off) >= MMU_L2_SIZE &&
            !(*slot & MMU_DESC_TABLE))
        {
            *slot = 0;
            off += MMU_L2_SIZE;
            continue;
        }

        l3 = mmu_next(slot, MMU_L3_SIZE, MMU_DESC_PAGE);
        if (!l3)
            return 0;

        l3[(v >> MMU_L3_SHIFT) & (MMU_TBL_ENTRIES - 1)] = 0;
        off += MMU_L3_SIZE;
    }

    mmu_commit();

    return 1;
}

/*
 * KrnMapGlobal() (see mapglobal.c). MAP_CacheInhibit and MAP_Guarded
 * mean Device-nGnRnE, MAP_WriteThrough means Normal Non-Cacheable.
 * MAP_Supervisor is ignored - the whole OS runs at EL1.
 */
int krnMMUMap(void *virt, void *phys, uint32_t length, KRN_MapAttr flags)
{
    uint64_t attr = MMU_ATTR_AF;

    D(bug("[Kernel] krnMMUMap(virt=%p, phys=%p, len=%08x, flags=%04x)\n",
          virt, phys, length, flags));

    if (!SysBase || !length)
        return 0;

    /* No access at all would be a descriptor with no purpose */
    if (!(flags & (MAP_Readable | MAP_Writable | MAP_Executable)))
        return 0;

    if (flags & (MAP_CacheInhibit | MAP_Guarded))
        attr |= MMU_ATTR_IDX(MAIR_IDX_DEVICE) | MMU_ATTR_SH_NONE;
    else if (flags & MAP_WriteThrough)
        attr |= MMU_ATTR_IDX(MAIR_IDX_NORMAL_NC) | MMU_ATTR_SH_INNER;
    else
        attr |= MMU_ATTR_IDX(MAIR_IDX_NORMAL_WB) | MMU_ATTR_SH_INNER;

    attr |= (flags & MAP_Writable) ? MMU_ATTR_AP_RW : MMU_ATTR_AP_RO;

    /*
     * Only device memory is no-execute, as in the boot tables. AROS runs
     * code out of ordinary allocations, so RAM a caller remaps and later
     * frees has to stay executable for its next owner.
     */
    if (flags & (MAP_CacheInhibit | MAP_Guarded))
        attr |= MMU_ATTR_PXN | MMU_ATTR_UXN;

    return mmu_map_range((uint64_t)(uintptr_t)virt, (uint64_t)(uintptr_t)phys,
                         length, attr);
}

/* KrnUnmapGlobal() (see unmapglobal.c) */
int krnMMUUnmap(void *virt, uint32_t length)
{
    D(bug("[Kernel] krnMMUUnmap(virt=%p, len=%08x)\n", virt, length));

    if (!SysBase || !length)
        return 0;

    return mmu_unmap_range((uint64_t)(uintptr_t)virt, length);
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
