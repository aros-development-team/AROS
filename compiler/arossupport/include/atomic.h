/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROS_ATOMIC_H
#define AROS_ATOMIC_H

#include <exec/types.h>

#if defined(__i386__) || defined(__x86_64__)

/* lock op is supposed to make these ops atomic and in result SMP safe 
Atomic addition of an immediate value to a memory location. 
*/

#define __AROS_ATOMIC_INC_B(var) \
    __asm__ __volatile__ ("lock; incb %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_INC_W(var) \
    __asm__ __volatile__ ("lock; incw %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_INC_L(var) \
    __asm__ __volatile__ ("lock; incl %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")

#if defined(__x86_64)
#define __AROS_ATOMIC_INC_Q(var) \
    __asm__ __volatile__ ("lock; incq %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#endif


/* lock op is supposed to make these ops atomic and in result SMP safe 
Atomic subtraction of an immediate value from a memory location. 
*/

#define __AROS_ATOMIC_DEC_B(var) \
    __asm__ __volatile__ ("lock; decb %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_DEC_W(var) \
    __asm__ __volatile__ ("lock; decw %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_DEC_L(var) \
    __asm__ __volatile__ ("lock; decl %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")

#if defined(__x86_64)
#define __AROS_ATOMIC_DEC_Q(var) \
    __asm__ __volatile__ ("lock; decq %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#endif


/* lock op is supposed to make these ops atomic and in result SMP safe 
Atomic AND of an immediate value with a value at a memory location. 
*/

#define __AROS_ATOMIC_AND_B(var, mask) \
    __asm__ __volatile__ ("lock; andb %0,%1" : : "r" ((UBYTE)(mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_AND_W(var, mask) \
    __asm__ __volatile__ ("lock; andw %0,%1" : : "r" ((UWORD)(mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_AND_L(var, mask) \
    __asm__ __volatile__ ("lock; andl %0,%1" : : "r" ((ULONG)(mask)), "m" ((var)) : "memory", "cc")

#if defined(__x86_64)
#define __AROS_ATOMIC_AND_Q(var, mask) \
    __asm__ __volatile__ ("lock; andq %0,%1" : : "r" ((UQUAD)(mask)), "m" ((var)) : "memory", "cc")
#endif


/* lock op is supposed to make these ops atomic and in result SMP safe 
Atomic OR of an immediate value with a memory location. 
*/

#define __AROS_ATOMIC_OR_B(var, mask) \
    __asm__ __volatile__ ("lock; orb %0,%1" : : "r" ((UBYTE)(mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_OR_W(var, mask) \
    __asm__ __volatile__ ("lock; orw %0,%1" : : "r" ((UWORD)(mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_OR_L(var, mask) \
    __asm__ __volatile__ ("lock; orl %0,%1" : : "r" ((ULONG)(mask)), "m" ((var)) : "memory", "cc")

#if defined(__x86_64)
#define __AROS_ATOMIC_OR_Q(var, mask) \
    __asm__ __volatile__ ("lock; orq %0,%1" : : "r" ((UQUAD)(mask)), "m" ((var)) : "memory", "cc")
#endif

  
#elif defined(__powerpc__) || defined(__ppc__)

void atomic_inc_b(UBYTE* p);
void atomic_dec_b(UBYTE* p);

void atomic_inc_w(UWORD* p);
void atomic_dec_w(UWORD* p);

void atomic_inc_l(ULONG* p);
void atomic_dec_l(ULONG* p);

void atomic_and_b(UBYTE* p, UBYTE mask);
void atomic_and_w(UWORD* p, UWORD mask);
void atomic_and_l(ULONG* p, ULONG mask);

void atomic_or_b(UBYTE* p, UBYTE mask);
void atomic_or_w(UWORD* p, UWORD mask);
void atomic_or_l(ULONG* p, ULONG mask);

#define __AROS_ATOMIC_INC_B(var) atomic_inc_b((UBYTE *) &(var))
#define __AROS_ATOMIC_DEC_B(var) atomic_dec_b((UBYTE *) &(var))

#define __AROS_ATOMIC_INC_W(var) atomic_inc_w((UWORD *) &(var))
#define __AROS_ATOMIC_DEC_W(var) atomic_dec_w((UWORD *) &(var))

#define __AROS_ATOMIC_INC_L(var) atomic_inc_l((ULONG *) &(var))
#define __AROS_ATOMIC_DEC_L(var) atomic_dec_l((ULONG *) &(var))

#define __AROS_ATOMIC_AND_B(var, mask) atomic_and_b((UBYTE *) &(var), (mask))
#define __AROS_ATOMIC_AND_W(var, mask) atomic_and_w((UWORD *) &(var), (mask))
#define __AROS_ATOMIC_AND_L(var, mask) atomic_and_l((ULONG *) &(var), (mask))

#define __AROS_ATOMIC_OR_B(var, mask) atomic_or_b((UBYTE *) &(var), (mask))
#define __AROS_ATOMIC_OR_W(var, mask) atomic_or_w((UWORD *) &(var), (mask))
#define __AROS_ATOMIC_OR_L(var, mask) atomic_or_l((ULONG *) &(var), (mask))

#else
/* Porting to other archs? Just define your asm atomics as above... else deadlock below will hit you! */
#include <proto/exec.h>

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
