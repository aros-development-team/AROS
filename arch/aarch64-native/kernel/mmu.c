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

/* TCR_EL1 configuration: 4KB granule, 32-bit VA (T0SZ=32), 36-bit PA */
#define TCR_VALUE ( \
    (32UL << 0)      | /* T0SZ = 32 (4GB VA space) */ \
    (0UL  << 7)      | /* EPD0 = 0 (TTBR0 enabled) */ \
    (1UL  << 8)      | /* IRGN0 = Write-Back */ \
    (1UL  << 10)     | /* ORGN0 = Write-Back */ \
    (3UL  << 12)     | /* SH0 = Inner Shareable */ \
    (0UL  << 14)     | /* TG0 = 4KB granule */ \
    (1UL  << 23)     | /* EPD1 = 1 (TTBR1 disabled) */ \
    (1UL  << 31)       /* RES1 */ \
)


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
