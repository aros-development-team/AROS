#ifndef ASM_RISCV64_MMU_H
#define ASM_RISCV64_MMU_H

/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 64bit RISC-V MMU definitions (Sv39/Sv48)
    Lang: english
*/

/* Page table entry bits */
#define PTE_V           0x001   /* Valid                              */
#define PTE_R           0x002   /* Readable                           */
#define PTE_W           0x004   /* Writable                           */
#define PTE_X           0x008   /* Executable                        */
#define PTE_U           0x010   /* User accessible                    */
#define PTE_G           0x020   /* Global                             */
#define PTE_A           0x040   /* Accessed                           */
#define PTE_D           0x080   /* Dirty                              */

/* A PTE with none of R/W/X set is a pointer to the next level */
#define PTE_LEAF_MASK   (PTE_R | PTE_W | PTE_X)

/* Physical page number field */
#define PTE_PPN_SHIFT   10
#define PTE_TO_PA(pte)  (((pte) >> PTE_PPN_SHIFT) << 12)
#define PA_TO_PTE(pa)   ((((unsigned long)(pa)) >> 12) << PTE_PPN_SHIFT)

/* satp register */
#define SATP_MODE_BARE  (0UL << 60)
#define SATP_MODE_SV39  (8UL << 60)
#define SATP_MODE_SV48  (9UL << 60)
#define SATP_PPN(pa)    (((unsigned long)(pa)) >> 12)

/* Sv39 geometry: 512-entry tables, 4KiB/2MiB/1GiB pages */
#define PT_ENTRIES      512
#define PAGE_SHIFT      12
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define MEGAPAGE_SIZE   (1UL << 21)
#define GIGAPAGE_SIZE   (1UL << 30)

#define SV39_VPN(va, level) ((((unsigned long)(va)) >> (12 + 9 * (level))) & 0x1FF)

#endif /* ASM_RISCV64_MMU_H */
