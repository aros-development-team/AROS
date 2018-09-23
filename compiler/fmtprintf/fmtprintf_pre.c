/*
    Copyright © 2018, The AROS Development Team. All rights reserved.
    $Id$

    Prelude for common code block to format a string like printf().
*/

#include <dos/bptr.h>

#ifndef BITSPERBYTE
#   define BITSPERBYTE 8
#endif

#if (__WORDSIZE == 64)
/* On 64-bit machines long and long long are the same, so we don't need separate processing for long long */
#undef AROS_HAVE_LONG_LONG
#endif

#define MININTSIZE (sizeof(unsigned long)*BITSPERBYTE/3+1)
#define MINPOINTSIZE (sizeof(void *)*BITSPERBYTE/4+1)
#if defined(FULL_SPECIFIERS)
#define MINFLOATSIZE (DBL_DIG+1) /* Why not 1 more - it's 97% reliable */
#define REQUIREDBUFFER (MININTSIZE>MINPOINTSIZE? \
                        (MININTSIZE>MINFLOATSIZE?MININTSIZE:MINFLOATSIZE): \
                        (MINPOINTSIZE>MINFLOATSIZE?MINPOINTSIZE:MINFLOATSIZE))
#else
#define REQUIREDBUFFER (MININTSIZE>MINPOINTSIZE ? (MININTSIZE) : (MINPOINTSIZE))
#endif
#define ALTERNATEFLAG 1  /* '#' is set */
#define ZEROPADFLAG   2  /* '0' is set */
#define LALIGNFLAG    4  /* '-' is set */
#define BLANKFLAG     8  /* ' ' is set */
#define SIGNFLAG      16 /* '+' is set */
#define LDBLFLAG      64 /* Processing a long double */

static size_t format_long(char *buffer, char type, int base, unsigned long v)
{
    size_t size = 0;
    char hex = 'a' - 10;
    unsigned char mask  = 0;
    unsigned char shift = 0;

    switch (type)
    {
    case 'X':
        hex = 'A' - 10;
    case 'x':
        shift   = 4;
        mask    = 0x0F;
        if (base == 10)
            base = 16;
        break;

    case 'o':
        shift   = 3;
        mask    = 0x07;
        if (base == 10)
            base = 8;
        break;

    default:	/* 'd' and 'u' */
        /* Use slow divide operations for decimal numbers */
        do
        {
            char c = v % base;

            *--buffer = c + '0';
            v /= base;
            size++;
        } while (v);

        return size;
    }

    /* Divisor is a power of 2, so use fast shifts for division */
    do
    {
        char c = v & mask;

        *--buffer = (c < 10) ? c + '0' : c + hex;
        v >>= shift;
        size++;
    } while (v);

    return size;
}

#ifdef AROS_HAVE_LONG_LONG

/*
 * This is the same as format_long(), but takes long long argument.
 * This is used to process long long values on 32-bit machines. 64-bit
 * operations are performed slower there, and may need to call libgcc routines.
 */
static size_t format_longlong(char *buffer, char type, int base, unsigned long long v)
{
    size_t size = 0;
    char hex = 'a' - 10;
    unsigned char mask  = 0;
    unsigned char shift = 0;

    switch (type)
    {
    case 'X':
        hex = 'A' - 10;

    case 'x':
        shift = 4;
        mask = 0x0F;
        if (base == 10)
            base = 16;
        break;

    case 'o':
        shift = 3;
        mask  = 0x07;
        if (base == 10)
            base = 8;
        break;

    default:
/*
 * FIXME: this is not compiled for $(GENDIR)/lib32/librom.a because this requires
 * __umoddi3() and __udivdi3() from 32-bit version of libgcc which is not supplied
 * with 64-bit AROS gcc.
 * Perhaps these routines needs to be implemented explicitly for the bootstrap. Or
 * this code needs to be rewritten without these division operations, implementing
 * decimal division explicitly.
 * As a consequence, %llu and %lld do not work in x86-64 bootstrap. Use hexadecimal
 * output or fix this.
 */
#ifndef STDC_LIB32
        do
        {
            char c = v % base;

            *--buffer = c + '0';
            v /= base;
            size++;
        } while (v);
#endif

        return size;
    }

    do
    {
        char c = v & mask;

        *--buffer = (c < 10) ? c + '0' : c + hex;
        v >>= shift;
        size++;
    } while (v);

    return size;
}

#endif
