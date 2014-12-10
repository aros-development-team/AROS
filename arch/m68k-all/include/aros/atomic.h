/*
    Copyright © 1997-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Atomic access functions to be used by macros in atomic.h.
    Lang: english
*/

#include <proto/exec.h>

static inline void atomic_inc_l(LONG* p)
{
    asm volatile ("addq.l #1,%0" : "+m" (*p));
}

static inline void atomic_dec_l(LONG* p)
{
    asm volatile ("subq.l #1,%0" : "+m" (*p));
}

static inline void atomic_and_l(ULONG* p, ULONG mask)
{
    asm volatile ("and.l %1,%0" : "+m" (*p) : "id" (mask));
}

static inline void atomic_or_l(ULONG* p, ULONG mask)
{
    asm volatile ("or.l %1,%0" : "+m" (*p) : "id" (mask));
}

static inline void atomic_inc_b(BYTE* p)
{
    asm volatile ("addq.b #1,%0" : "+m" (*p));
}

static inline void atomic_dec_b(BYTE* p)
{
    asm volatile ("subq.b #1,%0" : "+m" (*p));
}

static inline void atomic_and_b(UBYTE* p, UBYTE mask)
{
    asm volatile ("and.b %1,%0" : "+m" (*p) : "id" (mask));
}

static inline void atomic_or_b(UBYTE* p, UBYTE mask)
{
    asm volatile ("or.b %1,%0" : "+m" (*p) : "id" (mask));
}

static inline void atomic_inc_w(WORD* p)
{
    asm volatile ("addq.w #1,%0" : "+m" (*p));
}

static inline void atomic_dec_w(WORD* p)
{
    asm volatile ("subq.w #1,%0" : "+m" (*p));
}

static inline void atomic_and_w(UWORD* p, UWORD mask)
{
    asm volatile ("and.w %1,%0" : "+m" (*p) : "id" (mask));
}

static inline void atomic_or_w(UWORD* p, UWORD mask)
{
    asm volatile ("or.w %1,%0" : "+m" (*p) : "id" (mask));
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
