/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Atomic operations for ARMv3-5
    Lang: english
*/

#define __AROS_ATOMIC_INC_L(var)                                            \
do {                                                                        \
   unsigned int old, temp, result;                                          \
   __asm__ __volatile__(                                                    \
       "	ldr	%1, [%4]\n"                                         \
       "1:	mov	%2, %1\n"                                           \
       "	add	%0, %1, #1\n"                                       \
       "	swp	%1, %0, [%4]\n"                                     \
       "	teq	%1, %2\n"                                           \
       "	bne	1b"                                                 \
       :"=&r"(result), "=&r"(old), "=&r"(temp), "+Qo"(var):"r"(&var):"cc"); \
} while (0)

#define __AROS_ATOMIC_DEC_L(var)                                            \
do {                                                                        \
   unsigned int old, temp, result;                                          \
   __asm__ __volatile__(                                                    \
       "	ldr	%1, [%4]\n"                                         \
       "1:	mov	%2, %1\n"                                           \
       "	sub	%0, %1, #1\n"                                       \
       "	swp	%1, %0, [%4]\n"                                     \
       "	teq	%1, %2\n"                                           \
       "	bne	1b"                                                 \
       :"=&r"(result), "=&r"(old), "=&r"(temp), "+Qo"(var):"r"(&var):"cc"); \
} while (0)

static inline void atomic_and_l(const ULONG *var, ULONG mask)
{
    unsigned int old, temp, result;

   __asm__ __volatile__(
       "	ldr	%1, [%3]\n"
       "1:	mov	%2, %1\n"
       "	and	%0, %1, %4\n"
       "	swp	%1, %0, [%3]\n"
       "	teq	%1, %2\n"
       "	bne	1b"
       :"=&r"(result), "=&r"(old), "=&r"(temp):"r"(var), "Ir"(mask):"cc");
}

static inline void atomic_or_l(const ULONG *var, ULONG mask)
{
    unsigned int old, temp, result;

   __asm__ __volatile__(
       "	ldr	%1, [%3]\n"
       "1:	mov	%2, %1\n"
       "	orr	%0, %1, %4\n"
       "	swp	%1, %0, [%3]\n"
       "	teq	%1, %2\n"
       "	bne	1b"
       :"=&r"(result), "=&r"(old), "=&r"(temp):"r"(var), "Ir"(mask):"cc");
}

#define __AROS_ATOMIC_INC_B(var)                                            \
do {                                                                        \
   unsigned int old, temp, result;                                          \
   __asm__ __volatile__(                                                    \
       "	ldrb	%1, [%4]\n"                                         \
       "1:	mov	%2, %1\n"                                           \
       "	add	%0, %1, #1\n"                                       \
       "	swpb	%1, %0, [%4]\n"                                     \
       "	teq	%1, %2\n"                                           \
       "	bne	1b"                                                 \
       :"=&r"(result), "=&r"(old), "=&r"(temp), "+Qo"(var):"r"(&var):"cc"); \
} while (0)

#define __AROS_ATOMIC_DEC_B(var)                                            \
do {                                                                        \
   unsigned int old, temp, result;                                          \
   __asm__ __volatile__(                                                    \
       "	ldrb	%1, [%4]\n"                                         \
       "1:	mov	%2, %1\n"                                           \
       "	sub	%0, %1, #1\n"                                       \
       "	swpb	%1, %0, [%4]\n"                                     \
       "	teq	%1, %2\n"                                           \
       "	bne 1b"                                                     \
       :"=&r"(result), "=&r"(old), "=&r"(temp), "+Qo"(var):"r"(&var):"cc"); \
} while (0)

#define __AROS_ATOMIC_AND_B(var, mask)                                      \
do {                                                                        \
   unsigned int old, temp, result;                                          \
   __asm__ __volatile__(                                                    \
       "	ldrb	%1, [%4]\n"                                         \
       "1:	mov	%2, %1\n"                                           \
       "	and	%0, %1, %5\n"                                       \
       "	swpb	%1, %0, [%4]\n"                                     \
       "	teq	%1, %2\n"                                           \
       "	bne	1b"                                                 \
       :"=&r"(result), "=&r"(old), "=&r"(temp), "+Qo"(var):"r"(&var), "r"(mask):"cc"); \
} while (0)

#define __AROS_ATOMIC_OR_B(var, mask)                                       \
do {                                                                        \
   unsigned int old, temp, result;                                          \
   __asm__ __volatile__(                                                    \
       "	ldrb	%1, [%4]\n"                                         \
       "1:	mov	%2, %1\n"                                           \
       "	orr	%0, %1, %5\n"                                       \
       "	swpb	%1, %0, [%4]\n"                                     \
       "	teq	%1, %2\n"                                           \
       "	bne	1b"                                                 \
       :"=&r"(result), "=&r"(old), "=&r"(temp), "+Qo"(var):"r"(&var), "r"(mask):"cc"); \
} while (0)

/* WORD operations are emulated via LONG ones */

static inline void atomic_inc_w(WORD* p)
{
    CONST ULONG* addr  = (ULONG*)p;                  /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */
    unsigned long mask = (65535L << shift);          /* get mask for changed bytes   */
    unsigned long incval = (1L << shift);
    unsigned long old, temp, result;

   __asm__ __volatile__(
        "	ldr	%1, [%5]\n"             /* Load the longword */
        "1:     add	%2, %1, %3\n"           /* Increment */
        "       and	%2, %2, %4\n"           /* Mask away unneeded bytes in the result */
        "       bic	%0, %1, %4\n"           /* Now mask away modified bytes in the original longword */
        "       orr	%0, %0, %2\n"           /* Merge back results */
        "	mov	%2, %1\n"		/* Back up original longword for comparison */
        "       swp	%1, %0, [%5]\n"         /* Store */
        "       teq	%1, %2\n"               /* Repeat if not atomic */
        "       bne	1b"
        :"=&r"(result), "=&r"(old), "=&r"(temp)
        :"r"(incval), "r"(mask), "r"(addr):"cc");
}

static inline void atomic_dec_w(WORD* p)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */
    unsigned long mask = (65535L << shift);          /* get mask for changed bytes   */
    unsigned long decval = (1L << shift);
    unsigned long old, temp, result;

   __asm__ __volatile__(
        "	ldr	%1, [%5]\n"             /* Load the longword */
        "1:     sub	%2, %1, %3\n"           /* Decrement */
        "       and	%2, %2, %4\n"           /* Mask away unneeded bytes in the result */
        "       bic	%0, %1, %4\n"           /* Now mask away modified bytes in the original longword */
        "       orr	%0, %0, %2\n"           /* Merge back results */
        "	mov	%2, %1\n"		/* Back up original longword for comparison */
        "       swp	%1, %0, [%5]\n"         /* Store */
        "       teq	%1, %2\n"               /* Repeat if not atomic */
        "       bne	1b"
        :"=&r"(result), "=&r"(old), "=&r"(temp)
        :"r"(decval), "r"(mask), "r"(addr):"cc");
}

static inline void atomic_or_w(UWORD* p, UWORD mask)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */

    atomic_or_l(addr, (ULONG)mask << shift);
}

static inline void atomic_and_w(UWORD* p, UWORD mask)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */

    atomic_and_l(addr, ~0UL & (mask << shift));
}

#define __AROS_ATOMIC_INC_W(var) atomic_inc_w((WORD *) &(var))
#define __AROS_ATOMIC_DEC_W(var) atomic_dec_w((WORD *) &(var))

#define __AROS_ATOMIC_AND_W(var, mask) atomic_and_w((UWORD *) &(var), (mask))
#define __AROS_ATOMIC_AND_L(var, mask) atomic_and_l((ULONG *) &(var), (mask))

#define __AROS_ATOMIC_OR_W(var, mask) atomic_or_w((UWORD *) &(var), (mask))
#define __AROS_ATOMIC_OR_L(var, mask) atomic_or_l((ULONG *) &(var), (mask))
