/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: POSIX freelocale(). Every object newlocale() hands out is a shared
          immutable locale (see newlocale.c), so there is nothing to free;
          the call is accepted for conformance and ignored.
*/
#include <locale.h>

void freelocale(locale_t loc)
{
    (void)loc;
}
