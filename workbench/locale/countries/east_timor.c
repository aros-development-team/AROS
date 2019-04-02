/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for Timor-Leste (East Timor)
    Author: João Ralha
*/

#include "country_locale.h"
#include <libraries/locale.h>

struct IntCountryPrefs east_timorPrefs =
{
    {
        /* Reserved */
        { 0, 0, 0, 0 },

        /* Country code (licence plate number), telephone code, measuring system */
        MAKE_ID('T','L',0,0), 670, MS_ISO,

        /* Date time format, date format, time format */
        "%A, %e de %B de %Y, %H:%M:%S",
        "%A, %e de %B de %Y",
        "%H:%M:%S",

        /* Short datetime, short date, short time formats */
        "%e %b %Y, %H:%M:%S",
        "%e %b %Y",
        "%H:%M",

        /* Decimal point, group separator, frac group separator */
        ",", "'", "'",

        /* For grouping rules, see <libraries/locale.h> */

        /* Grouping, Frac Grouping */
        { 3 }, { 3 },

        /* Mon dec pt, mon group sep, mon frac group sep */
        ",", ".", ".",


        /* Mon Grouping, Mon frac grouping */
        { 3 }, { 3 },

        /* Mon Frac digits, Mon IntFrac digits, then number of digits in
           the fractional part of the money value. Most countries that
           use dollars and cents, would have 2 for this value

           (As would many of those you don't).
        */
        2, 3,

        /* Currency symbol, Small currency symbol */
        "$", "\xA2",

        /* Int CS, this is the ISO 4217 symbol, followed by the character to
           separate that symbol from the rest of the money. (\x00 for none).
        */
        "USD",

        /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
        "+", SS_SPACE, SP_PREC_ALL, CSP_PRECEDES,

        /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
        "-", SS_SPACE, SP_PREC_ALL, CSP_SUCCEEDS,

        /* Calendar type */
        CT_7MON
    },
    "$VER: east_timor.country 44.0 (02.04.2019)",
    "Timor_Leste",
    "Countries/East_Timor"
};
