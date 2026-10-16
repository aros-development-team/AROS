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
/****************************************************************************
    NAME
        struct lconv * localeconv_l(

    SYNOPSIS
        locale_t loc)

    FUNCTION
        Return the default numeric-formatting data when loc is
        NULL. Locale-object queries are not implemented correctly.

    INPUTS
        loc - Pass NULL. Non-NULL locale handles, including those
               returned by newlocale(), are unsupported by this
               implementation. Do not pass LC_GLOBAL_LOCALE.

    RESULT
        For NULL, the borrowed static structure returned by
        localeconv(): a decimal point of ".", empty separator,
        grouping and monetary strings, and CHAR_MAX for numeric
        monetary fields. Do not modify or free the structure or
        its strings. No storage is allocated for the result.

    NOTES
        Include <locale.h>. Every call emits a not-implemented
        diagnostic through kprintf(), including the NULL case.
        The function does not select or retain a locale.

    BUGS
        For non-NULL loc, the function assumes that the object
        starts with an embedded struct lconv. The actual locale
        objects use a different layout. The returned pointer is
        not a valid formatting-data result for those handles;
        reading through it can access unrelated or invalid data.
        This failure is not reported with a NULL result or errno.

    SEE ALSO
        localeconv(), newlocale()

*****************************************************************************/
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
