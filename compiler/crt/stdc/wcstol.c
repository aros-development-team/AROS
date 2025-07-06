#include <wctype.h>
#include <limits.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

long wcstol(

/*  SYNOPSIS */
    const wchar_t *nptr,
    wchar_t **endptr,
    int base)

/*  FUNCTION
        Converts the initial part of wide string nptr to a long integer value,
        using the specified base (2 to 36) or auto-detect if base is 0.

    INPUTS
        nptr   - Wide string to convert.
        endptr - If not NULL, set to character after last used in conversion.
        base   - Numeric base for conversion (0, or 2 to 36).

    RESULT
        Returns the converted long integer. On overflow, returns LONG_MAX or
        LONG_MIN and sets errno to ERANGE. Returns 0 if no conversion was done.

    NOTES
        Skips optional whitespace and supports optional '+' or '-' sign.
        Base 0 auto-detects hexadecimal (0x), octal (0), or decimal.

    EXAMPLE

        wchar_t *end;
        long val = wcstol(L"0x10abc", &end, 0);
        // val == 16, *end == L'a'

    BUGS

    SEE ALSO
        strtol(), wcstoll(), wcstoul(), wcstoull()

    INTERNALS

******************************************************************************/
{
    const wchar_t *p = nptr;
    long result = 0;
    int sign = 1;
    int digit;
    int any = 0; // flag if we parsed any digit

    // Skip whitespace
    while (iswspace(*p)) p++;

    // Parse optional sign
    if (*p == L'+') {
        p++;
    } else if (*p == L'-') {
        sign = -1;
        p++;
    }

    // Auto-detect base if 0
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

        if (digit >= base)
            break;

        // Check overflow (optional, here simple check)
        if (result > (LONG_MAX - digit) / base) {
            errno = ERANGE;
            result = LONG_MAX;
            if (sign < 0) result = LONG_MIN;
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
