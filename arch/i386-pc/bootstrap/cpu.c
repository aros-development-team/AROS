/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <runtime.h>

void setup_mmu(void)
{
}

void kick(int (*entry)(), void *km)
{
    kprintf("[BOOT] Entering kernel at 0x%p...\n", entry);
    entry(km, AROS_BOOT_MAGIC);
}
