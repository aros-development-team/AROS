/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for brasil
*/

#include <exec/types.h>
#include <libraries/locale.h>
#include <libraries/iffparse.h>
#include <prefs/locale.h>

/* united_states.country: jdias */
   
struct CountryPrefs brasilPrefs =
{
    /* Reserved */
    { 0, 0, 0, 0 },

    /* Country code (licence plate number), telephone code, measuring (system BRZ = ?)*/
    MAKE_ID('B','R',0,0), 55, MS_ISO,

    /* Date time format (dd de mmm de yyyy - h:m = ?), date format (dd de mmm de yyyy =?), time format */
    "%A %B %e de %Y - %H:%M",
    "%A %B %e de %Y",
    "%H:%M:%S %p",

    /* Short datetime, short date, short time formats */
    "%d/%m/%Y - %H:%M %p",
    "%d/%m/%Y",
    "%H:%M %p",

    /* Decimal point, group separator, frac group separator */
    ",", ".", ",",

    /* For grouping rules, see <libraries/locale.h> */

    /* Grouping, Frac Grouping */
    { 3 }, { 2 },

    /* Mon dec pt, mon group sep, mon frac group sep */
    ".", ",", ",",

    /* Mon Grouping, Mon frac grouping */
    { 3 }, { 2 },

    /* Mon Frac digits, Mon IntFrac digits, then number of digits in
       the fractional part of the money value. Most countries that
       use dollars and cents, would have 2 for this value

       (As would many of those you don't).
    */
    2, 2,

    /* Currency symbol, Small currency symbol */
    "R$", "Centavos",

    /* Int CS, this is the ISO 4217 symbol, followed by the character to
       separate that symbol from the rest of the money. (\x00 for none).
    */
    "BRL",

    /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
    "", SS_NOSPACE, SP_PREC_ALL, CSP_PRECEDES,

    /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
    "-", SS_NOSPACE, SP_PREC_ALL, CSP_PRECEDES,

    /* Calendar type */
    CT_7MON
};
