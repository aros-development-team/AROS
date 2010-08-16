/*
    Copyright � 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for bosna_i_hercegovina
*/

#include <libraries/locale.h>
#include <prefs/locale.h>

/* bosna_i_hercegovina.country: V44 */
   
struct CountryPrefs bosna_i_hercegovinaPrefs =
{
    /* Reserved */
    { 0, 0, 0, 0 },

    /* Country code (licence plate number), telephone code, measuring system */
    MAKE_ID('B','i','H', 0 ), 387, MS_ISO,

    /* Date time format, date format, time format */
    "%q:%M:%S %d. %m. %Y.",
    "%A %e %B %Y",
    "%q:%M:%S",

    /* Short datetime, short date, short time formats */
    "%H:%M:%S",
    "%e. %b. %Y",
    "%H:%M:%S",

    /* Decimal point, group separator, frac group separator */
    ",", " ", "",

    /* For grouping rules, see <libraries/locale.h> */

    /* Grouping, Frac Grouping */
    { 3 }, { 3 },

    /* Mon dec pt, mon group sep, mon frac group sep */
    ",", ".", "",

    /* Mon Grouping, Mon frac grouping */
    { 0 }, { 0 },

    /* Mon Frac digits, Mon IntFrac digits, then number of digits in
       the fractional part of the money value. Most countries that
       use dollars and cents, would have 2 for this value

       (As would many of those you don't).
    */
    2, 2,

    /* Currency symbol, Small currency symbol */
    "KM", "Pf",

    /* Int CS, this is the ISO 4217 symbol, followed by the character to
       separate that symbol from the rest of the money. (\x00 for none).
    */
    "BAM",

    /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
    "", SS_NOSPACE, SP_PREC_ALL, CSP_SUCCEEDS,

    /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
    "-", SS_NOSPACE, SP_PARENS, CSP_SUCCEEDS,

    /* Calendar type */
    CT_7THU
};
