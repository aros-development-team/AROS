/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Posix function strtof_l().
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
float strtof_l(const char * restrict nptr, char ** restrict endptr, locale_t loc)
{
    /* Once-only notice: __sync test-and-set, so exactly one print even on
       SMP builds (no data race: the flag is only touched atomically).
       bug() is kprintf to the debug channel, never stdout/stderr, so
       libc++ callers parsing their own output are unaffected. */
    static int warned = 0;
    if (__sync_lock_test_and_set(&warned, 1) == 0)
        bug("strtof_l: locale argument ignored (struct __locale has no numeric data yet)\n");
    (void)loc;
    return strtof(nptr, endptr);
}
