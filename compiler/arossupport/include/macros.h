#ifndef AROS_MACROS_H
#define AROS_MACROS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Miscellanous macros
    Lang: english
*/

#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif

/* Reverse the bits in a byte */
#define AROS_SWAP_BITS_BYTE_GENERIC(_b)        \
({                                             \
    UBYTE __aros_swap_tmp = _b,                \
          b = __aros_swap_tmp;                 \
                                               \
    b = ((b >> 1) & 0x55) | ((b << 1) & 0xaa); \
    b = ((b >> 2) & 0x33) | ((b << 2) & 0xcc); \
        ((b >> 4) & 0x0f) | ((b << 4) & 0xf0); \
})

/* Reverse the bits in a word */
#define AROS_SWAP_BITS_WORD_GENERIC(_w)            \
({                                                 \
    UWORD __aros_swap_tmp = _w,                    \
          w = __aros_swap_tmp;                     \
                                                   \
    w = ((w >> 1) & 0x5555) | ((w << 1) & 0xaaaa); \
    w = ((w >> 2) & 0x3333) | ((w << 2) & 0xcccc); \
    w = ((w >> 4) & 0x0f0f) | ((w << 4) & 0xf0f0); \
        ((w >> 8) & 0x00ff) | ((w << 8) & 0xff00); \
})

/* Reverse the bits in a long */
#define AROS_SWAP_BITS_LONG_GENERIC(_l)                          \
({                                                               \
    ULONG __aros_swap_tmp = _l, l = __aros_swap_tmp;             \
	                                                         \
    l = ((l >>  1) & 0x55555555UL) | ((l <<  1) & 0xaaaaaaaaUL); \
    l = ((l >>  2) & 0x33333333UL) | ((l <<  2) & 0xccccccccUL); \
    l = ((l >>  4) & 0x0f0f0f0fUL) | ((l <<  4) & 0xf0f0f0f0UL); \
    l = ((l >>  8) & 0x00ff00ffUL) | ((l <<  8) & 0xff00ff00UL); \
        ((l >> 16) & 0x0000ffffUL) | ((l << 16) & 0xffff0000UL); \
})

/* Reverse the bits in a quad */
#define AROS_SWAP_BITS_QUAD_GENERIC(_q)                                            \
({                                                                                 \
    UQUAD __aros_swap_tmp = _q, q = __aros_swap_tmp;                               \
	                                                                           \
    q = ((q >>  1) & 0x5555555555555555ULL) | ((q <<  1) & 0xaaaaaaaaaaaaaaaaULL); \
    q = ((q >>  2) & 0x3333333333333333ULL) | ((q <<  2) & 0xccccccccccccccccULL); \
    q = ((q >>  4) & 0x0f0f0f0f0f0f0f0fULL) | ((q <<  4) & 0xf0f0f0f0f0f0f0f0ULL); \
    q = ((q >>  8) & 0x00ff00ff00ff00ffULL) | ((q <<  8) & 0xff00ff00ff00ff00ULL); \
    q = ((q >> 16) & 0x0000ffff0000ffffULL) | ((q << 16) & 0xffff0000ffff0000ULL); \
        ((q >> 32) & 0x00000000ffffffffULL) | ((q << 32) & 0xffffffff00000000ULL); \
})

/* Reverse the bytes in a word */
#define AROS_SWAP_BYTES_WORD_GENERIC(_w)                \
({                                                      \
    const UWORD __aros_swap_tmp = _w,                   \
                w = __aros_swap_tmp;                    \
                                                        \
    (UWORD)(((w >> 8) & 0x00FF) | ((w << 8) & 0xFF00)); \
})

/* Reverse the bytes in a long */
#define AROS_SWAP_BYTES_LONG_GENERIC(_l)              \
({                                                    \
    const ULONG __aros_swap_tmp = _l,                 \
                l = __aros_swap_tmp;                  \
                                                      \
    ((ULONG)AROS_SWAP_BYTES_WORD(l & 0xFFFF) << 16) | \
    (AROS_SWAP_BYTES_WORD((l >> 16) & 0xFFFF));       \
})

/* Reverse the bytes in a quad */
#define AROS_SWAP_BYTES_QUAD_GENERIC(_q)                     \
({                                                           \
    const UQUAD __aros_swap_tmp = _q,                        \
                q = __aros_swap_tmp;                         \
                                                             \
    ((UQUAD)AROS_SWAP_BYTES_LONG(q & 0xFFFFFFFFULL) << 32) | \
    (AROS_SWAP_BYTES_LONG(q >> 32) & 0xFFFFFFFFULL);         \
})

/* Reverse the words in a long */
#define AROS_SWAP_WORDS_LONG_GENERIC(_l)                              \
({                                                                    \
    const ULONG __aros_swap_tmp = _l, l = __aros_swap_tmp;            \
                                                                      \
    (ULONG)(((l >> 16) & 0x0000FFFFUL) | ((l << 16) & 0xFFFF0000UL)); \
})

/* Reverse the words in a quad */
#define AROS_SWAP_WORDS_QUAD_GENERIC(_q)                     \
({                                                           \
    const UQUAD __aros_swap_tmp = _q, q = __aros_swap_tmp;   \
                                                             \
    ((UQUAD)AROS_SWAP_WORDS_LONG(q & 0xFFFFFFFFULL) << 32) | \
    (AROS_SWAP_WORDS_LONG(q >> 32) & 0xFFFFFFFFULL);         \
})

/* Reverse the longs in a quad */
#define AROS_SWAP_LONGS_QUAD_GENERIC(_q)                                        \
({                                                                              \
    const UQUAD __aros_swap_tmp = _q, q = __aros_swap_tmp;                      \
                                                                                \
    (UQUAD)(((q >> 32) & 0xFFFFFFFFULL) | ((q << 32) & 0xFFFFFFFF00000000ULL)); \
})

/* Use the CPU-specific definitions of the above macros, if they exist, but reuse
   the generic macros in case the given value is a compile-time constant, because
   the compiler will optimize things out for us.  */
#if !defined(AROS_SWAP_BITS_BYTE_CPU)
#   define AROS_SWAP_BITS_BYTE(b) AROS_SWAP_BITS_BYTE_GENERIC(b)
#elif defined(__GNUC__)
#   define AROS_SWAP_BITS_BYTE(b)             \
    (                                         \
	__builtin_constant_p(b)               \
	    ? AROS_SWAP_BITS_BYTE_GENERIC(b)  \
	    : AROS_SWAP_BITS_BYTE_CPU(b)      \
    )
#else
#   define AROS_SWAP_BITS_BYTE(b) AROS_SWAP_BITS_BYTE_CPU(b)
#endif

#if !defined(AROS_SWAP_BITS_WORD_CPU)
#   define AROS_SWAP_BITS_WORD(w) AROS_SWAP_BITS_WORD_GENERIC(w)
#elif defined(__GNUC__)
#   define AROS_SWAP_BITS_WORD(w)             \
    (                                         \
	__builtin_constant_p(w)               \
	    ? AROS_SWAP_BITS_WORD_GENERIC(w)  \
	    : AROS_SWAP_BITS_WORD_CPU(w)      \
    )
#else
#   define AROS_SWAP_BITS_WORD(w) AROS_SWAP_BITS_WORD_CPU(w)
#endif

#if !defined(AROS_SWAP_BITS_LONG_CPU)
#   define AROS_SWAP_BITS_LONG(l) AROS_SWAP_BITS_LONG_GENERIC(l)
#elif defined(__GNUC__)
#   define AROS_SWAP_BITS_LONG(l)             \
    (                                         \
	__builtin_constant_p(l)               \
	    ? AROS_SWAP_BITS_LONG_GENERIC(l)  \
	    : AROS_SWAP_BITS_LONG_CPU(l)      \
    )
#else
#   define AROS_SWAP_BITS_LONG(l) AROS_SWAP_BITS_LONG_CPU(l)
#endif

#if !defined(AROS_SWAP_BITS_QUAD_CPU)
#   define AROS_SWAP_BITS_QUAD(q) AROS_SWAP_BITS_QUAD_GENERIC(q)
#elif defined(__GNUC__)
#   define AROS_SWAP_BITS_QUAD(q)             \
    (                                         \
	__builtin_constant_p(q)               \
	    ? AROS_SWAP_BITS_QUAD_GENERIC(q)  \
	    : AROS_SWAP_BITS_QUAD_CPU(q)      \
    )
#else
#   define AROS_SWAP_BITS_QUAD(q) AROS_SWAP_BITS_QUAD_CPU(q)
#endif

/* Just for consistency... */
#define AROS_SWAP_BYTES_BYTE(b)  ((UBYTE)b)
#define AROS_SWAP_WORDS_WORD(w)  ((ULONG)l)
#define AROS_SWAP_LONGS_LONG(l)  ((UWORD)l)
#define AROS_SWAP_QUANDS_QUAD(q) ((UQUAD)q)

#if !defined(AROS_SWAP_BYTES_WORD_CPU)
#   define AROS_SWAP_BYTES_WORD(w) AROS_SWAP_BYTES_WORD_GENERIC(w)
#elif defined(__GNUC__)
#   define AROS_SWAP_BYTES_WORD(w)            \
    (                                         \
	__builtin_constant_p(w)               \
	    ? AROS_SWAP_BYTES_WORD_GENERIC(w) \
	    : AROS_SWAP_BYTES_WORD_CPU(w)     \
    )
#else
#   define AROS_SWAP_BYTES_WORD(w) AROS_SWAP_BYTES_WORD_CPU(w)
#endif

#if !defined(AROS_SWAP_BYTES_LONG_CPU)
#   define AROS_SWAP_BYTES_LONG(l) AROS_SWAP_BYTES_LONG_GENERIC(l)
#elif defined(__GNUC__)
#   define AROS_SWAP_BYTES_LONG(l)            \
    (                                         \
	__builtin_constant_p(l)               \
	    ? AROS_SWAP_BYTES_LONG_GENERIC(l) \
	    : AROS_SWAP_BYTES_LONG_CPU(l)     \
    )
#else
#   define AROS_SWAP_BYTES_LONG(l) AROS_SWAP_BYTES_LONG_CPU(l)
#endif

#if !defined(AROS_SWAP_WORDS_LONG_CPU)
#   define AROS_SWAP_WORDS_LONG(l) AROS_SWAP_WORDS_LONG_GENERIC(l)
#elif defined(__GNUC__)
#   define AROS_SWAP_WORDS_LONG(l)            \
    (                                         \
	__builtin_constant_p(l)               \
	    ? AROS_SWAP_WORDS_LONG_GENERIC(l) \
	    : AROS_SWAP_WORDS_LONG_CPU(l)     \
    )
#else
#   define AROS_SWAP_WORDS_LONG(l) AROS_SWAP_WORDS_LONG_CPU(l)
#endif

#if !defined(AROS_SWAP_WORDS_QUAD_CPU)
#   define AROS_SWAP_WORDS_QUAD(q) AROS_SWAP_WORDS_QUAD_GENERIC(q)
#elif defined(__GNUC__)
#   define AROS_SWAP_WORDS_QUAD(q)            \
    (                                         \
	__builtin_constant_p(q)               \
	    ? AROS_SWAP_WORDS_QUAD_GENERIC(q) \
	    : AROS_SWAP_WORDS_QUAD_CPU(q)     \
    )
#else
#   define AROS_SWAP_WORDS_QUAD(l) AROS_SWAP_WORDS_QUAD_CPU(l)
#endif

#if !defined(AROS_SWAP_LONGS_QUAD_CPU)
#   define AROS_SWAP_LONGS_QUAD(q) AROS_SWAP_LONGS_QUAD_GENERIC(q)
#elif defined(__GNUC__)
#   define AROS_SWAP_LONGS_QUAD(q)            \
    (                                         \
	__builtin_constant_p(q)               \
	    ? AROS_SWAP_LONGS_QUAD_GENERIC(q) \
	    : AROS_SWAP_LONGS_QUAD_CPU(q)     \
    )
#else
#   define AROS_SWAP_LONGS_QUAD(q) AROS_SWAP_LONGS_QUAD_CPU(q)
#endif

#if AROS_BIG_ENDIAN
#    define AROS_BE(type)
#    define AROS_LE(type) AROS_SWAP_BYTES_ ## type
#else
#    define AROS_BE(type) AROS_SWAP_BYTES_ ## type
#    define AROS_LE(type)
#endif

/* Convert a word, long or quad to big endian and vice versa on the current hardware */
#define AROS_WORD2BE(w) AROS_BE(WORD)(w)
#define AROS_LONG2BE(l) AROS_BE(LONG)(l)
#define AROS_QUAD2BE(q) AROS_BE(QUAD)(q)
#define AROS_BE2WORD(w) AROS_BE(WORD)(w)
#define AROS_BE2LONG(l) AROS_BE(LONG)(l)
#define AROS_BE2QUAD(q) AROS_BE(QUAD)(q)

/* Convert a word, long or quad to little endian and vice versa on the current hardware */
#define AROS_WORD2LE(w) AROS_LE(WORD)(w)
#define AROS_LONG2LE(l) AROS_LE(LONG)(l)
#define AROS_QUAD2LE(q) AROS_LE(QUAD)(q)
#define AROS_LE2WORD(w) AROS_LE(WORD)(w)
#define AROS_LE2LONG(l) AROS_LE(LONG)(l)
#define AROS_LE2QUAD(q) AROS_LE(QUAD)(q)

/* Return the least set bit, ie. 0xFF00 will return 0x0100 */
#ifndef AROS_LEAST_BIT
#   define AROS_LEAST_BIT(l)    ((l) & -(l))
#endif

/* Check if an int is a power of two */
#ifndef AROS_IS_POWER_OF_2
#   define AROS_IS_POWER_OF_2(l)    (((l) & -(l)) == (l))
#endif

/* Round down <x> to a multiple of <r>. <r> must be a power of two */
#ifndef AROS_ROUNDDOWN2
#   define AROS_ROUNDDOWN2(x,r) ((x) & ~((r) - 1))
#endif

/* Round up <x> to a multiple of <r>. <r> must be a power of two */
#ifndef AROS_ROUNDUP2
#   define AROS_ROUNDUP2(x,r) (((x) + ((r) - 1)) &  ~((r) - 1))
#endif

/* Return the number of the least set bit, ie. 0xFF00 will return 8 */
#ifndef AROS_LEAST_BIT_POS
#   define AROS_LEAST_BIT_POS(l) \
    (((l) & 0x0000FFFFUL) ? \
	((l) & 0x000000FFUL) ? \
	    ((l) & 0x0000000FUL) ? \
		((l) & 0x03UL) ? \
		    ((l) & 0x01UL) ? 0 : 1 \
		:   ((l) & 0x04UL) ? 2 : 3 \
	    :	((l) & 0x30UL) ? \
		    ((l) & 0x10UL) ? 4 : 5 \
		:   ((l) & 0x40UL) ? 6 : 7 \
	:   ((l) & 0x00000F00UL) ? \
		((l) & 0x0300UL) ? \
		    ((l) & 0x0100UL) ?  8 :  9 \
		:   ((l) & 0x0400UL) ? 10 : 11 \
	    :	((l) & 0x3000UL) ? \
		    ((l) & 0x1000UL) ? 12 : 13 \
		:   ((l) & 0x4000UL) ? 14 : 15 \
    :	((l) & 0x00FF0000UL) ? \
	    ((l) & 0x000F0000UL) ? \
		((l) & 0x030000UL) ? \
		    ((l) & 0x010000UL) ? 16 : 17 \
		:   ((l) & 0x040000UL) ? 18 : 19 \
	    :	((l) & 0x300000UL) ? \
		    ((l) & 0x100000UL) ? 20 : 21 \
		:   ((l) & 0x400000UL) ? 22 : 23 \
	:   ((l) & 0x0F00000000UL) ? \
		((l) & 0x03000000UL) ? \
		    ((l) & 0x01000000UL) ? 24 : 25 \
		:   ((l) & 0x04000000UL) ? 26 : 27 \
	    :	((l) & 0x30000000UL) ? \
		    ((l) & 0x10000000UL) ? 28 : 29 \
		:   ((l) & 0x40000000UL) ? 30 : (l) ? 31 : -1)
#endif /* AROS_LEAST_BIT_POS */

/* Swap two integer variables */
#ifndef AROS_SWAP
#   define AROS_SWAP(x,y)       (x) ^= (y) ^= (x) ^= (y)
#endif

/* Build an 'ID' as used by iffparse.library and some other libraries as well. */
#define AROS_MAKE_ID(a,b,c,d) (((ULONG) (a)<<24) | ((ULONG) (b)<<16) | \
                               ((ULONG) (c)<<8)  | ((ULONG) (d)))

#endif /* AROS_MACROS_H */
