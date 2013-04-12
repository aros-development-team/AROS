/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for Schweiz (Switzerland)
          based on the Amiga Developer CD 2.1 file -: 
          NDK/NDK_3.5/Examples/Locale/Countries/make_country_files.c
*/

#include "country_locale.h"
#include <libraries/locale.h>

struct IntCountryPrefs schweizPrefs =
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
        MAKE_ID('C','H',0,0), 41, MS_ISO,

        /* Date time format, date format, time format */
        "%A, %e. %B %Y %H:%M:%S",
        "%A, %e. %B %Y",
        "%H:%M:%S",

        /* Short datetime, short date, short time formats */
        "%e.%m.%Y %H:%M:%S",
        "%e.%m.%Y",
        "%H:%M",

        /* Decimal point, group separator, frac group separator */
        ".", "'", "",

        /* For grouping rules, see <libraries/locale.h> */

        /* Grouping, Frac Grouping */
        { 3 }, { 0 },

        /* Mon dec pt, mon group sep, mon frac group sep */
        ".", "'", "'",

        /* Mon Grouping, Mon frac grouping */
        { 3 }, { 3 },

        /* Mon Frac digits, Mon IntFrac digits, then number of digits in
           the fractional part of the money value. Most countries that
           use dollars and cents, would have 2 for this value

           (As would many of those you don't).
        */
        2, 2,

        /* Currency symbol, Small currency symbol */
        "SFr.", "Rp.",

        /* Int CS, this is the ISO 4217 symbol, followed by the character to
           separate that symbol from the rest of the money. (\x00 for none).
        */
        "SRF",

        /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
        "", SS_NOSPACE, SP_PREC_ALL, CSP_PRECEDES,

        /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
        "-", SS_NOSPACE, SP_SUCC_CURR, CSP_PRECEDES,

        /* Calendar type */
        CT_7MON
    },
    "$VER: schweiz.country 44.0 (12.04.2013)",
    "Countries/Switzerland"
};
