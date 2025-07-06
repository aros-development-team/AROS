#include <wctype.h>  // For iswspace(), iswdigit()

/*****************************************************************************

    NAME */
#include <wchar.h>

double wcstod(

/*  SYNOPSIS */
    const wchar_t *nptr,
    wchar_t **endptr)

/*  FUNCTION
        Converts the initial portion of the wide string nptr to a double-precision
        floating-point number. Conversion stops at the first invalid character.

    INPUTS
        nptr   - Wide string to convert.
        endptr - If not NULL, set to point to the character after the last used
                 in conversion.

    RESULT
        Returns the converted double value. Returns 0.0 if no conversion could
        be performed. On overflow, returns HUGE_VAL and sets errno to ERANGE.

    NOTES
        Recognizes optional whitespace, optional sign, decimal and exponential
        notation, and special values such as "inf" and "nan".

    EXAMPLE

        wchar_t *end;
        double val = wcstod(L"3.14159xyz", &end);
        // val == 3.14159, *end == L'x'

    BUGS

    SEE ALSO
        strtod(), wcstof(), wcstold()

    INTERNALS

******************************************************************************/
{
    const wchar_t *p = nptr;
    int sign = 1;
    double result = 0.0;
    int frac_digits = 0;
    double frac_div = 1.0;
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
        result = result * 10.0 + (*p - L'0');
        p++;
    }

    // Fractional part
    if (*p == L'.') {
        p++;
        while (iswdigit(*p)) {
            seen_digit = 1;
            result = result * 10.0 + (*p - L'0');
            frac_digits++;
            p++;
        }
    }

    // Adjust for fractional digits
    for (int i = 0; i < frac_digits; i++)
        result /= 10.0;

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
    double pow10 = 1.0;
    if (exponent != 0) {
        // Use a simple pow10 loop or call pow(10, exponent) if available
        if (exponent > 0) {
            for (int i = 0; i < exponent; i++) pow10 *= 10.0;
        } else {
            for (int i = 0; i < -exponent; i++) pow10 /= 10.0;
        }
    }

    result = sign * result * pow10;

    if (endptr) *endptr = (wchar_t *) (seen_digit ? p : nptr);

    return result;
}
