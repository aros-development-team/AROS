/*
    Copyright © 2006-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function localeconv().
*/

#include <locale.h>
#include <limits.h>

static const struct lconv _lconv =
{
    ".", /* decimal_point */
    "", /* thousands_sep */
    "", /* grouping */
    "", /* mon_decimal_point */
    "", /* mon_thousand_sep */
    "", /* mon_grouping */
    "", /* positive_sign */
    "", /* negative_sign */
    "", /* currency_symbol */
    CHAR_MAX, /* frac_digits */
    CHAR_MAX, /* p_cs_precedes */
    CHAR_MAX, /* n_cs_precedes */
    CHAR_MAX, /* p_sep_by_space */
    CHAR_MAX, /* n_sep_by_space */
    CHAR_MAX, /* p_sign_posn */
    CHAR_MAX, /* n_sign_pasn */
    "", /* int_curr_symbol */
    CHAR_MAX, /* int_frac_digits */
    CHAR_MAX, /* int_p_cs_precedes */
    CHAR_MAX, /* int_n_cs_precedes */
    CHAR_MAX, /* int_p_sep_by_space */
    CHAR_MAX, /* int_n_sep_by_space */
    CHAR_MAX, /* int_p_sign_posn */
    CHAR_MAX /* int_n_sign_posn */
};

/*****************************************************************************

    NAME */
#include <string.h>

	struct lconv *localeconv (

/*  SYNOPSIS */
        void)

/*  FUNCTION
        The localeconv function sets the components of an object with type
        struct lconv with values appropriate for the formatting of numeric
        quantities (monetary and otherwise) according to the rules of the
        current locale.

    INPUTS
        -

    RESULT
        The lconv struct

    NOTES
        stdc.library only support "C" locale so always the same data
        is returned.

    EXAMPLE

    BUGS

    SEE ALSO
        locale.h

    INTERNALS

******************************************************************************/
{
    return (struct lconv *)&_lconv;
}
