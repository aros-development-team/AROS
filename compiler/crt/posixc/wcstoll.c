/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function wcstoll.
*/

#include <wctype.h>
#include <limits.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

long long wcstoll(

/*  SYNOPSIS */
    const wchar_t *nptr,
    wchar_t **endptr,
    int base)

/*  FUNCTION
        Converts the initial part of wide string nptr to a long long integer,
        respecting the base (2 to 36) or auto-detect if base is 0.

    INPUTS
        nptr   - Wide string input.
        endptr - If not NULL, updated to point after last converted character.
        base   - Numeric base for conversion.

    RESULT
        Returns converted long long integer. On overflow, returns LLONG_MAX or
        LLONG_MIN and sets errno to ERANGE. Returns 0 if no conversion.

    NOTES
        Handles optional whitespace and sign. Base 0 auto-detects hex/octal/decimal.

    EXAMPLE

        wchar_t *end;
        long long val = wcstoll(L"123456789LLxyz", &end, 10);
        // val == 123456789LL, *end == L'x'

    BUGS

    SEE ALSO
        wcstol(), wcstoul(), wcstoull()

    INTERNALS

******************************************************************************/
{
    const wchar_t *p = nptr;
    long long result = 0;
    int sign = 1;
    int digit;
    int any = 0;

    // Skip leading whitespace
    while (iswspace(*p)) p++;

    // Optional sign
    if (*p == L'+') {
        p++;
    } else if (*p == L'-') {
        sign = -1;
        p++;
    }

    // Detect base if base==0
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

        // Check overflow:
        if (result > (LLONG_MAX - digit) / base) {
            errno = ERANGE;
            result = LLONG_MAX;
            if (sign < 0) result = LLONG_MIN;
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

    return sign * result;
}
