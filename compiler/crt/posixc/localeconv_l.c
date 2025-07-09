/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Posix uselocale.
*/
#include <aros/debug.h>

#include <locale.h>

/*
   This is broken just now and subject to change
   Do not use, or expect it to work.
 */
struct __locale {
    struct lconv locale_lconv;
};

// Given your locale type alias
typedef struct __locale *locale_t;

// Example implementation:
struct lconv *localeconv_l(locale_t loc)
{
    static struct lconv fallback = {
        ".",        // decimal_point
        "",         // thousands_sep
        "\3",       // grouping
        "",         // int_curr_symbol
        "",         // currency_symbol
        "",         // mon_decimal_point
        "",         // mon_thousands_sep
        "",         // mon_grouping
        "",         // positive_sign
        (char)0,    // negative_sign
        (char)0xFF, // int_frac_digits
        (char)0xFF, // frac_digits
        (char)0xFF, // p_cs_precedes
        (char)0xFF, // p_sep_by_space
        (char)0xFF, // n_cs_precedes
        (char)0xFF, // n_sep_by_space
        "",         // p_sign_posn
        (char)0xFF  // n_sign_posn
    };

    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");

    if (loc == NULL)
        return localeconv(); // fallback to global localeconv()

    // Return the embedded or cached lconv structure
    return &(loc->locale_lconv);
}
