/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    C99 function strtod().
*/
#ifndef AROS_NOFPU
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <stdint.h>

extern const unsigned char * const __stdc_char_decimalpoint;

#define WORDS 128

struct big { int n; uint32_t w[WORDS]; };

static void big_set(struct big *b, uint32_t v) { b->n = v ? 1 : 0; b->w[0] = v; }

static void big_mul_add(struct big *b, uint32_t m, uint32_t a)
{
    uint64_t carry = a;
    int i;

    for (i = 0; i < b->n; i++)
    {
        uint64_t t = (uint64_t)b->w[i] * m + carry;
        b->w[i] = (uint32_t)t;
        carry = t >> 32;
    }
    if (carry && b->n < WORDS)
        b->w[b->n++] = (uint32_t)carry;
}

static int big_bits(const struct big *b)
{
    int bits;
    uint32_t top;

    if (b->n == 0)
        return 0;
    top = b->w[b->n - 1];
    for (bits = 0; top; bits++)
        top >>= 1;
    return (b->n - 1) * 32 + bits;
}

static void big_shl(struct big *b, int s)
{
    int words = s / 32, bits = s % 32, i;

    if (b->n == 0)
        return;
    for (i = b->n - 1 + words + 1; i >= 0; i--)
    {
        uint64_t hi = (i - words >= 0 && i - words < b->n) ? b->w[i - words] : 0;
        uint64_t lo = (i - words - 1 >= 0 && i - words - 1 < b->n) ? b->w[i - words - 1] : 0;
        uint32_t v = bits ? (uint32_t)((hi << bits) | (lo >> (32 - bits))) : (uint32_t)hi;
        if (i < WORDS)
            b->w[i] = v;
    }
    b->n += words + 1;
    while (b->n > 0 && b->w[b->n - 1] == 0)
        b->n--;
}

static int big_cmp(const struct big *a, const struct big *b)
{
    int i;

    if (a->n != b->n)
        return a->n < b->n ? -1 : 1;
    for (i = a->n - 1; i >= 0; i--)
        if (a->w[i] != b->w[i])
            return a->w[i] < b->w[i] ? -1 : 1;
    return 0;
}

static void big_sub(struct big *a, const struct big *b)
{
    int64_t borrow = 0;
    int i;

    for (i = 0; i < a->n; i++)
    {
        int64_t t = (int64_t)a->w[i] - (i < b->n ? b->w[i] : 0) - borrow;
        borrow = t < 0;
        a->w[i] = (uint32_t)(t + (borrow ? ((int64_t)1 << 32) : 0));
    }
    while (a->n > 0 && a->w[a->n - 1] == 0)
        a->n--;
}

static void big_shr1(struct big *b)
{
    int i;

    for (i = 0; i < b->n; i++)
        b->w[i] = (b->w[i] >> 1) | (i + 1 < b->n ? b->w[i + 1] << 31 : 0);
    while (b->n > 0 && b->w[b->n - 1] == 0)
        b->n--;
}

/* q = num / den (q < 2^64), num becomes the remainder, den is unchanged on return. */
static uint64_t big_div(struct big *num, struct big *den)
{
    uint64_t q = 0;
    int shift = big_bits(num) - big_bits(den), i;

    if (shift < 0)
        return 0;
    big_shl(den, shift);
    for (i = 0; i <= shift; i++)
    {
        q <<= 1;
        if (big_cmp(num, den) >= 0)
        {
            big_sub(num, den);
            q |= 1;
        }
        if (i < shift)
            big_shr1(den);
    }
    return q;
}

static int digit_value(unsigned char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int word_matches(const char *s, const char *word)
{
    while (*word)
        if (tolower((unsigned char)*s++) != *word++) return 0;
    return 1;
}

/*****************************************************************************

    NAME */
#include <stdlib.h>
#include <math.h>

        double strtod (

/*  SYNOPSIS */
        const char * str,
        char      ** endptr)

/*  FUNCTION
        Convert the initial part of a string to a double.

    INPUTS
        str - The string to convert. Leading whitespace is skipped. The
                number may be prefixed by '+' or '-', and is either a
                decimal number with an optional 'e' exponent, a hexadecimal
                number introduced by "0x" with an optional 'p' binary
                exponent, "inf", "infinity" or "nan" with an optional
                parenthesised character sequence, all case-insensitive.
        endptr - If this is non-NULL, the address of the first character
                after the number is stored here.

    RESULT
        The value of the string, correctly rounded to the nearest double,
        ties to even. If no conversion can be performed, *endptr is set to
        str and 0 is returned. If the value overflows, HUGE_VAL with the
        sign of the number is returned and errno is set to ERANGE. If the
        value underflows, 0 or a denormal with the sign of the number is
        returned and errno is set to ERANGE.

    NOTES
        The decimal point is the one in __stdc_char_decimalpoint.

    EXAMPLE

    BUGS

    SEE ALSO
        atof(), strtof(), strtold(), strtol(), strtoul()

    INTERNALS
        Up to 800 significant digits are accumulated in a fixed-size big
        integer; any further digits only set a sticky bit, which is enough
        since every double rounding boundary has fewer than 800 significant
        decimal digits. The exponent is bounded before any scaling, so
        the 4096-bit workspace cannot overflow (at most 10^1125 for the
        divisor and 54 bits more for the dividend). Where double
        arithmetic is evaluated in double precision (FLT_EVAL_METHOD 0),
        a decimal number of at most 15 digits with an exponent within
        +/-22 is converted with one exact multiplication or division
        instead; on the x87 that operation would round twice.

******************************************************************************/
{
    const char *s = str;
    struct big num, den;
    int neg = 0, digits = 0, kept = 0, point = 0, tail = 0;
    int base = 10, step = 1, e2, nb, sticky, d;
    /* A string's length fits ptrdiff_t; its exponent may not. Counters are
       saturated well beyond the representable binary64 range. */
    long long scale = 0, exponent = 0;
    uint64_t q;

    if (endptr) *endptr = (char *)str;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '+' || *s == '-') neg = *s++ == '-';
    if (word_matches(s, "inf"))
    {
        s += 3;
        if (word_matches(s, "inity")) s += 5;
        if (endptr) *endptr = (char *)s;
        return neg ? -INFINITY : INFINITY;
    }
    if (word_matches(s, "nan"))
    {
        s += 3;
        if (*s == '(')
        {
            const char *p = s + 1;
            while (isalnum((unsigned char)*p) || *p == '_') p++;
            if (*p == ')') s = p + 1;
        }
        if (endptr) *endptr = (char *)s;
        return neg ? -NAN : NAN;
    }
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X') &&
        (isxdigit((unsigned char)s[2]) ||
         (s[2] == __stdc_char_decimalpoint[0] && isxdigit((unsigned char)s[3]))))
    {
        base = 16;
        step = 4;
        s += 2;
    }
    big_set(&num, 0);
    for (;;)
    {
        d = digit_value((unsigned char)*s);
        if (d >= 0 && d < base)
        {
            digits = 1;
            if (kept < 800)
            {
                if (d || kept)
                {
                    big_mul_add(&num, base, d);
                    kept++;
                }
                if (point) scale -= step;
            }
            else
            {
                tail |= d != 0;
                if (!point) scale += step;
            }
            s++;
        }
        else if (*s == __stdc_char_decimalpoint[0] && !point)
        {
            point = 1;
            s++;
        }
        else break;
    }
    if (!digits) return 0.0;
    if ((base == 10 && (*s == 'e' || *s == 'E')) ||
        (base == 16 && (*s == 'p' || *s == 'P')))
    {
        const char *p = s + 1;
        int eneg = 0;
        if (*p == '+' || *p == '-') eneg = *p++ == '-';
        if (isdigit((unsigned char)*p))
        {
            while (isdigit((unsigned char)*p))
            {
                if (exponent < 0x100000000LL)
                    exponent = exponent * 10 + (*p - '0');
                p++;
            }
            scale += eneg ? -exponent : exponent;
            s = p;
        }
    }
    if (endptr) *endptr = (char *)s;
    if (!num.n) return neg ? -0.0 : 0.0;
#if FLT_EVAL_METHOD == 0
    if (base == 10 && kept <= 15 && scale >= -22 && scale <= 22)
    {
        static const double p10[] = {
            1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11,
            1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
        };
        double m = (double)(((uint64_t)(num.n > 1 ? num.w[1] : 0) << 32) | num.w[0]);
        m = scale < 0 ? m / p10[(int)-scale] : m * p10[(int)scale];
        return neg ? -m : m;
    }
#endif

    /* Conservative early bounds leave all boundary rounding to exact math. */
    if ((base == 10 && scale + kept > 310) ||
        (base == 16 && scale + big_bits(&num) > 1025))
    {
        errno = ERANGE;
        return neg ? -HUGE_VAL : HUGE_VAL;
    }
    if ((base == 10 && scale + kept < -325) ||
        (base == 16 && scale + big_bits(&num) < -1075))
    {
        errno = ERANGE;
        return neg ? -0.0 : 0.0;
    }
    big_set(&den, 1);
    if (base == 10)
    {
        while (scale > 0) big_mul_add(&num, 10, 0), scale--;
        while (scale < 0) big_mul_add(&den, 10, 0), scale++;
    }
    /* For hex, scale is already the exact binary exponent. */
    e2 = (int)scale;
    nb = big_bits(&num) - big_bits(&den);
    if (nb < 54) big_shl(&num, 54 - nb), e2 -= 54 - nb;
    else if (nb > 54) big_shl(&den, nb - 54), e2 += nb - 54;
    q = big_div(&num, &den);
    sticky = tail;
    while (q >= ((uint64_t)1 << 54))
    {
        sticky |= (q & 1) != 0;
        q >>= 1;
        e2++;
    }
    while (q < ((uint64_t)1 << 53))
    {
        big_shl(&num, 1);
        q <<= 1;
        e2--;
        if (big_cmp(&num, &den) >= 0) big_sub(&num, &den), q |= 1;
    }
    sticky |= num.n != 0;
    {
        int extra = 1;
        uint64_t keep, half, rest;
        double r;
        if (e2 + 53 < -1022) extra += -1022 - (e2 + 53);
        if (extra > 60)
        {
            errno = ERANGE;
            return neg ? -0.0 : 0.0;
        }
        keep = q >> extra;
        half = (uint64_t)1 << (extra - 1);
        rest = q & ((half << 1) - 1);
        if (rest > half || (rest == half && (sticky || (keep & 1)))) keep++;
        r = ldexp((double)keep, e2 + extra);
        if (isinf(r) || (r < DBL_MIN && (rest || sticky))) errno = ERANGE;
        return neg ? -r : r;
    }
}
#else
void strtod(const char *str, char **endptr) { return; }
#endif
