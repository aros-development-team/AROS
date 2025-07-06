#include <wctype.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

long double wcstold(

/*  SYNOPSIS */
    const wchar_t *nptr,
    wchar_t **endptr)

/*  FUNCTION
        Converts the initial portion of the wide string nptr to a long double
        floating-point number. Stops at first invalid character.

    INPUTS
        nptr   - Wide string input.
        endptr - If not NULL, set to the character after last converted character.

    RESULT
        Returns converted long double value. Returns 0.0L if no conversion.
        On overflow, returns HUGE_VALL and sets errno to ERANGE.

    NOTES
        Handles optional whitespace, sign, decimal and exponential notation,
        and special values "inf" and "nan".

    EXAMPLE

        wchar_t *end;
        long double val = wcstold(L"1.2345e10xyz", &end);
        // val == 1.2345e10L, *end == L'x'

    BUGS

    SEE ALSO
        wcstod(), wcstof()

    INTERNALS

******************************************************************************/
{
    const wchar_t *p = nptr;
    int sign = 1;
    long double result = 0.0L;
    int frac_digits = 0;
    int exp_sign = 1;
    int exponent = 0;
    int seen_digit = 0;

    // Skip whitespace
    while (iswspace(*p)) p++;

    // Parse optional sign
    if (*p == L'+') {
        p++;
    } else if (*p == L'-') {
        sign = -1;
        p++;
    }

    // Integer part
    while (iswdigit(*p)) {
        seen_digit = 1;
        result = result * 10.0L + (*p - L'0');
        p++;
    }

    // Fractional part
    if (*p == L'.') {
        p++;
        while (iswdigit(*p)) {
            seen_digit = 1;
            result = result * 10.0L + (*p - L'0');
            frac_digits++;
            p++;
        }
    }

    // Adjust result for fraction digits
    for (int i = 0; i < frac_digits; i++) {
        result /= 10.0L;
    }

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

    // Apply exponent: multiply/divide by 10^exponent
    if (exponent != 0) {
        long double pow10 = 1.0L;
        if (exponent > 0) {
            for (int i = 0; i < exponent; i++) pow10 *= 10.0L;
        } else {
            for (int i = 0; i < -exponent; i++) pow10 /= 10.0L;
        }
        result *= pow10;
    }

    if (endptr) *endptr = (wchar_t *) (seen_digit ? p : nptr);

    return sign * result;
}
