/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data template.
    Author: Iain Templeton <iain@ugh.net.au>
*/

#include "country_locale.h"
#include <libraries/locale.h>

/*  Country data template/example.
    Authors should add their name, and optionaly email address in the top 
    section of this file.

    Note: NULL values in strings that actually mean something __MUST__
    be explicitly specified...

    N.B. - The example values in this file are from australia.country,
    You must change the values to suit your own country - the data
    shown is purely here to show what goes where.
*/

struct IntCountryPrefs templatePrefs =
{
    {
        /* Reserved */
        { 0, 0, 0, 0 },

        /* The country codes should be based upon ISO 3166 for AROS.
           I (Iain) have a copy of the ISO3166 codes if anyone wants them... */

        /* Country code (left justify), telephone code, measuring system */
        MAKE_ID('A','U','S',0), 61, MS_ISO,

        /* Date time format, date format, time format */
        "%A %B %e %Y %I:%M%p",
        "%A %B %e %Y",
        "%I:%M:%S%p",

        /* Short datetime, short date, short time formats */
        "%e/%m/%y %I:%M%p",
        "%e/%m/%y",
        "%I:%M%p",

        /* Decimal point, group separator, frac group separator */
        ".", " ", "\x00",

        /* For grouping rules, see <libraries/locale.h> */

        /* Grouping, Frac Grouping */
        "\x03\x00", "\x00",

        /* Mon dec pt, mon group sep, mon frac group sep */
        ".", " ", "\x00",

        /* Mon Grouping, Mon frac grouping */
        "\x03\x00", "\x00",

        /* Mon Frac digits, Mon IntFrac digits, then number of digits in
           the fractional part of the money value. Most countries that
           use dollars and cents, would have 2 for this value

           (As would many of those who don't).
        */
        2, 2,

        /* Currency symbol, Small currency symbol */
        "$", "c",

        /* Int CS, this is the ISO 4217 symbol, followed by the character to
           separate that symbol from the rest of the money. (\x00 for none).
        */
        "AUD\x00",

        /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
        "\x00", SS_SPACE, SP_PREC_ALL, CSP_PRECEDES,

        /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
        "-", SS_NOSPACE, SP_PARENS, CSP_PRECEDES,

        /* Calendar type */
        CT_7SUN
    },
    "$VER: template.country 44.0 (12.12.2017)",
    NULL,
    "Countries/Template"
};
