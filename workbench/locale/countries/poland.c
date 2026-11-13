/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc: Country data for Polska (Poland)
    Author: Przemyslaw Szczygielski <qus@go2.pl>
*/

#include "country_locale.h"
#include <libraries/locale.h>

struct IntCountryPrefs polandPrefs =
{
    {
        /* Reserved */
        { 0, 0, 0, 0 },

        /* Country code (licence plate number), telephone code, measuring system */
        MAKE_ID('P','L',0,0), 48, MS_ISO,

        /* Date time format, date format, time format */
        "%A %e. %B %Y %q:%M",
        "%A %e. %B %Y",
        "%q:%M:%S",

        /* Short datetime, short date, short time formats */
        "%e.%m.%y %q:%M",
        "%e.%m.%y",
        "%q:%M",

        /* Decimal point, group separator, frac group separator */
        ",", ".", "",

        /* For grouping rules, see <libraries/locale.h> */

        /* Grouping, Frac Grouping */
        { 3 }, { 0 },

        /* Mon dec pt, mon group sep, mon frac group sep */
        ",", ".", "",

        /* Mon Grouping, Mon frac grouping */
        { 3 }, { 0 },

        /* Mon Frac digits, Mon IntFrac digits, then number of digits in
           the fractional part of the money value. Most countries that
           use dollars and cents, would have 2 for this value

           (As would many of those who don't).
        */
        2, 2,

        /* Currency symbol, Small currency symbol */
        "z\263", "gr",

        /* Int CS, this is the ISO 4217 symbol, followed by the character to
           separate that symbol from the rest of the money. (\x00 for none).
        */
        "PLN",

        /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
        "", SS_NOSPACE, SP_PREC_ALL, CSP_SUCCEEDS,

        /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
        "-", SS_NOSPACE, SP_PREC_ALL, CSP_SUCCEEDS,

        /* Calendar type */
        CT_7MON
    },
    "$VER: poland.country 44.0 (12.04.2013)",
    "Polska",
    "Countries/Poland"
};
