/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for Malta.
    Lang: English
*/

#include <libraries/locale.h>
#include <prefs/locale.h>

/* malta.country: Stefan Haubenthal <polluks@sdf.lonestar.org> */
/* Based on v51.1 */

struct CountryPrefs maltaPrefs =
{
    /* Reserved */
    { 0, 0, 0, 0 },

    /* Country code (licence plate number), telephone code, measuring system */
    MAKE_ID('M',0,0,0), 356, MS_ISO,

    /* Date time format, date format, time format */
    "%A, %d ta %b, %Y %H:%M:%S",
    "%A, %d ta %b, %Y",
    "%H:%M:%S",

    /* Short datetime, short date, short time formats */
    "%A, %d ta %b, %y %H:%M",
    "%A, %d ta %b, %y",
    "%H:%M",

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

#ifdef _EURO
    /* Currency symbol, Small currency symbol */
    "Euro", "Cent",

    /* Int CS, this is the ISO 4217 symbol, followed by the character to
       separate that symbol from the rest of the money. (\x00 for none).
    */
    "EUR",
#else
    /* Currency symbol, Small currency symbol */
    "Lm", "Cent",

    /* Int CS, this is the ISO 4217 symbol, followed by the character to
       separate that symbol from the rest of the money. (\x00 for none).
    */
    "MTL",
#endif

    /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
    "", SS_SPACE, SP_PREC_ALL, CSP_PRECEDES,

    /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
    "-", SS_NOSPACE, SP_PARENS, CSP_PRECEDES,

    /* Calendar type */
    CT_7SUN
};
