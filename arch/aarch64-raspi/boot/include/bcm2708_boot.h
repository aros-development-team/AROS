/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    Desc: AArch64 boot memory layout for BCM2836/2837 (Raspberry Pi 2/3)
*/

#ifndef _BCM2708_BOOT_H
#define _BCM2708_BOOT_H

#include <stdint.h>

/*
 * AArch64 boot memory layout.
 * On RPi3 in aarch64 mode, the firmware loads kernel.img at 0x80000.
 * We place our boot data structures in the low memory area (0x0 - 0x80000).
 *
 * Page table structure for AArch64 with 4KB granule:
 *   Level 0 (PGD): 512 entries, each covering 512GB -> 1 page  (4KB)
 *   Level 1 (PUD): 512 entries, each covering 1GB   -> 1 page  (4KB)
 *   Level 2 (PMD): 512 entries, each covering 2MB   -> 4 pages (16KB, for 4GB coverage)
 *
 * We use 2MB block descriptors at level 2 (similar to ARM32's 1MB sections).
 */

#define __bcm2708_vectsize              0x800    /* AArch64 exception vector table: 2KB */
#define __bcm2708_bootstacksize         (768 << 3)
#define __bcm2708_boottagssize          (64 << 4)

/* Page table sizes */
#define PGD_SIZE                        4096     /* Level 0: 512 entries x 8 bytes */
#define PUD_SIZE                        4096     /* Level 1: 512 entries x 8 bytes */
#define PMD_SIZE                        (4*4096) /* Level 2: 4 pages for 4GB coverage */

struct bcm2708bootmem
{
    uint8_t     bm_vectors[__bcm2708_vectsize];                          /* 0x0000 */
    uint8_t     bm_padding1[(0x4000 - __bcm2708_vectsize - __bcm2708_bootstacksize - __bcm2708_boottagssize - 16)];
    uint8_t     bm_boottags[__bcm2708_boottagssize];
    uint8_t     bm_bootstack[__bcm2708_bootstacksize];
    uint8_t     bm_padding2[16];
    /* 0x4000 */
    uint8_t     bm_mboxmsg[0x1000];                                     /* 0x4000 - VideoCore mailbox */
    uint8_t     bm_mctrampoline[0x2000];                                 /* 0x5000 - SMP trampoline */
    /* 0x7000 - Page tables */
    uint8_t     bm_pgd[PGD_SIZE];                                        /* Level 0 page table */
    uint8_t     bm_pud[PUD_SIZE];                                        /* Level 1 page table */
    uint8_t     bm_pmd[PMD_SIZE];                                        /* Level 2 page tables (2MB blocks) */
}  __attribute__((packed));

#define BOOTMEMADDR(offset) (&(((struct bcm2708bootmem *)0x0)->offset))

#endif	/* _BCM2708_BOOT_H */
