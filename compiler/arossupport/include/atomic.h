/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROS_ATOMIC_H
#define AROS_ATOMIC_H

#include <exec/types.h>

#ifdef __i386__

#define __AROS_ATOMIC_INC_B(var) \
    __asm__ __volatile__ ("incb %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_INC_W(var) \
    __asm__ __volatile__ ("incw %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_INC_L(var) \
    __asm__ __volatile__ ("incl %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")

#define __AROS_ATOMIC_DEC_B(var) \
    __asm__ __volatile__ ("decb %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_DEC_W(var) \
    __asm__ __volatile__ ("decw %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_DEC_L(var) \
    __asm__ __volatile__ ("decl %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")

#define __AROS_ATOMIC_AND_B(var, mask) \
    __asm__ __volatile__ ("andb %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_AND_W(var, mask) \
    __asm__ __volatile__ ("andw %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_AND_L(var, mask) \
    __asm__ __volatile__ ("andl %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")


#define __AROS_ATOMIC_OR_B(var, mask) \
    __asm__ __volatile__ ("orb %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_OR_W(var, mask) \
    __asm__ __volatile__ ("orw %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_OR_L(var, mask) \
    __asm__ __volatile__ ("orl %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")

#elif defined(__powerpc__) || defined(__ppc__)

void atomic_inc_b(BYTE *p);
void atomic_dec_b(BYTE *p);

void atomic_inc_w(WORD *p);
void atomic_dec_w(WORD *p);

void atomic_inc_l(LONG *p);
void atomic_dec_l(LONG *p);

void atomic_and_b(BYTE *p, BYTE mask);
void atomic_and_w(WORD *p, WORD mask);
void atomic_and_l(LONG *p, LONG mask);

void atomic_or_b(BYTE *p, BYTE mask);
void atomic_or_w(WORD *p, WORD mask);
void atomic_or_l(LONG *p, LONG mask);

#define __AROS_ATOMIC_INC_B(var) atomic_inc_b((BYTE *) &(var))
#define __AROS_ATOMIC_DEC_B(var) atomic_dec_b((BYTE *) &(var))

#define __AROS_ATOMIC_INC_W(var) atomic_inc_w((WORD *) &(var))
#define __AROS_ATOMIC_DEC_W(var) atomic_dec_w((WORD *) &(var))

#define __AROS_ATOMIC_INC_L(var) atomic_inc_w(&(var))
#define __AROS_ATOMIC_DEC_L(var) atomic_dec_w(&(var))

#define __AROS_ATOMIC_AND_B(var, mask) atomic_and_b((BYTE *) &(var), (mask))
#define __AROS_ATOMIC_AND_W(var, mask) atomic_and_w((WORD *) &(var), (mask))
#define __AROS_ATOMIC_AND_L(var, mask) atomic_and_l(&(var), (mask))

#define __AROS_ATOMIC_OR_B(var, mask) atomic_or_b((BYTE *) &(var), (mask))
#define __AROS_ATOMIC_OR_W(var, mask) atomic_or_w((WORD *)&(var), (mask))
#define __AROS_ATOMIC_OR_L(var, mask) atomic_or_l(&(var), (mask))

#else

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

#define AROS_ATOMIC_INC(var)       __AROS_ATOMIC(INC, (var))
#define AROS_ATOMIC_DEC(var)       __AROS_ATOMIC(DEC, (var))
#define AROS_ATOMIC_AND(var, mask) __AROS_ATOMIC(AND, (var), (mask))
#define AROS_ATOMIC_OR(var,  mask) __AROS_ATOMIC(OR,  (var), (mask))

#endif /* AROS_ATOMIC_H */
