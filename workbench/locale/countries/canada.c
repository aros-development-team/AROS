/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for Canada
          based on the Amiga Developer CD 2.1 file -: 
          NDK/NDK_3.5/Examples/Locale/Countries/make_country_files.c
*/

#include "country_locale.h"
#include <libraries/locale.h>

struct IntCountryPrefs canadaPrefs =
{
    {
        /* Reserved */
        { 0, 0, 0, 0 },

        /* Country code (licence plate number), telephone code, measuring system */
        MAKE_ID('C','D','N',0), 1, MS_ISO,

        /* Date time format, date format, time format */
        "%A %B %e %Y %H:%M:%S",
        "%A %B %e %Y",
        "%H:%M:%S",

        /* Short datetime, short date, short time formats */
        "%d/%m/%Y %H:%M:%S",
        "%d/%m/%Y",
        "%H:%M",

        /* Decimal point, group separator, frac group separator */
        ",", " ", " ",

        /* For grouping rules, see <libraries/locale.h> */

        /* Grouping, Frac Grouping */
        { 3 }, { 3 },

        /* Mon dec pt, mon group sep, mon frac group sep */
        ",", " ", " ",

        /* Mon Grouping, Mon frac grouping */
        { 3 }, { 3 },

        /* Mon Frac digits, Mon IntFrac digits, then number of digits in
           the fractional part of the money value. Most countries that
           use dollars and cents, would have 2 for this value

           (As would many of those you don't).
        */
        2, 2,

        /* Currency symbol, Small currency symbol */
        "$", "¢",

        /* Int CS, this is the ISO 4217 symbol, followed by the character to
           separate that symbol from the rest of the money. (\x00 for none).
        */
        "CAD",

        /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
        "", SS_NOSPACE, SP_PREC_ALL, CSP_SUCCEEDS,

        /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
        "-", SS_NOSPACE, SP_PREC_ALL, CSP_SUCCEEDS,

        /* Calendar type */
        CT_7SUN
    },
    "$VER: canada.country 44.1 (21.07.2016)",
    NULL,
    "Countries/Canada"
};
