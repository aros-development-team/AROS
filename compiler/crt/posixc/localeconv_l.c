/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Posix uselocale.
*/

#include <locale.h>  // For struct lconv and locale_t

// Your locale structure, for reference (simplified)
struct __locale {
    // ... other members ...

    struct lconv locale_lconv;  // Embed or point to the lconv for this locale

    // Alternatively, you might have individual members for
    // decimal_point, thousands_sep, grouping, currency_symbol, etc.
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

    if (loc == NULL)
        return localeconv(); // fallback to global localeconv()

    // Return the embedded or cached lconv structure
    return &(loc->locale_lconv);
}
