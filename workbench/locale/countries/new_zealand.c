/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for New Zealand.
    Author: Stefan Haubenthal <polluks@sdf.lonestar.org>
*/

#include "country_locale.h"
#include <libraries/locale.h>

struct IntCountryPrefs new_zealandPrefs =
{
    {
        /* Reserved */
        { 0, 0, 0, 0 },

        /* Country code (licence plate number), telephone code, measuring system */
        MAKE_ID('N','Z',0,0), 64, MS_ISO,

        /* Date time format, date format, time format */
        "%A %B %e %Y %I:%M%p",
        "%A %B %e %Y",
        "%I:%M:%S%p",

        /* Short datetime, short date, short time formats */
        "%e/%m/%y %I:%M%p",
        "%e/%m/%y",
        "%I:%M%p",

        /* Decimal point, group separator, frac group separator */
        ".", " ", "",

        /* For grouping rules, see <libraries/locale.h> */

        /* Grouping, Frac Grouping */
        { 3 }, { 0 },

        /* Mon dec pt, mon group sep, mon frac group sep */
        ".", " ", "",

        /* Mon Grouping, Mon frac grouping */
        { 3 }, { 0 },

        /* Mon Frac digits, Mon IntFrac digits, then number of digits in
           the fractional part of the money value. Most countries that
           use dollars and cents, would have 2 for this value

           (As would many of those you don't).
        */
        2, 2,

        /* Currency symbol, Small currency symbol */
        "NZ$", "\242",

        /* Int CS, this is the ISO 4217 symbol, followed by the character to
           separate that symbol from the rest of the money. (\x00 for none).
        */
        "NZD",

        /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
        "", SS_SPACE, SP_PREC_ALL, CSP_PRECEDES,

        /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
        "", SS_NOSPACE, SP_PARENS, CSP_PRECEDES,

        /* Calendar type */
        CT_7SUN
    },
    "$VER: new_zealand.country 44.0 (12.04.2013)",
    "Countries/New_Zealand"
};
