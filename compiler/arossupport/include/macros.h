#ifndef AROS_MACROS_H
#define AROS_MACROS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Work on 64bit data types
    Lang: english
*/

#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif

/* Convert a word or long to big endian and vice versa on the current hardware */
#if AROS_BIG_ENDIAN
#   define AROS_WORD2BE(w)     (w)
#   define AROS_LONG2BE(l)     (l)
#   define AROS_BE2WORD(w)     (w)
#   define AROS_BE2LONG(l)     (l)
#else
#   define AROS_WORD2BE(w)     ((((w) >> 8) & 0x00FF) | (((w) & 0x00FF) << 8))
#   define AROS_LONG2BE(l)     \
	(                                  \
	    ((((unsigned long)(l)) >> 24) & 0x000000FFUL) | \
	    ((((unsigned long)(l)) >>  8) & 0x0000FF00UL) | \
	    ((((unsigned long)(l)) <<  8) & 0x00FF0000UL) | \
	    ((((unsigned long)(l)) << 24) & 0xFF000000UL)   \
	)
#   define AROS_BE2WORD(w)     AROS_WORD2BE(w)
#   define AROS_BE2LONG(l)     AROS_LONG2BE(l)
#endif

/* Convert a word or long to little endian and vice versa on the current hardware */
#if AROS_BIG_ENDIAN
#   define AROS_WORD2LE(w)     ((((w) >> 8) & 0x00FF) | (((w) & 0x00FF) << 8))
#   define AROS_LONG2LE(l)     \
	(                                  \
	    ((((unsigned long)(l)) >> 24) & 0x000000FFUL) | \
	    ((((unsigned long)(l)) >>  8) & 0x0000FF00UL) | \
	    ((((unsigned long)(l)) <<  8) & 0x00FF0000UL) | \
	    ((((unsigned long)(l)) << 24) & 0xFF000000UL)   \
	)
#   define AROS_LE2WORD(w)     AROS_WORD2LE(w)
#   define AROS_LE2LONG(l)     AROS_LONG2LE(l)
#else
#    define AROS_WORD2LE(w)    (w)
#    define AROS_LONG2LE(l)    (l)
#    define AROS_LE2WORD(w)    (w)
#    define AROS_LE2LONG(l)    (l)
#endif

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

/* Swap to integer variables */
#ifndef AROS_SWAP
#   define AROS_SWAP(x,y)       (x) ^= (y) ^= (x) ^= (y)
#endif

/* Reverse the bits in a word */
#ifndef AROS_SWAP_BITS_WORD
#   define AROS_SWAP_BITS_WORD(w) \
	((w) = ((w) = ((((w) >> 1) & 0x5555) | (((w) << 1) & 0xaaaa)), \
		(w) = ((((w) >> 2) & 0x3333) | (((w) << 2) & 0xcccc)), \
		(w) = ((((w) >> 4) & 0x0f0f) | (((w) << 4) & 0xf0f0)), \
		(w) = ((((w) >> 8) & 0x00ff) | (((w) << 8) & 0xff00))) \
	)
#endif

/* Reverse the bits in a long */
#ifndef AROS_SWAP_BITS_LONG
#   define AROS_SWAP_BITS_LONG(l) \
	((l) = ((l) = ((((l) >>  1) & 0x55555555UL) | (((l) <<  1) & 0xaaaaaaaaUL)), \
		(l) = ((((l) >>  2) & 0x33333333UL) | (((l) <<  2) & 0xccccccccUL)), \
		(l) = ((((l) >>  4) & 0x0f0f0f0fUL) | (((l) <<  4) & 0xf0f0f0f0UL)), \
		(l) = ((((l) >>  8) & 0x00ff00ffUL) | (((l) <<  8) & 0xff00ff00UL)), \
		(l) = ((((l) >> 16) & 0x0000ffffUL) | (((l) << 16) & 0xffff0000UL))) \
	)
#endif

#endif /* AROS_MACROS_H */
