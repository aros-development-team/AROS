/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function wcstoull.
*/

#include <wctype.h>
#include <limits.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

unsigned long long wcstoull(

/*  SYNOPSIS */
    const wchar_t *nptr,
    wchar_t **endptr,
    int base)

/*  FUNCTION
        Converts the initial part of wide string nptr to an unsigned long long
        integer according to the specified base or auto-detect if base is 0.

    INPUTS
        nptr   - Wide string input.
        endptr - If not NULL, set to the character after last consumed character.
        base   - Numeric base for conversion (0 or 2 to 36).

    RESULT
        Returns converted unsigned long long integer. On overflow, returns
        ULLONG_MAX and sets errno to ERANGE. Returns 0 if no conversion.

    NOTES
        Supports optional whitespace and plus sign. Base 0 auto-detects.

    EXAMPLE

        wchar_t *end;
        unsigned long long val = wcstoull(L"18446744073709551615xyz", &end, 10);
        // val == 18446744073709551615ULL, *end == L'x'

    BUGS

    SEE ALSO
        wcstol(), wcstoll(), wcstoul()

    INTERNALS

******************************************************************************/
{
    const wchar_t *p = nptr;
    unsigned long long result = 0;
    int digit;
    int any = 0;

    // Skip leading whitespace
    while (iswspace(*p)) p++;

    // Optional sign — plus only, minus results in ERANGE and returns 0 here
    if (*p == L'+') {
        p++;
    } else if (*p == L'-') {
        errno = ERANGE;
        if (endptr) *endptr = (wchar_t *)nptr;
        return 0ULL;
    }

    // Detect base if base == 0
    if (base == 0) {
        if (*p == L'0') {
            p++;
            if (*p == L'x' || *p == L'X') {
                base = 16;
                p++;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*p == L'0' && (p[1] == L'x' || p[1] == L'X')) {
            p += 2;
        }
    }

    // Parse digits
    while (*p) {
        wchar_t c = *p;
        if (iswdigit(c)) {
            digit = c - L'0';
        } else if (c >= L'a' && c <= L'z') {
            digit = c - L'a' + 10;
        } else if (c >= L'A' && c <= L'Z') {
            digit = c - L'A' + 10;
        } else {
            break;
        }

        if (digit >= base) break;

        // Check overflow for unsigned long long
        if (result > (ULLONG_MAX - digit) / base) {
            errno = ERANGE;
            result = ULLONG_MAX;
            any = 1;
            break;
        }

        result = result * base + digit;
        any = 1;
        p++;
    }

    if (endptr != NULL) {
        *endptr = (wchar_t *)(any ? p : nptr);
    }

    return result;
}
