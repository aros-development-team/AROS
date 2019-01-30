#ifndef ASM_CPU_H
#define ASM_CPU_H

/*
    Copyright � 1995-2016, The AROS Development Team. All rights reserved.
    $Id$

    CPU-specific assembler definitions.
    This file and included one describe hardware-level control structures
    and registers. Use them with care, only if you know what you are doing
    and why.
*/

/* Include the actual CPU-dependent definitions */
#ifdef __x86_64__
#   include <asm/x86_64/cpu.h>
#endif
#ifdef __i386__
#   include <asm/i386/cpu.h>
#endif
#ifdef __powerpc__
#   include <asm/ppc/cpu.h>
#endif
#ifdef __arm__
#   if defined __thumb2__
#       if defined __ARMEB__
#           include <asm/armeb/cpu-thumb2.h>
#       else
#           include <asm/arm/cpu-thumb2.h>
#       endif
#   else
#       if defined __ARMEB__
#           include <asm/armeb/cpu.h>
#       else
#           include <asm/arm/cpu.h>
#       endif
#   endif
#endif

/* Some default generic definitions. */
#ifndef HALT
#define HALT
#endif

#endif
