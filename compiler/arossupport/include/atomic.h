/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROS_ATOMIC_H
#define AROS_ATOMIC_H

#ifdef __i386__

#define AROS_ATOMIC_INCB(var) \
    __asm__ __volatile__ ("incb %0" : "=m" ((var)) : "m" ((var)) : "memory")
#define AROS_ATOMIC_INCW(var) \
    __asm__ __volatile__ ("incw %0" : "=m" ((var)) : "m" ((var)) : "memory")
#define AROS_ATOMIC_INCL(var) \
    __asm__ __volatile__ ("incl %0" : "=m" ((var)) : "m" ((var)) : "memory")

#define AROS_ATOMIC_DECB(var) \
    __asm__ __volatile__ ("decb %0" : "=m" ((var)) : "m" ((var)) : "memory")
#define AROS_ATOMIC_DECW(var) \
    __asm__ __volatile__ ("decw %0" : "=m" ((var)) : "m" ((var)) : "memory")
#define AROS_ATOMIC_DECL(var) \
    __asm__ __volatile__ ("decl %0" : "=m" ((var)) : "m" ((var)) : "memory")

#define AROS_ATOMIC_ANDB(var,mask) \
    __asm__ __volatile__ ("andb %0,%1" : : "r" ((mask)), "m" ((var)) : "memory")

#define AROS_ATOMIC_ANDW(var,mask) \
    __asm__ __volatile__ ("andw %0,%1" : : "r" ((mask)), "m" ((var)) : "memory")

#define AROS_ATOMIC_ANDL(var,mask) \
    __asm__ __volatile__ ("andl %0,%1" : : "r" ((mask)), "m" ((var)) : "memory")


#define AROS_ATOMIC_ORB(var,mask) \
    __asm__ __volatile__ ("orb %0,%1" : : "r" ((mask)), "m" ((var)) : "memory")

#define AROS_ATOMIC_ORW(var,mask) \
    __asm__ __volatile__ ("orw %0,%1" : : "r" ((mask)), "m" ((var)) : "memory")

#define AROS_ATOMIC_ORL(var,mask) \
    __asm__ __volatile__ ("orl %0,%1" : : "r" ((mask)), "m" ((var)) : "memory")
    
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

#endif /* AROS_ATOMIC_H */
