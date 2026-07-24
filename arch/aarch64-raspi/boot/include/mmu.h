/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    Desc: mmu.h - AArch64 MMU definitions for boot
*/

#ifndef _MMU_H_
#define _MMU_H_

#include <stdint.h>

void mmu_init(void);
void mmu_load(void);
void mmu_unmap_section(uintptr_t virt, uintptr_t length);
void mmu_map_section(uintptr_t phys, uintptr_t virt, uintptr_t length, int normal, int cacheable, int ap, int tex);

#endif /* _MMU_H_ */
