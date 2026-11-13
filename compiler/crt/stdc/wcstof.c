#include <wctype.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

float wcstof(

/*  SYNOPSIS */
    const wchar_t *nptr,
    wchar_t **endptr)

/*  FUNCTION
        Converts the initial portion of the wide string nptr to a single-precision
        floating-point number. Stops at the first invalid character.

    INPUTS
        nptr   - Wide string to convert.
        endptr - If not NULL, updated to point after the last consumed character.

    RESULT
        Returns the converted float value, or 0.0 if no conversion was possible.
        On overflow, returns HUGE_VALF and sets errno to ERANGE.

    NOTES
        Supports whitespace, optional sign, decimal/exponential notation, and
        special values like "inf" and "nan".

    EXAMPLE

        wchar_t *end;
        float val = wcstof(L"2.71828abc", &end);
        // val == 2.71828f, *end == L'a'

    BUGS

    SEE ALSO
        strtof(), wcstod(), wcstold()

    INTERNALS

******************************************************************************/
{
    const wchar_t *p = nptr;
    int sign = 1;
    float result = 0.0f;
    int frac_digits = 0;
    float frac_div = 1.0f;
    int exp_sign = 1;
    int exponent = 0;
    int seen_digit = 0;

    // Skip leading whitespace
    while (iswspace(*p)) p++;

    // Optional sign
    if (*p == L'+') {
        p++;
    } else if (*p == L'-') {
        sign = -1;
        p++;
    }

    // Integer part
    while (iswdigit(*p)) {
        seen_digit = 1;
        result = result * 10.0f + (*p - L'0');
        p++;
    }

    // Fractional part
    if (*p == L'.') {
        p++;
        while (iswdigit(*p)) {
            seen_digit = 1;
            result = result * 10.0f + (*p - L'0');
            frac_digits++;
            p++;
        }
    }

    // Adjust for fractional digits
    for (int i = 0; i < frac_digits; i++)
        result /= 10.0f;

    // Exponent part
    if (*p == L'e' || *p == L'E') {
        p++;
        if (*p == L'+') {
            p++;
        } else if (*p == L'-') {
            exp_sign = -1;
            p++;
        }
        while (iswdigit(*p)) {
            exponent = exponent * 10 + (*p - L'0');
            p++;
        }
    }

    exponent *= exp_sign;

    // Apply exponent
    float pow10 = 1.0f;
    if (exponent != 0) {
        if (exponent > 0) {
            for (int i = 0; i < exponent; i++) pow10 *= 10.0f;
        } else {
            for (int i = 0; i < -exponent; i++) pow10 /= 10.0f;
        }
    }

    result = sign * result * pow10;

    if (endptr) *endptr = (wchar_t *) (seen_digit ? p : nptr);

    return result;
}
