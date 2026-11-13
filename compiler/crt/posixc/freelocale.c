/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: POSIX freelocale(). Every object newlocale() hands out is a shared
          immutable locale (see newlocale.c), so there is nothing to free;
          the call is accepted for conformance and ignored.
*/
#include <locale.h>

/****************************************************************************
    NAME
        void freelocale(

    SYNOPSIS
        locale_t loc)

    FUNCTION
        Accept a locale object for release. No storage is freed
        by this implementation.

    INPUTS
        loc - Locale object returned by newlocale(). The argument
              is not inspected or dereferenced.

    RESULT
        None. The function does not change errno.

    NOTES
        Include <locale.h>. newlocale() returns shared static
        objects with no caller-owned allocation. This call neither
        invalidates the object nor changes the active locale. Do
        not release these objects with free().

    SEE ALSO
        newlocale(), uselocale()

*****************************************************************************/
void freelocale(locale_t loc)
{
    (void)loc;
}
