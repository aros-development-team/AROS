/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for Cabo Verde
    Author: João Ralha
*/

#include "country_locale.h"
#include <libraries/locale.h>

struct IntCountryPrefs cape_verdePrefs =
{
    {
        /* Reserved */
        { 0, 0, 0, 0 },

        /* The country codes in the past have been rather inconsistant,
           sometimes they are 1 character, 2 chars or 3. It would be nice
           to have some consistency. Maybe use the 3 character name from
           ISO 3166?
        */

        /* Country code, telephone code, measuring system */
        MAKE_ID('C','V',0,0), 238, MS_ISO,

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
        "$", "",

        /* Int CS, this is the ISO 4217 symbol, followed by the character to
           separate that symbol from the rest of the money. (\x00 for none).
        */
        "CVE",

        /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
        "+", SS_SPACE, SP_PREC_ALL, CSP_PRECEDES,

        /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
        "-", SS_SPACE, SP_PREC_ALL, CSP_SUCCEEDS,

        /* Calendar type */
        CT_7MON
    },
    "$VER: cape_verde.country 44.0 (12.04.2013)",
    "Cabo Verde",
    "Countries/Cape_Verde"
};
