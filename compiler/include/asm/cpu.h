#ifndef ASM_CPU_H
#define ASM_CPU_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    CPU-specific assembler definitions.
    This file and included one describe hardware-level control structures
    and registers. Use them with care, only if you know what you are doing
    and why.
*/

/* Include the actual CPU-dependent definitions */
#ifdef __x86_64__
#  include <asm/x86_64/cpu.h>
#endif
#ifdef __i386__
#  include <asm/i386/cpu.h>
#endif
#ifdef __powerpc__
#  include <asm/ppc/cpu.h>
#endif
#ifdef __arm__
#  include <asm/arm/cpu.h>
#endif

/* Some default generic definitions. */
#ifndef HALT
#define HALT
#endif

#endif
