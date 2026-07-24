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

#endif /* _MMU_H */
