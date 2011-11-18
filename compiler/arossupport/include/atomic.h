/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROS_ATOMIC_H
#define AROS_ATOMIC_H

#include <exec/types.h>

#if defined(__i386__)
#include <aros/i386/atomic.h>
#elif defined(__x86_64__)
#include <aros/x86_64/atomic.h>
#elif defined(__powerpc__) || defined(__ppc__)
#include <aros/ppc/atomic.h>
#elif defined(__arm__)
#include <aros/arm/atomic.h>
#elif defined(__mc68000)
#include <aros/m68k/atomic.h>
#endif

/* Porting to other archs? Just define your asm atomics as above... else deadlock below will hit you! */
#ifndef __AROS_ATOMIC_INC_L

#include <proto/exec.h>
#define AROS_NO_ATOMIC_OPERATIONS

#define __AROS_ATOMIC_INC_B(var) do {Disable(); (var)++; Enable(); } while(0)
#define __AROS_ATOMIC_INC_W(var) do {Disable(); (var)++; Enable(); } while(0)
#define __AROS_ATOMIC_INC_L(var) do {Disable(); (var)++; Enable(); } while(0)

#define __AROS_ATOMIC_DEC_B(var) do {Disable(); (var)--; Enable(); } while(0)
#define __AROS_ATOMIC_DEC_W(var) do {Disable(); (var)--; Enable(); } while(0)
#define __AROS_ATOMIC_DEC_L(var) do {Disable(); (var)--; Enable(); } while(0)

#define __AROS_ATOMIC_AND_B(var, mask) do {Disable(); (var) &= (mask); Enable(); } while(0)
#define __AROS_ATOMIC_AND_W(var, mask) do {Disable(); (var) &= (mask); Enable(); } while(0)
#define __AROS_ATOMIC_AND_L(var, mask) do {Disable(); (var) &= (mask); Enable(); } while(0)

#define __AROS_ATOMIC_OR_B(var, mask) do {Disable(); (var) |= (mask); Enable(); } while(0)
#define __AROS_ATOMIC_OR_W(var, mask) do {Disable(); (var) |= (mask); Enable(); } while(0)
#define __AROS_ATOMIC_OR_L(var, mask) do {Disable(); (var) |= (mask); Enable(); } while(0)

#endif

#ifdef __x86_64__
/* Don't rely on sizeof(LONG) and I want it this way for now */

#define __AROS_ATOMIC(__instr__, var, args...)               \
do                                                           \
{                                                            \
    struct atomic_size                                       \
    {                                                        \
        int unsupported_atomic_size                          \
        [                                                    \
            (sizeof(var) != 1 &&                             \
            sizeof(var) != 2 &&                              \
            sizeof(var) != 4 &&                              \
            sizeof(var) != 8) ? -1 : 1                       \
        ];                                                   \
    };                                                       \
                                                             \
    if (sizeof(var) == 1)                                    \
        __AROS_ATOMIC_ ## __instr__ ## _B((var) , ## args);  \
    else                                                     \
    if (sizeof(var) == 2)                                    \
        __AROS_ATOMIC_ ## __instr__ ## _W((var) , ## args);  \
    else                                                     \
    if (sizeof(var) == 4)                                    \
        __AROS_ATOMIC_ ## __instr__ ## _L((var) , ## args);  \
    else                                                     \
    if (sizeof(var) == 8)                                    \
        __AROS_ATOMIC_ ## __instr__ ## _Q((var) , ## args);  \
} while (0)

#else

#define __AROS_ATOMIC(__instr__, var, args...)               \
do                                                           \
{                                                            \
    struct atomic_size                                       \
    {                                                        \
        int unsupported_atomic_size                          \
        [                                                    \
            sizeof(var) != sizeof(BYTE) &&                   \
            sizeof(var) != sizeof(WORD) &&                   \
            sizeof(var) != sizeof(LONG) ? -1 : 1             \
        ];                                                   \
    };                                                       \
                                                             \
    if (sizeof(var) == sizeof(BYTE))                         \
        __AROS_ATOMIC_ ## __instr__ ## _B((var) , ## args);  \
    else                                                     \
    if (sizeof(var) == sizeof(WORD))                         \
        __AROS_ATOMIC_ ## __instr__ ## _W((var) , ## args);  \
    else                                                     \
    if (sizeof(var) == sizeof(LONG))                         \
        __AROS_ATOMIC_ ## __instr__ ## _L((var) , ## args);  \
} while (0)

#endif


#define AROS_ATOMIC_INC(var)       __AROS_ATOMIC(INC, (var))
#define AROS_ATOMIC_DEC(var)       __AROS_ATOMIC(DEC, (var))
#define AROS_ATOMIC_AND(var, mask) __AROS_ATOMIC(AND, (var), (mask))
#define AROS_ATOMIC_OR(var,  mask) __AROS_ATOMIC(OR,  (var), (mask))

#endif /* AROS_ATOMIC_H */
