/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: boot.c
    Lang: english
*/

#include <inttypes.h>
#include <asm/cpu.h>
#include "boot.h"
#include "serialdebug.h"
#include "atags.h"

static void parse_atags(struct tag *tags)
{
	struct tag *t = NULL;

	kprintf("Parsing ATAGS\n");

	for_each_tag(t, tags)
	{
		kprintf("  %08x: \n\r", t->hdr.tag, t->hdr.size);
	}
}

__startup void bootstrap(uintptr_t dummy, uintptr_t arch, struct tag * atags)
{
	uint32_t tmp;

	/* Enable NEON and VFP */
    asm volatile ("mrc p15, 0, %0, c1, c0, 2":"=r"(tmp));
    tmp |= 3 << 20;
    tmp |= 3 << 22;
    asm volatile ("mcr p15, 0, %0, c1, c0, 2"::"r"(tmp));

    fmxr(cr8, fmrx(cr8) | 1 << 30);

    kprintf("AROS for EfikaMX bootstrap\n\r");

    parse_atags(atags);

    while(1);
}
