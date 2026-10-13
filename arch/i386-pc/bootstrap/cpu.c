/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>
#include <runtime.h>

#include "bootstrap.h"

void setup_mmu(void)
{
}

void kick(void *kick_base, struct TagItem *km)
{
    IPTR (*entry)(struct TagItem *, ULONG) = kick_base;

    kprintf("[BOOT] Entering kernel at 0x%p...\n", entry);
    entry(km, AROS_BOOT_MAGIC);
}
