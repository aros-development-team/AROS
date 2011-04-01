/*
    Copyright © 1997-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Atomic access functions to be used by macros in atomic.h.
    Lang: english
*/

#include <proto/exec.h>

static inline void atomic_inc_l(LONG* p)
{
    asm volatile ("addql #1,%0" : "+m" (*p));
}

static inline void atomic_dec_l(LONG* p)
{
    asm volatile ("subql #1,%0" : "+m" (*p));
}

static inline void atomic_and_l(ULONG* p, ULONG mask)
{
    asm volatile ("andl %1,%0" : "+m" (*p) : "id" (mask));
}

static inline void atomic_or_l(ULONG* p, ULONG mask)
{
    asm volatile ("orl %1,%0" : "+m" (*p) : "id" (mask));
}

static inline void atomic_inc_b(BYTE* p)
{
    asm volatile ("addqb #1,%0" : "+m" (*p));
}

static inline void atomic_dec_b(BYTE* p)
{
    asm volatile ("subqb #1,%0" : "+m" (*p));
}

static inline void atomic_and_b(UBYTE* p, UBYTE mask)
{
    asm volatile ("andb %1,%0" : "+m" (*p) : "id" (mask));
}

static inline void atomic_or_b(UBYTE* p, UBYTE mask)
{
    asm volatile ("orb %1,%0" : "+m" (*p) : "id" (mask));
}

static inline void atomic_inc_w(WORD* p)
{
    asm volatile ("addqw #1,%0" : "+m" (*p));
}

static inline void atomic_dec_w(WORD* p)
{
    asm volatile ("subqw #1,%0" : "+m" (*p));
}

static inline void atomic_and_w(UWORD* p, UWORD mask)
{
    asm volatile ("andw %1,%0" : "+m" (*p) : "id" (mask));
}

static inline void atomic_or_w(UWORD* p, UWORD mask)
{
    asm volatile ("orw %1,%0" : "+m" (*p) : "id" (mask));
}

#define __AROS_ATOMIC_INC_B(var) atomic_inc_b((BYTE *) &(var))
#define __AROS_ATOMIC_DEC_B(var) atomic_dec_b((BYTE *) &(var))

#define __AROS_ATOMIC_INC_W(var) atomic_inc_w((WORD *) &(var))
#define __AROS_ATOMIC_DEC_W(var) atomic_dec_w((WORD *) &(var))

#define __AROS_ATOMIC_INC_L(var) atomic_inc_l((LONG *) &(var))
#define __AROS_ATOMIC_DEC_L(var) atomic_dec_l((LONG *) &(var))

#define __AROS_ATOMIC_AND_B(var, mask) atomic_and_b((UBYTE *) &(var), (UBYTE)(mask))
#define __AROS_ATOMIC_AND_W(var, mask) atomic_and_w((UWORD *) &(var), (UWORD)(mask))
#define __AROS_ATOMIC_AND_L(var, mask) atomic_and_l((ULONG *) &(var), (ULONG)(mask))

#define __AROS_ATOMIC_OR_B(var, mask) atomic_or_b((UBYTE *) &(var), (UBYTE)(mask))
#define __AROS_ATOMIC_OR_W(var, mask) atomic_or_w((UWORD *) &(var), (UWORD)(mask))
#define __AROS_ATOMIC_OR_L(var, mask) atomic_or_l((ULONG *) &(var), (ULONG)(mask))
