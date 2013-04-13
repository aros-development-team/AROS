/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for Hellas (Greece)
    Author: George Sokianos <walkero@yahoo.com>
*/

#include "country_locale.h"
#include <libraries/locale.h>

struct IntCountryPrefs greecePrefs =
{
    {
        /* Reserved */
        { 0, 0, 0, 0 },

        /* The country codes in the past have been rather inconsistant,
           sometimes they are 1 character, 2 chars or 3. It would be nice
           to have some consistency. Maybe use the 3 character name from
           ISO 3166? I (Iain) have a copy of the ISO3166 codes if anyone
           wants them...
        */

        /* Country code (left justify), telephone code, measuring system */
        MAKE_ID('G','R',0,0), 30, MS_ISO,

        /* Date time format, date format, time format */
        "%d/%m/%Y %q:%M:%S",
        "%A %e %B %Y",
        "%q:%M:%S",

        /* Short datetime, short date, short time formats */
        "%H:%M:%S %d/%m/%Y",
        "%e-%b-%Y",
        "%H:%M:%S",

        /* Decimal point, group separator, frac group separator */
        ",", ".", "",

        /* For grouping rules, see <libraries/locale.h> */

        /* Grouping, Frac Grouping */
        { 3 }, { 255 },

        /* Mon dec pt, mon group sep, mon frac group sep */
        ",", ".", ".",

        /* Mon Grouping, Mon frac grouping */
        { 3 }, { 3 },

#ifdef _EURO
        /* Mon Frac digits, Mon IntFrac digits, then number of digits in
           the fractional part of the money value. Most countries that
           use dollars and cents, would have 2 for this value

           (As would many of those you don't).
        */
        2, 3,

        /* Currency symbol, Small currency symbol */
        "Euro", "Cent",

        /* Int CS, this is the ISO 4217 symbol, followed by the character to
           separate that symbol from the rest of the money. (\x00 for none).
        */
        "EUR",
#else
        0, 3,
        "drx", "d",
        "DRX",
#endif
        /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
        "", SS_SPACE, SP_PREC_ALL, CSP_PRECEDES,

        /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
        "-", SS_SPACE, SP_SUCC_CURR, CSP_PRECEDES,

        /* Calendar type */
        CT_7SUN
    },
    "$VER: greece.country 44.0 (12.04.2013)",
    "Hellas",
    "Countries/Greece"
};
