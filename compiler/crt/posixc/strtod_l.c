/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Posix function strtod_l().
*/

#ifndef AROS_NOFPU

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>

/*****************************************************************************

    NAME
        strtod_l -- Locale-aware string to double conversion

    SYNOPSIS
        double strtod_l(const char *str, char **endptr, locale_t loc)

    FUNCTION
        Convert a string of digits into a double, respecting the specified locale.

    INPUTS
        str - The string to convert. Leading whitespace is ignored.
        endptr - If non-NULL, points to the character after the number.
        loc - Locale to use for classification and decimal point.

    RESULT
        The converted value. If no digits could be converted, returns 0
        and stores str in *endptr (if non-NULL).

    BUGS
        Does not handle NAN or INF at the moment.
        Exponent always uses 'e'/'E', regardless of locale.

******************************************************************************/

double strtod_l(const char *str, char **endptr, locale_t loc)
{
    double val = 0, precision;
    int exp = 0;
    char c = 0, c2 = 0;
    int digits = 0;
    struct lconv *lc = localeconv_l(loc);
    char decimal_point = lc ? lc->decimal_point[0] : '.';

    if (endptr)
        *endptr = (char *)str;

    while (isspace_l(*str, loc))
        str++;

    if (*str)
    {
        if (*str == '+' || *str == '-')
            c = *str++;

        while (isdigit_l(*str, loc))
        {
            digits++;
            val = val * 10 + (*str - '0');
            str++;
        }

        if ((*str == decimal_point) && ((digits > 0) || isdigit_l(*(str + 1), loc)))
        {
            str++;
            precision = 0.1;
            while (isdigit_l(*str, loc))
            {
                digits++;
                val += ((*str - '0') * precision);
                str++;
                precision *= 0.1;
            }
        }

        if ((digits > 0) && (tolower_l(*str, loc) == 'e'))
        {
            int edigits = 0;
            str++;

            if (*str == '+' || *str == '-')
                c2 = *str++;

            while (isdigit_l(*str, loc))
            {
                edigits++;
                exp = exp * 10 + (*str - '0');
                str++;
            }

            if (c2 == '-')
                exp = -exp;

            if (edigits == 0)
            {
                str--;
                if (c2 != 0) str--;
            }

            val *= pow(10, exp);
        }

        if (c == '-')
            val = -val;

        if ((digits == 0) && (c != 0))
            str--;
    }

    if (endptr && digits > 0)
        *endptr = (char *)str;

    return val;
}

#else

void strtod_l(const char *str, char **endptr, locale_t loc)
{
    return;
}

#endif /* AROS_NOFPU */