#ifndef ASM_CPU_H
#define ASM_CPU_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: io.h 37974 2011-04-01 06:37:44Z sonic $

    CPU-specific assembler definitions.
    This file and included one describe hardware-level control structures
    and registers. Use them with care, only if you know what you are doing
    and why.
*/

/* Include the actual CPU-dependent definitions */
#if defined __x86_64__
#  include <asm/x86_64/cpu.h>

/* TODO: add other architectures */

#endif

/* Some default generic definitions. */
#ifndef HALT
#define HALT
#endif

#endif
