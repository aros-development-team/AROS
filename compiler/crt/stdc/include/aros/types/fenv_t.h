#ifndef _AROS_TYPES_FENV_T_H
#define _AROS_TYPES_FENV_T_H

/*
    Copyright © 2016-2026, The AROS Development Team. All rights reserved.
    $Id$

    fenv_t / fexcept_t type definitions.

    fenv.c and <aros/stdc/fenv.h> need just the floating-point environment types,
    without the architecture's static-inline fenv functions (which would clash
    with the out-of-line implementations in fenv.c). The full definitions live in
    each <aros/fenv.h>; this header mirrors only the types, dispatched by CPU.
*/

#include <aros/cpu.h>
#include <aros/types/int_t.h>

#if defined(__aarch64__) || defined(__arm__) || defined(__ppc__) || defined(__powerpc__)

typedef uint32_t        fenv_t;
typedef uint32_t        fexcept_t;

#elif defined(__riscv)

typedef uint64_t        fenv_t;
typedef uint64_t        fexcept_t;

#elif defined(__x86_64__)

typedef struct {
        struct {
                uint32_t        __control;
                uint32_t        __status;
                uint32_t        __tag;
                char            __other[16];
        } __x87;
        uint32_t                __mxcsr;
} fenv_t;
typedef uint16_t        fexcept_t;

#elif defined(__i386__)

typedef struct {
        uint16_t        __control;
        uint16_t        __mxcsr_hi;
        uint16_t        __status;
        uint16_t        __mxcsr_lo;
        uint32_t        __tag;
        char            __other[16];
} fenv_t;
typedef uint16_t        fexcept_t;

#else
#error fenv_t is not defined for this architecture
#endif

#endif /* _AROS_TYPES_FENV_T_H */
