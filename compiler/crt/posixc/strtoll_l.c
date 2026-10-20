/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Posix function strtoll_l().
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
        long long strtoll_l(

    SYNOPSIS
        const char * restrict nptr, char ** restrict endptr, int base, locale_t loc)

    FUNCTION
        Convert the initial integer in nptr using the specified
        base. Leading whitespace and one optional sign are
        accepted. Conversion stops at the first invalid digit.

    INPUTS
        nptr - Readable, null-terminated string; must not be NULL.
        endptr - Optional pointer to writable pointer storage,
                 separate from the input string. May be NULL.
        base - 0 to select octal for a leading zero, hexadecimal
               for 0x or 0X followed by a hexadecimal digit, and
               decimal otherwise; or an explicit base from 2 to 36.
               Base 16 also accepts the hexadecimal prefix.
        loc - Ignored; neither dereferenced nor retained.

    RESULT
        The converted value. If endptr is non-NULL, *endptr points
        to the first unconverted character. If no digits are
        converted, returns 0 and stores nptr in *endptr.

        An invalid base returns 0, sets errno to EINVAL and stores
        nptr in *endptr when supplied. Range errors set ERANGE;
        scanning continues through all remaining valid digits.
        Successful conversion does not clear errno.
        Values outside the signed range return LLONG_MIN or
        LLONG_MAX, according to the sign.

    NOTES
        Include <stdlib.h> with _GNU_SOURCE defined,
        _XOPEN_SOURCE >= 700, or _POSIX_C_SOURCE >= 200809L.
        Letters A through Z and a through z represent digits
        10 through 35. No binary 0b prefix is recognized.

        The first call through this function instance emits a
        debug diagnostic that the locale argument is ignored.
        Subsequent calls suppress that diagnostic.

    BUGS
        The supplied locale does not affect conversion.

    SEE ALSO
        strtoll(), strtoull_l()

*****************************************************************************/
long long strtoll_l(const char * restrict nptr, char ** restrict endptr, int base, locale_t loc)
{
    /* Once-only notice: __sync test-and-set, so exactly one print even on
       SMP builds (no data race: the flag is only touched atomically).
       bug() is kprintf to the debug channel, never stdout/stderr, so
       libc++ callers parsing their own output are unaffected. */
    static int warned = 0;
    if (__sync_lock_test_and_set(&warned, 1) == 0)
        bug("strtoll_l: locale argument ignored (struct __locale has no numeric data yet)\n");
    (void)loc;
    return strtoll(nptr, endptr, base);
}
