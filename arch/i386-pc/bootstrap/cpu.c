/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>
#include <runtime.h>

#include "bootstrap.h"

void setup_mmu(void)
{
}

/*
 * The kickstart's entry point is kernel_entry(struct TagItem *bootMsg,
 * ULONG magic) (arch/i386-pc/kernel/kernel_startup.c). Spell the prototype
 * out: `int (*entry)()` meant "unspecified arguments" up to C17 but is
 * `(void)` from C23 on, which gcc 15 defaults to, and the two-argument call
 * then no longer compiles.
 */
void kick(void *kick_base, struct TagItem64 *km)
{
    unsigned long (*entry)(void *, unsigned long) = kick_base;

    kprintf("[BOOT] Entering kernel at 0x%p...\n", entry);
    entry(km, AROS_BOOT_MAGIC);
}
