/*
    Copyright (C) 2018-2026, The AROS Development Team. All rights reserved.

    Prelude for common code block to format a string like printf().
*/

#include <dos/bptr.h>

#ifndef BITSPERBYTE
#   define BITSPERBYTE 8
#endif

/* Output character width selector: 0 = narrow (char), 1 = wide (wchar_t).
   Defaulted here so includers that do not set it (e.g. the raw kprintf
   backend) keep the historic narrow behaviour. */
#ifndef FMTPRINTF_WIDE
#   define FMTPRINTF_WIDE 0
#endif

#if (__WORDSIZE == 64)
/* On 64-bit machines long and long long are the same, so we don't need separate processing for long long */
#undef AROS_HAVE_LONG_LONG
#endif

#define MININTSIZE (sizeof(unsigned long)*BITSPERBYTE/3+1)
#define MINPOINTSIZE (sizeof(void *)*BITSPERBYTE/4+1)
#if defined(FULL_SPECIFIERS)
/* Number of significant digits generated for floating point conversions.
   DBL_DIG+2 (== DBL_DECIMAL_DIG, 17) is the number of decimal digits needed to
   round-trip a double, so this yields the full inherent precision of a double.
   Going higher than this only emits digits the simple digit-by-digit algorithm
   can no longer compute reliably. */
#define MINFLOATSIZE (DBL_DIG+2)
#define REQUIREDBUFFER (MININTSIZE>MINPOINTSIZE? \
                        (MININTSIZE>MINFLOATSIZE?MININTSIZE:MINFLOATSIZE): \
                        (MINPOINTSIZE>MINFLOATSIZE?MINPOINTSIZE:MINFLOATSIZE))
#else
#define REQUIREDBUFFER 17 // Hexadecimal output of QUAD needs 16 characters
#endif
#define ALTERNATEFLAG 1  /* '#' is set */
#define ZEROPADFLAG   2  /* '0' is set */
#define LALIGNFLAG    4  /* '-' is set */
#define BLANKFLAG     8  /* ' ' is set */
#define SIGNFLAG      16 /* '+' is set */
#define LDBLFLAG      64 /* Processing a long double */

static size_t format_long(FMTPRINTF_TYPE *buffer, FMTPRINTF_TYPE type, int base, unsigned long v)
{
    size_t size = 0;
    FMTPRINTF_TYPE hex = FMTPRINTF_STR('a') - 10;
    FMTPRINTF_UTYPE mask  = 0;
    FMTPRINTF_UTYPE shift = 0;

    switch (type)
    {
    case FMTPRINTF_STR('X'):
        hex = FMTPRINTF_STR('A') - 10;
    case FMTPRINTF_STR('x'):
        shift   = 4;
        mask    = 0x0F;
        if (base == 10)
            base = 16;
        break;

    case FMTPRINTF_STR('o'):
        shift   = 3;
        mask    = 0x07;
        if (base == 10)
            base = 8;
        break;

    default:    /* 'd' and 'u' */
        /* Use slow divide operations for decimal numbers */
        do
        {
            FMTPRINTF_TYPE c = v % base;

            *--buffer = c + FMTPRINTF_STR('0');
            v /= base;
            size++;
        } while (v);

        return size;
    }

    /* Divisor is a power of 2, so use fast shifts for division */
    do
    {
        FMTPRINTF_TYPE c = v & mask;

        *--buffer = (c < 10) ? c + FMTPRINTF_STR('0') : c + hex;
        v >>= shift;
        size++;
    } while (v);

    return size;
}

#if defined(FULL_SPECIFIERS)

/* Base 10000; the longest exact value of a double has 767 significant decimal digits */
#define FLOATLIMBS 192

static int format_double_mul(UWORD *limb, int count, ULONG factor, ULONG carry)
{
    int i;

    for (i = 0; i < count; i++)
    {
        carry += limb[i] * factor;
        limb[i] = carry % 10000;
        carry /= 10000;
    }
    while (carry)
    {
        limb[count++] = carry % 10000;
        carry /= 10000;
    }

    return count;
}

/*
 * The decimal digits of a finite, non-negative double, rounded once from its exact
 * value to nearest, ties to even: 'count' significant digits, or with 'fixed' that
 * many after the decimal point, and never more than MINFLOATSIZE. Returns the decimal
 * exponent of the first digit. Integer arithmetic only, no math library.
 */
static __attribute__((noinline)) int format_double(FMTPRINTF_TYPE *buffer, size_t *size, double value,
                                                  int fixed, size_t count)
{
    static const UWORD pow10[] = { 1, 10, 100, 1000 };
    union { double d; UQUAD q; } bits;
    UWORD limb[FLOATLIMBS];
    UQUAD mant;
    int ex2, ex10, limbs = 0, digits, keep, pos, i, up = 0;

    bits.d = value;
    mant = bits.q & 0x000FFFFFFFFFFFFFULL;
    ex2 = (int)(bits.q >> 52) & 0x7FF;
    if (ex2)
        mant |= 0x0010000000000000ULL;
    else
        ex2 = 1;
    ex2 -= 1075;

    if (!mant)
    {
        buffer[0] = FMTPRINTF_STR('0');
        *size = 1;
        return 0;
    }

    while (!(mant & 1))
    {
        mant >>= 1;
        ex2++;
    }

    /* value = limb[] * 10^ex10 */
    for (i = 48; i >= 0; i -= 16)
        limbs = format_double_mul(limb, limbs, 65536, (ULONG)(mant >> i) & 0xFFFF);
    for (; ex2 >= 16; ex2 -= 16)
        limbs = format_double_mul(limb, limbs, 65536, 0);
    if (ex2 > 0)
    {
        limbs = format_double_mul(limb, limbs, 1UL << ex2, 0);
        ex2 = 0;
    }
    ex10 = ex2;
    for (; ex2 <= -8; ex2 += 8)
        limbs = format_double_mul(limb, limbs, 390625, 0);
    for (; ex2 < 0; ex2++)
        limbs = format_double_mul(limb, limbs, 5, 0);

    digits = limbs * 4;
    for (i = 3; i > 0 && limb[limbs - 1] < pow10[i]; i--)
        digits--;
    ex10 += digits - 1;

#define FLOATDIGIT(n) (limb[(n) / 4] / pow10[(n) % 4] % 10)

    if (count > 400)
        count = 400;
    keep = count;
    if (fixed)
        keep += 1 + ex10;
    if (keep > MINFLOATSIZE)
        keep = MINFLOATSIZE;

    if (keep >= 0 && keep < digits)
    {
        pos = digits - 1 - keep;
        i = FLOATDIGIT(pos);
        if (i > 5)
            up = 1;
        else if (i == 5)
        {
            up = (keep && (FLOATDIGIT(pos + 1) & 1)) || limb[pos / 4] % pow10[pos % 4];
            for (i = 0; i < pos / 4; i++)
                if (limb[i])
                    up = 1;
        }
    }
    else if (keep > digits)
        keep = digits;
    if (keep < 0)
        keep = 0;

    for (i = 0; i < keep; i++)
        buffer[i] = FMTPRINTF_STR('0') + FLOATDIGIT(digits - 1 - i);

#undef FLOATDIGIT

    if (up)
    {
        for (i = keep; i > 0 && buffer[i - 1] == FMTPRINTF_STR('9'); i--)
            buffer[i - 1] = FMTPRINTF_STR('0');
        if (i > 0)
            buffer[i - 1]++;
        else
        {
            buffer[0] = FMTPRINTF_STR('1');
            ex10++;
            if (!keep)
                keep = 1;
        }
    }
    else if (!keep)
    {
        buffer[0] = FMTPRINTF_STR('0');
        keep = 1;
        ex10 = 0;
    }

    *size = keep;
    return ex10;
}

#endif /* FULL_SPECIFIERS */

#ifdef AROS_HAVE_LONG_LONG

/*
 * This is the same as format_long(), but takes long long argument.
 * This is used to process long long values on 32-bit machines. 64-bit
 * operations are performed slower there, and may need to call libgcc routines.
 */
static size_t format_longlong(FMTPRINTF_TYPE *buffer, FMTPRINTF_TYPE type, int base, unsigned long long v)
{
    size_t size = 0;
    FMTPRINTF_TYPE hex = FMTPRINTF_STR('a') - 10;
    FMTPRINTF_UTYPE mask  = 0;
    FMTPRINTF_UTYPE shift = 0;

    switch (type)
    {
    case FMTPRINTF_STR('X'):
        hex = FMTPRINTF_STR('A') - 10;

    case FMTPRINTF_STR('x'):
        shift = 4;
        mask = 0x0F;
        if (base == 10)
            base = 16;
        break;

    case FMTPRINTF_STR('o'):
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
            FMTPRINTF_TYPE c = v % base;

            *--buffer = c + FMTPRINTF_STR('0');
            v /= base;
            size++;
        } while (v);
#endif

        return size;
    }

    do
    {
        FMTPRINTF_TYPE c = v & mask;

        *--buffer = (c < 10) ? c + FMTPRINTF_STR('0') : c + hex;
        v >>= shift;
        size++;
    } while (v);

    return size;
}

#endif
