/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: boot.c
    Lang: english
*/

#include <inttypes.h>
#include <asm/arm/cpu.h>
#include "boot.h"

__startup void bootstrap(uintptr_t a0, uintptr_t a1, uintptr_t a2)
{
	uint32_t tmp;

	/* Enable NEON and VFP */
    asm volatile ("mrc p15, 0, %0, c1, c0, 2":"=r"(tmp));
    tmp |= 3 << 20;
    tmp |= 3 << 22;
    asm volatile ("mcr p15, 0, %0, c1, c0, 2"::"r"(tmp));

    fmxr(cr8, fmrx(cr8) | 1 << 30);


}


