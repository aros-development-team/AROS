/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for Vaticano (Vatican).
*/

#include <exec/types.h>
#include <libraries/locale.h>
#include <libraries/iffparse.h>
#include <prefs/locale.h>

/* vaticano.country: Stefan Haubenthal <polluks@sdf.lonestar.org> */
/* Is probably wrong in a few places */
   
struct CountryPrefs vaticanoPrefs =
{
    /* Reserved */
    { 0, 0, 0, 0 },

    /* Country code (licence plate number), telephone code, measuring system */
    MAKE_ID('S','C','V',0), 379, MS_ISO,

    /* Date time format, date format, time format */
    "%q:%M:%S %d/%m/%Y",
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
    /* Mon Frac digits, Mon IntFrac digits, then number of digits in
       the fractional part of the money value. Most countries that
       use dollars and cents, would have 2 for this value

       (As would many of those you don't).
    */
    0, 3,

    /* Currency symbol, Small currency symbol */
    "Lire", "£",

    /* Int CS, this is the ISO 4217 symbol, followed by the character to
       separate that symbol from the rest of the money. (\x00 for none).
    */
    "LIT",
#endif
    /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
    "", SS_SPACE, SP_PREC_ALL, CSP_PRECEDES,

    /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
    "-", SS_SPACE, SP_SUCC_CURR, CSP_PRECEDES,

    /* Calendar type */
    CT_7SUN
};
