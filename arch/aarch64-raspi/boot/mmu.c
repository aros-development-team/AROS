/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    Desc: mmu.c - AArch64 MMU setup for Raspberry Pi 3 boot
*/

#include <stdint.h>
#include "bcm2708_boot.h"
#include "mmu.h"
#include "boot.h"

#define DMMU(x)

/*
 * AArch64 MMU page table descriptors (4KB granule)
 *
 * Level 0 (PGD): each entry covers 512 GB
 * Level 1 (PUD): each entry covers 1 GB (can be 1GB block descriptor)
 * Level 2 (PMD): each entry covers 2 MB (we use 2MB block descriptors)
 * Level 3 (PTE): each entry covers 4 KB (not used in boot)
 *
 * Descriptor format for block (level 1 or 2):
 *   [63:52] Upper attributes
 *   [51:n]  Output address (n depends on level)
 *   [n-1:12] Reserved
 *   [11:2]  Lower attributes
 *   [1]     Valid = 1
 *   [0]     Block = 0 (block), Table = 1 (points to next level)
 *
 * For level 2 block (2MB):
 *   [47:21] Output address bits [47:21]
 *   [1:0]   0b01 = valid block
 *
 * Table descriptor (level 0 or 1):
 *   [47:12] Next-level table address bits [47:12]
 *   [1:0]   0b11 = valid table
 */

/* Descriptor types */
#define DESC_INVALID    0x0UL
#define DESC_BLOCK      0x1UL   /* Valid block descriptor (level 1 or 2) */
#define DESC_TABLE      0x3UL   /* Valid table descriptor (level 0 or 1) */

/* Block descriptor attribute fields */
#define ATTR_IDX(idx)   ((uint64_t)(idx) << 2)     /* AttrIndx[2:0] in MAIR_EL1 */
#define ATTR_AP_RW_EL1  (0x0UL << 6)               /* AP[2:1] = 00: EL1 RW, EL0 no access */
#define ATTR_AP_RW_ALL  (0x1UL << 6)               /* AP[2:1] = 01: EL1 RW, EL0 RW */
#define ATTR_AP_RO_EL1  (0x2UL << 6)               /* AP[2:1] = 10: EL1 RO, EL0 no access */
#define ATTR_AP_RO_ALL  (0x3UL << 6)               /* AP[2:1] = 11: EL1 RO, EL0 RO */
#define ATTR_SH_NON     (0x0UL << 8)               /* Non-shareable */
#define ATTR_SH_OUTER   (0x2UL << 8)               /* Outer shareable */
#define ATTR_SH_INNER   (0x3UL << 8)               /* Inner shareable */
#define ATTR_AF         (0x1UL << 10)               /* Access Flag */
#define ATTR_nG         (0x1UL << 11)               /* non-Global */
#define ATTR_PXN        (1UL << 53)                 /* Privileged Execute Never */
#define ATTR_UXN        (1UL << 54)                 /* Unprivileged Execute Never */

/*
 * MAIR_EL1 attribute indices:
 *   0: Device-nGnRnE  (0x00)  - for MMIO
 *   1: Normal WB Cacheable (0xFF) - for RAM (Inner/Outer Write-Back, Read/Write Allocate)
 *   2: Normal Non-Cacheable (0x44) - for non-cached RAM
 */
#define MAIR_ATTR0_DEVICE   0x00UL
#define MAIR_ATTR1_NORMAL   0xFFUL
#define MAIR_ATTR2_NC       0x44UL

#define MAIR_VALUE  ((MAIR_ATTR0_DEVICE << 0) | (MAIR_ATTR1_NORMAL << 8) | (MAIR_ATTR2_NC << 16))

/* MAIR index shortcuts */
#define ATTR_DEVICE     ATTR_IDX(0)
#define ATTR_NORMAL     ATTR_IDX(1)
#define ATTR_NORMAL_NC  ATTR_IDX(2)

/* Page table pointers */
static uint64_t *pud;
static uint64_t *pmd;

void mmu_init(void)
{
    int i;

    /*
     * The level-1 table lives in the bm_pgd page (0x7000): level 0 is unused
     * with T0SZ=25, and the bm_pud page (0x8000) turned out to be written by
     * another agent on the BCM2712 (the built-in armstub parking the
     * secondary cores, by all evidence: constant content, invisible to a
     * verified CPU watchpoint). 0x8000 is left as a sentinel-filled honeypot
     * so its writes stay observable.
     */
    pud = (uint64_t *)BOOTMEMADDR(bm_pgd);
    pmd = (uint64_t *)BOOTMEMADDR(bm_pmd);

    /* Clear the tables; sentinel-fill the ceded 0x8000 page */
    for (i = 0; i < 512; i++)
        pud[i] = 0;

    for (i = 0; i < 512; i++)
        ((uint64_t *)BOOTMEMADDR(bm_pud))[i] = 0xDEADBEEF00000000UL | i;

    /* Clear PMD: 4 pages covering 4GB (4 * 512 = 2048 entries) */
    for (i = 0; i < 2048; i++)
        pmd[i] = 0;

    /*
     * Set up the page table hierarchy:
     * PUD[0] -> PMD page 0 (covers 0x00000000 - 0x3FFFFFFF, 1GB)
     * PUD[1] -> PMD page 1 (covers 0x40000000 - 0x7FFFFFFF, 1GB)
     * PUD[2] -> PMD page 2 (covers 0x80000000 - 0xBFFFFFFF, 1GB)
     * PUD[3] -> PMD page 3 (covers 0xC0000000 - 0xFFFFFFFF, 1GB)
     * Entries >= 4 get 1GB blocks on demand (mmu_map_section high path).
     */
    pud[0] = (uint64_t)(uintptr_t)&pmd[0 * 512] | DESC_TABLE;
    pud[1] = (uint64_t)(uintptr_t)&pmd[1 * 512] | DESC_TABLE;
    pud[2] = (uint64_t)(uintptr_t)&pmd[2 * 512] | DESC_TABLE;
    pud[3] = (uint64_t)(uintptr_t)&pmd[3 * 512] | DESC_TABLE;

    /*
     * The RP1 windows sit under /axi/pcie, which no DT walk touches, so
     * they stay hardcoded. The boot UART lives behind them - without
     * these, serial dies the moment the MMU loads.
     */
    pud[0x1C00000000UL >> 30] = DESC_BLOCK | 0x1C00000000UL | ATTR_DEVICE |
                                ATTR_AF | ATTR_AP_RW_EL1 | ATTR_SH_NON |
                                ATTR_PXN | ATTR_UXN;   /* BCM2712 D0 */
    pud[0x1F00000000UL >> 30] = DESC_BLOCK | 0x1F00000000UL | ATTR_DEVICE |
                                ATTR_AF | ATTR_AP_RW_EL1 | ATTR_SH_NON |
                                ATTR_PXN | ATTR_UXN;   /* BCM2712 C1 */
}

/*
 * Return the level-2 slot for a virtual address, or NULL if the address
 * is outside the translated ranges (first 4GB).
 */
static uint64_t *mmu_slot(uintptr_t virt)
{
    if (virt < 0x100000000UL)
        return &pmd[virt >> 21];
    return (uint64_t *)0;
}

void mmu_load(void)
{
    uint64_t tmp;

    aarch64_flush_cache((uintptr_t)pud, PUD_SIZE);
    aarch64_flush_cache((uintptr_t)pmd, PMD_SIZE);

    /* Set MAIR_EL1 */
    tmp = MAIR_VALUE;
    asm volatile ("msr MAIR_EL1, %0" :: "r"(tmp));

    /*
     * Set TCR_EL1:
     *   T0SZ = 25  (39-bit VA space = 512GB, bits [5:0]; reaches the BCM2711
     *               PCIe outbound window at 0x6_0000_0000 and the BCM2712
     *               peripheral and RP1 windows at 0x10_7C00_0000 and
     *               0x1F_0000_0000. Lookup still starts at level 1 for
     *               T0SZ 25-33 with a 4KB granule, so TTBR0 holds the
     *               level-1 table.)
     *   IRGN0 = 01 (Inner Write-Back Write-Allocate Cacheable, bits [9:8])
     *   ORGN0 = 01 (Outer Write-Back Write-Allocate Cacheable, bits [11:10])
     *   SH0 = 11   (Inner Shareable, bits [13:12])
     *   TG0 = 00   (4KB granule, bits [15:14])
     *   IPS = 010  (40-bit physical address space, bits [34:32]; the BCM2712
     *               windows need 37 PA bits, and 40 is Cortex-A76's PARange)
     */
    tmp = (25UL << 0)       /* T0SZ = 25 -> 512GB VA space */
        | (0x1UL << 8)      /* IRGN0 = 01 */
        | (0x1UL << 10)     /* ORGN0 = 01 */
        | (0x3UL << 12)     /* SH0 = 11 (inner shareable) */
        | (0x0UL << 14)     /* TG0 = 00 (4KB granule) */
        | (0x1UL << 23)     /* EPD1 = 1: TTBR1 walks disabled (TTBR1 is
                             * never programmed; leaving it enabled with
                             * T1SZ=0 is CONSTRAINED UNPREDICTABLE) */
        | (0x2UL << 32);    /* IPS = 010 (40-bit PA) */
    asm volatile ("msr TCR_EL1, %0" :: "r"(tmp));
    asm volatile ("isb");

    /*
     * Set TTBR0_EL1 to the Level 1 (PUD) table.
     * With T0SZ=25 (39-bit VA) and 4KB granule, the initial lookup
     * level is 1, so TTBR0 must point to the Level 1 table directly.
     * Level 0 (PGD) is not used.
     */
    asm volatile ("msr TTBR0_EL1, %0" :: "r"((uint64_t)(uintptr_t)pud));
    asm volatile ("isb");

    /* Invalidate TLB */
    asm volatile ("tlbi vmalle1");
    asm volatile ("dsb sy");
    asm volatile ("isb");

    /* Enable MMU via SCTLR_EL1 */
    asm volatile ("mrs %0, SCTLR_EL1" : "=r"(tmp));
    tmp |= (1UL << 0);     /* M - MMU enable */
    tmp |= (1UL << 2);     /* C - Data cache enable */
    tmp |= (1UL << 12);    /* I - Instruction cache enable */
    tmp &= ~(1UL << 1);    /* A - Alignment check disable */
    asm volatile ("dsb sy");
    asm volatile ("msr SCTLR_EL1, %0" :: "r"(tmp));
    asm volatile ("isb");

    DMMU(kprintf("[BOOT] MMU enabled\n"));
}

void mmu_unmap_section(uintptr_t virt, uintptr_t length)
{
    /* Round down to 2MB boundary */
    uintptr_t start = virt & ~(2*1024*1024UL - 1);
    uintptr_t end = (virt + length + 2*1024*1024UL - 1) & ~(2*1024*1024UL - 1);

    while (start < end)
    {
        uint64_t *slot = mmu_slot(start);
        if (slot)
            *slot = 0;
        start += 2*1024*1024UL;
    }
}

/*
 * Map a section of memory using 2MB block descriptors.
 *
 * Parameters match the ARM32 version for compatibility:
 *   phys, virt, length - physical and virtual addresses and size
 *   normal  - 1 = normal memory, 0 = device memory
 *   cacheable - 1 = cacheable (only meaningful for normal memory)
 *   ap      - access permissions (simplified: 2=RO, 3=RW at EL1)
 *   tex     - not used directly in AArch64, kept for API compatibility
 *             (1 = cacheable, 0 = device)
 */
void mmu_map_section(uintptr_t phys, uintptr_t virt, uintptr_t length, int normal, int cacheable, int ap, int tex)
{
    /* Round to 2MB boundaries */
    uintptr_t start = virt & ~(2*1024*1024UL - 1);
    uintptr_t end = (virt + length + 2*1024*1024UL - 1) & ~(2*1024*1024UL - 1);
    uintptr_t count = (end - start) >> 21;
    uintptr_t va = start;
    uintptr_t pa = phys >> 21;

    DMMU(kprintf("[BOOT] MMU map %p:%p->%p:%p, normal=%d, cacheable=%d, ap=%x, tex=%x\n",
            phys, phys+length-1, virt, virt+length-1, normal, cacheable, ap, tex));

    /*
     * Above 4GB everything on these boards is large, uniform and
     * GB-aligned (peripheral windows, high RAM), and the static boot
     * tables have no PMD pages to spare - use 1GB PUD blocks. Below
     * 4GB the 2MB PMD path stays: the first four PUD slots are table
     * pointers and must not be clobbered.
     */
    if (virt >= 0x100000000UL)
    {
        if ((virt | phys) & ((1UL << 30) - 1))
        {
            kprintf("[BOOT] MMU: unaligned high mapping %p dropped\n", virt);
            return;
        }

        while (length)
        {
            uint64_t *slot = &pud[virt >> 30];
            uint64_t desc = DESC_BLOCK | ATTR_AF | (uint64_t)phys;

            /* same attributelogic as the 2MB-way */
            if (normal && (cacheable || tex))
                desc |= ATTR_NORMAL | ATTR_SH_INNER;
            else if (normal)
                desc |= ATTR_NORMAL_NC | ATTR_SH_INNER;
            else
                desc |= ATTR_DEVICE | ATTR_SH_NON | ATTR_PXN | ATTR_UXN;
            desc |= (ap == 2) ? ATTR_AP_RO_EL1 : ATTR_AP_RW_EL1;

            if ((*slot & 3) != DESC_TABLE)     /* never overwrite a table */
                *slot = desc;

            virt   += (1UL << 30);
            phys   += (1UL << 30);
            length -= (length > (1UL << 30)) ? (1UL << 30) : length;
        }
        return;
    }

    while (count--)
    {
        uint64_t desc = DESC_BLOCK | ATTR_AF;

        if (normal && (cacheable || tex))
        {
            /* Normal cacheable memory */
            desc |= ATTR_NORMAL;
            desc |= ATTR_SH_INNER;
        }
        else if (normal)
        {
            /* Normal non-cacheable memory */
            desc |= ATTR_NORMAL_NC;
            desc |= ATTR_SH_INNER;
        }
        else
        {
            /* Device memory */
            desc |= ATTR_DEVICE;
            desc |= ATTR_SH_NON;
            desc |= ATTR_PXN;    /* No execute for device memory */
            desc |= ATTR_UXN;
        }

        /* Access permissions */
        if (ap == 2)
            desc |= ATTR_AP_RO_EL1;
        else
            desc |= ATTR_AP_RW_EL1;

        desc |= (pa << 21);

        {
            uint64_t *slot = mmu_slot(va);
            if (slot)
                *slot = desc;
        }

        pa++;
        va += 2*1024*1024UL;
    }
}
