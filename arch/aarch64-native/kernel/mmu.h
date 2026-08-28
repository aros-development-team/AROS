/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    AArch64 MMU definitions for kernel.
*/

#ifndef _MMU_H
#define _MMU_H

#define ENABLE_MMU      (1 << 0)
#define ENABLE_D_CACHE  (1 << 2)
#define ENABLE_I_CACHE  (1 << 12)

void core_MMUUpdatePageTables(void);

/* KrnMapGlobal()/KrnUnmapGlobal() proper, see mmu.c */
int krnMMUMap(void *virt, void *phys, uint32_t length, KRN_MapAttr flags);
int krnMMUUnmap(void *virt, uint32_t length);

#endif /* _MMU_H */
