/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Posix function strtold_l().
*/
#include <aros/debug.h>

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>

/* FIXME (interim): the locale argument is ignored because struct __locale
   (compiler/crt/stdc/include/aros/types/locale_s.h) carries only ctype and
   case tables and has no numeric data yet. These functions must be reworked
   when locale objects gain numeric conventions (strtod() then consults the
   current locale, strto*_l() the object passed in). The forward keeps
   libc++ num_get working today. */
/****************************************************************************
    NAME
        long double strtold_l(

    SYNOPSIS
        const char * restrict nptr, char ** restrict endptr, locale_t loc)

    FUNCTION
        Convert the initial decimal floating-point number in
        nptr. Leading whitespace and an optional sign are
        accepted. The decimal separator is a period; e or E
        introduces an optional signed decimal exponent.

    INPUTS
        nptr - Readable, null-terminated string; must not be NULL.
        endptr - Optional pointer to writable pointer storage,
                 separate from the input string. May be NULL.
        loc - Ignored; neither dereferenced nor retained.

    RESULT
        The converted value. If endptr is supplied, *endptr points
        to the first unconverted character. If no conversion is
        performed, returns zero and stores nptr in *endptr.
        An exponent with no digits is left unconsumed.

        Case-insensitive inf, infinity and nan are accepted. A
        parenthesized NaN suffix is consumed only if it contains
        alphanumeric characters or underscores and has a closing
        parenthesis; an empty suffix is also accepted. The suffix
        does not select the NaN payload.

    NOTES
        Include <stdlib.h> with _GNU_SOURCE defined,
        _XOPEN_SOURCE >= 700, or _POSIX_C_SOURCE >= 200809L.
        The first call through this function instance emits a
        debug diagnostic that the locale argument is ignored.

    BUGS
        The supplied locale has no effect. Hexadecimal floating-
        point notation is unsupported: parsing 0x1p0 stops at x.

        Conversion uses double arithmetic without a correctly
        rounded conversion algorithm. Range-error reporting is
        incomplete; errno alone cannot establish a valid result.
        Large exponents can overflow an unchecked int accumulator.
        Intermediate overflow or underflow can corrupt a result
        even when the mathematical value is representable.
        The double result is widened to long double. This does not
        provide the additional range or precision of a wider
        long double format.

    SEE ALSO
        strtold(), strtod_l()

*****************************************************************************/
long double strtold_l(const char * restrict nptr, char ** restrict endptr, locale_t loc)
{
    /* Once-only notice: __sync test-and-set, so exactly one print even on
       SMP builds (no data race: the flag is only touched atomically).
       bug() is kprintf to the debug channel, never stdout/stderr, so
       libc++ callers parsing their own output are unaffected. */
    static int warned = 0;
    if (__sync_lock_test_and_set(&warned, 1) == 0)
        bug("strtold_l: locale argument ignored (struct __locale has no numeric data yet)\n");
    (void)loc;
    return strtold(nptr, endptr);
}
