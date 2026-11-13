/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function strcoll_l.
*/

#include <string.h>

/****************************************************************************
    NAME
        int strcoll_l(

    SYNOPSIS
        const char *s1, const char *s2, locale_t loc)

    FUNCTION
        Compare two null-terminated strings in unsigned-byte
        order. Comparison stops at the first differing byte or
        the terminating null byte.

    INPUTS
        s1 - First readable, null-terminated string.
        s2 - Second readable, null-terminated string.
        loc - Ignored; neither dereferenced nor retained.

        Neither string pointer may be NULL. The strings may
        overlap; their contents are not modified.

    RESULT
        A negative value if s1 precedes s2, zero if they are equal,
        or a positive value if s1 follows s2. The result is not
        limited to -1, 0 and 1. errno is unchanged.

    NOTES
        Include <string.h>. The comparison allocates no storage
        and does not retain either string.

    BUGS
        Locale-specific collation is not implemented. The supplied
        locale and current locale do not affect the result.
        Multibyte characters are compared as individual bytes;
        no linguistic ordering or normalization is performed.

    SEE ALSO
        strcmp(), strcoll(), strxfrm_l()

*****************************************************************************/
int strcoll_l(const char *s1, const char *s2, locale_t loc)
{
	return strcoll(s1, s2);
}