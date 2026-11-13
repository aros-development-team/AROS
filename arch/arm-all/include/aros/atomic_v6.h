/*
 * atomic_v6.h
 *
 *  Created on: Oct 23, 2010
 *      Author: misc
 */

#define __AROS_ATOMIC_INC_L(var) \
do { \
   unsigned long temp; \
   int result; \
   __asm__ __volatile__("\n1: ldrex %0, [%3]; add %0, %0, #1; strex %1, %0, [%3]; teq %1, #0; bne 1b" \
		   	   	   	   	   :"=&r"(result), "=&r"(temp), "+Qo"(var) \
		   	   	   	   	   :"r"(&var) \
		   	   	   	   	   :"cc"); \
} while (0)

#define __AROS_ATOMIC_DEC_L(var) \
do { \
   unsigned long temp; \
   int result; \
   __asm__ __volatile__("\n1: ldrex %0, [%3]; sub %0, %0, #1; strex %1, %0, [%3]; teq %1, #0; bne 1b" \
		   	   	   	   	   :"=&r"(result), "=&r"(temp), "+Qo"(var) \
		   	   	   	   	   :"r"(&var) \
		   	   	   	   	   :"cc"); \
} while (0)

static inline void atomic_and_l(const ULONG *var, ULONG mask)
{
    unsigned long temp; int result;

    __asm__ __volatile__("\n1: ldrex %0, [%2]; and %0, %0, %3; strex %1, %0, [%2]; teq %1, #0; bne 1b"
                        :"=&r"(result), "=&r"(temp)
                        :"r"(var), "Ir"(mask)
                        :"cc");
}

static inline void atomic_or_l(const ULONG *var, ULONG mask)
{
    unsigned long temp; int result;

    __asm__ __volatile__("\n1: ldrex %0, [%2]; orr %0, %0, %3; strex %1, %0, [%2]; teq %1, #0; bne 1b"
                        :"=&r"(result), "=&r"(temp)
                        :"r"(var), "Ir"(mask)
                        :"cc");
}

static inline void atomic_inc_b(BYTE* p)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */
    unsigned long mask = (255L << shift);	     /* get mask for changed byte    */
    unsigned long incval = 0x01010101;
    unsigned long temp, result;

   __asm__ __volatile__(
	"1:	ldrex %0, [%4]\n"		/* Load the longword and reserve location */
	"	uadd8 %1, %0, %2\n"		/* Increment all 4 bytes */
	"	and %1, %1, %3\n"		/* Mask away unneeded bytes in the result */
	"	bic %0, %0, %3\n"		/* Now mask away modified byte in the original longword */
	"	orr %0, %0, %1\n"		/* Merge back results */
	"	strex %1, %0, [%4]\n"		/* Store */
	"	teq %1, #0\n"			/* Repeat if not atomic */
	"	bne 1b"
	:"=&r"(result), "=&r"(temp)
	:"r"(incval), "r"(mask), "r"(addr)
	:"cc");
}

static inline void atomic_dec_b(BYTE* p)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */
    unsigned long mask = (255L << shift);	     /* get mask for changed byte    */
    unsigned long decval = 0x01010101;
    unsigned long temp, result;

   __asm__ __volatile__(
	"1:	ldrex %0, [%4]\n"		/* Load the longword and reserve location */
	"	usub8 %1, %0, %2\n"		/* Decrement all 4 bytes */
	"	and %1, %1, %3\n"		/* Mask away unneeded bytes in the result */
	"	bic %0, %0, %3\n"		/* Now mask away modified byte in the original longword */
	"	orr %0, %0, %1\n"		/* Merge back results */
	"	strex %1, %0, [%4]\n"		/* Store */
	"	teq %1, #0\n"			/* Repeat if not atomic */
	"	bne 1b"
	:"=&r"(result), "=&r"(temp)
	:"r"(decval), "r"(mask), "r"(addr)
	:"cc");
}

static inline void atomic_inc_w(WORD* p)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */
    unsigned long mask = (65535L << shift);	     /* get mask for changed bytes   */
    unsigned long incval = 0x00010001;
    unsigned long temp, result;

   __asm__ __volatile__(
	"1:	ldrex %0, [%4]\n"		/* Load the longword and reserve location */
	"	uadd16 %1, %0, %2\n"		/* Increment both halves */
	"	and %1, %1, %3\n"		/* Mask away unneeded bytes in the result */
	"	bic %0, %0, %3\n"		/* Now mask away modified bytes in the original longword */
	"	orr %0, %0, %1\n"		/* Merge back results */
	"	strex %1, %0, [%4]\n"		/* Store */
	"	teq %1, #0\n"			/* Repeat if not atomic */
	"	bne 1b"
	:"=&r"(result), "=&r"(temp)
	:"r"(incval), "r"(mask), "r"(addr)
	:"cc");
}

static inline void atomic_dec_w(WORD* p)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */
    unsigned long mask = (65535L << shift);	     /* get mask for changed bytes   */
    unsigned long decval = 0x00010001;
    unsigned long temp, result;

   __asm__ __volatile__(
	"1:	ldrex %0, [%4]\n"		/* Load the longword and reserve location */
	"	usub16 %1, %0, %2\n"		/* Decrement both halves */
	"	and %1, %1, %3\n"		/* Mask away unneeded bytes in the result */
	"	bic %0, %0, %3\n"		/* Now mask away modified bytes in the original longword */
	"	orr %0, %0, %1\n"		/* Merge back results */
	"	strex %1, %0, [%4]\n"		/* Store */
	"	teq %1, #0\n"			/* Repeat if not atomic */
	"	bne 1b"
	:"=&r"(result), "=&r"(temp)
	:"r"(decval), "r"(mask), "r"(addr)
	:"cc");
}

static inline void atomic_or_b(UBYTE* p, ULONG mask)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */

    atomic_or_l(addr, mask << shift);
}

static inline void atomic_and_b(UBYTE* p, ULONG mask)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */

    atomic_and_l(addr, ~0UL & ((UBYTE)mask << shift));
}

static inline void atomic_or_w(UWORD* p, ULONG mask)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */

    atomic_or_l(addr, mask << shift);
}

static inline void atomic_and_w(UWORD* p, ULONG mask)
{
    CONST ULONG* addr  = (ULONG*)((ULONG)p & ~0x03); /* Longword-align the address   */
    UBYTE shift        = ((ULONG)p & 0x03) << 3;     /* get number of bits to shift  */

    atomic_and_l(addr, ~0UL & ((UWORD)mask << shift));
}

#define __AROS_ATOMIC_INC_B(var) atomic_inc_b((BYTE *) &(var))
#define __AROS_ATOMIC_DEC_B(var) atomic_dec_b((BYTE *) &(var))

#define __AROS_ATOMIC_INC_W(var) atomic_inc_w((WORD *) &(var))
#define __AROS_ATOMIC_DEC_W(var) atomic_dec_w((WORD *) &(var))

#define __AROS_ATOMIC_AND_B(var, mask) atomic_and_b((UBYTE *) &(var), (mask))
#define __AROS_ATOMIC_AND_W(var, mask) atomic_and_w((UWORD *) &(var), (mask))
#define __AROS_ATOMIC_AND_L(var, mask) atomic_and_l((ULONG *) &(var), (mask))

#define __AROS_ATOMIC_OR_B(var, mask) atomic_or_b((UBYTE *) &(var), (mask))
#define __AROS_ATOMIC_OR_W(var, mask) atomic_or_w((UWORD *) &(var), (mask))
#define __AROS_ATOMIC_OR_L(var, mask) atomic_or_l((ULONG *) &(var), (mask))
