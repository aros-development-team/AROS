/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROS_ATOMIC_H
#define AROS_ATOMIC_H

#ifdef __i386__

#define AROS_ATOMIC_INCB(var) \
    __asm__ __volatile__ ("incb %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define AROS_ATOMIC_INCW(var) \
    __asm__ __volatile__ ("incw %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define AROS_ATOMIC_INCL(var) \
    __asm__ __volatile__ ("incl %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")

#define AROS_ATOMIC_DECB(var) \
    __asm__ __volatile__ ("decb %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define AROS_ATOMIC_DECW(var) \
    __asm__ __volatile__ ("decw %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define AROS_ATOMIC_DECL(var) \
    __asm__ __volatile__ ("decl %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")

#define AROS_ATOMIC_ANDB(var,mask) \
    __asm__ __volatile__ ("andb %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")
#define AROS_ATOMIC_ANDW(var,mask) \
    __asm__ __volatile__ ("andw %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")
#define AROS_ATOMIC_ANDL(var,mask) \
    __asm__ __volatile__ ("andl %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")


#define AROS_ATOMIC_ORB(var,mask) \
    __asm__ __volatile__ ("orb %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")
#define AROS_ATOMIC_ORW(var,mask) \
    __asm__ __volatile__ ("orw %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")
#define AROS_ATOMIC_ORL(var,mask) \
    __asm__ __volatile__ ("orl %0,%1" : : "r" ((mask)), "m" ((var)) : "memory", "cc")

#else

#define AROS_ATOMIC_INCB(var) do {Disable(); (var)++; Enable(); } while(0)
#define AROS_ATOMIC_INCW(var) do {Disable(); (var)++; Enable(); } while(0)
#define AROS_ATOMIC_INCL(var) do {Disable(); (var)++; Enable(); } while(0)

#define AROS_ATOMIC_DECB(var) do {Disable(); (var)--; Enable(); } while(0)
#define AROS_ATOMIC_DECW(var) do {Disable(); (var)--; Enable(); } while(0)
#define AROS_ATOMIC_DECL(var) do {Disable(); (var)--; Enable(); } while(0)

#define AROS_ATOMIC_ANDB(var,mask) do {Disable(); (var) &= (mask); Enable(); } while(0)
#define AROS_ATOMIC_ANDW(var,mask) do {Disable(); (var) &= (mask); Enable(); } while(0)
#define AROS_ATOMIC_ANDL(var,mask) do {Disable(); (var) &= (mask); Enable(); } while(0)

#define AROS_ATOMIC_ORB(var,mask) do {Disable(); (var) |= (mask); Enable(); } while(0)
#define AROS_ATOMIC_ORW(var,mask) do {Disable(); (var) |= (mask); Enable(); } while(0)
#define AROS_ATOMIC_ORL(var,mask) do {Disable(); (var) |= (mask); Enable(); } while(0)

#endif

#define __AROS_ATOMIC(__instr__, var)             \
do                                                \
{                                                 \
    struct atomic_size                            \
    {                                             \
        int unsupported[sizeof(var)>4 ? 1 : -1];  \
    };                                            \
                                                  \
    if (sizeof(var) == 1)                         \
        AROS_ATOMIC_ ## __instr__ ## B(var);      \
    else                                          \
    if (sizeof(var) == 2)                         \
        AROS_ATOMIC_ ## __instr__ ## W(var);      \
    else                                          \
    if (sizeof(var) == 4)                         \
        AROS_ATOMIC_ ## __instr__ ## L(var);      \
} while (0)

#define AROS_ATOMIC_INC(var) __AROS_ATOMIC(INC, (var))
#define AROS_ATOMIC_DEC(var) __AROS_ATOMIC(DEC, (var))
#define AROS_ATOMIC_AND(var) __AROS_ATOMIC(AND, (var))
#define AROS_ATOMIC_OR(var)  __AROS_ATOMIC(OR,  (var))

#endif /* AROS_ATOMIC_H */
