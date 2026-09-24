/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Posix function strtoull_l().
*/
#include <aros/debug.h>

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>

/* AROS locales are C-locale-only objects (see newlocale.c): numeric
   conversion does not depend on any constructible locale here, so forward
   to the locale-independent implementation. */
unsigned long long strtoull_l(const char * restrict nptr, char ** restrict endptr, int base, locale_t loc)
{
    (void)loc;
    return strtoull(nptr, endptr, base);
}
