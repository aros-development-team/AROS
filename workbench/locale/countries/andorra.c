/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for Andorra
    Author: Stefan Haubenthal <polluks@sdf.lonestar.org>
*/

#include "country_locale.h"
#include <libraries/locale.h>

struct IntCountryPrefs andorraPrefs =
{
    {
        /* Reserved */
        { 0, 0, 0, 0 },

        /* Country code (licence plate number), telephone code, measuring system */
        MAKE_ID('A','N','D',0), 376, MS_ISO,

        /* Date time format, date format, time format */
        "%e-%m-%Y %H:%M:%S",
        "%e-%m-%Y",
        "%H:%M:%S",

        /* Short datetime, short date, short time formats */
        "%e-%m-%Y %H:%M:%S",
        "%e-%m-%Y",
        "%H:%M:%S",

        /* Decimal point, group separator, frac group separator */
        "'", ",", "",

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

           (As would many of those you don't).
        */
        2, 2,

#ifdef _EURO
        /* Currency symbol, Small currency symbol */
        "Euro", "Cent",

        /* Int CS, this is the ISO 4217 symbol, followed by the character to
           separate that symbol from the rest of the money. (\x00 for none).
        */
        "EUR",
#else
        "Pesetas", "",
        "ESB",
#endif
        /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
        "", SS_SPACE, SP_PREC_ALL, CSP_SUCCEEDS,

        /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
        "-", SS_NOSPACE, SP_PREC_ALL, CSP_PRECEDES,

        /* Calendar type */
        CT_7MON
    },
    "$VER: andorra.country 44.0 (12.04.2013)",
    NULL,
    "Countries/Andorra"
};
