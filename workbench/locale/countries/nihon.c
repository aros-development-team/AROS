/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for nihon
*/

#include <libraries/locale.h>
#include <prefs/locale.h>

/* nihon.country: V44 */
   
struct CountryPrefs nihonPrefs =
{
    /* Reserved */
    { 0, 0, 0, 0 },

    /* Country code (licence plate number), telephone code, measuring system */
    MAKE_ID('J', 0 , 0 , 0 ), 81, MS_ISO,

    /* Date time format, date format, time format */
    "%YÇ¯%m·î%eÆü %H»þ%MÊ¬%SÉÃ",
    "%YÇ¯%m·î%eÆü",
    "%H»þ%MÊ¬%SÉÃ",

    /* Short datetime, short date, short time formats */
    "%YÇ¯%m·î%eÆü %H»þ%MÊ¬%SÉÃ",
    "%YÇ¯%m·î%eÆü",
    "%H»þ%MÊ¬",

    /* Decimal point, group separator, frac group separator */
    ".", ",", "",

    /* For grouping rules, see <libraries/locale.h> */

    /* Grouping, Frac Grouping */
    { 3 }, { 0 },

    /* Mon dec pt, mon group sep, mon frac group sep */
    ",", ".", "",

    /* Mon Grouping, Mon frac grouping */
    { 3 }, { 0 },

    /* Mon Frac digits, Mon IntFrac digits, then number of digits in
       the fractional part of the money value. Most countries that
       use dollars and cents, would have 2 for this value

       (As would many of those you don't).
    */
    2, 2,

    /* Currency symbol, Small currency symbol */
    "YEN", "",

    /* Int CS, this is the ISO 4217 symbol, followed by the character to
       separate that symbol from the rest of the money. (\x00 for none).
    */
    "YEN",

    /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
    "", SS_NOSPACE, SP_PREC_ALL, CSP_SUCCEEDS,

    /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
    "-", SS_NOSPACE, SP_PREC_ALL, CSP_SUCCEEDS,

    /* Calendar type */
    CT_7SUN
};
