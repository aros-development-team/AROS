/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for Finland
*/

#include <exec/types.h>
#include <libraries/locale.h>
#include <libraries/iffparse.h>
#include <prefs/locale.h>

/* suomi.country: based on this file on Amiga Developer CD 2.1: 
   NDK/NDK_3.5/Examples/Locale/Countries/make_country_files.c */
   
struct CountryPrefs suomiPrefs =
{
    /* Reserved */
    { 0, 0, 0, 0 },

    /* Country code (licence plate number), telephone code, measuring system */
    MAKE_ID('F','I','N',0), 358, MS_ISO,

    /* Date time format, date format, time format */
    "%d.%m.%Y klo %H:%M:%S",
    "%d.%m.%Y",
    "%H:%M:%S",

    /* Short datetime, short date, short time formats */
    "%d.%m.%Y kl %H:%M:%S",
    "%d.%m.%Y",
    "%H:%M:%S",

    /* Decimal point, group separator, frac group separator */
    ",", ".", "",

    /* For grouping rules, see <libraries/locale.h> */

    /* Grouping, Frac Grouping */
    { 3 }, { 0 },

    /* Mon dec pt, mon group sep, mon frac group sep */
    ",", ".", ".",

    /* Mon Grouping, Mon frac grouping */
    { 3 }, { 2 },

    /* Mon Frac digits, Mon IntFrac digits, then number of digits in
       the fractional part of the money value. Most countries that
       use dollars and cents, would have 2 for this value

       (As would many of those you don't).
    */
    2, 2,

    /* Currency symbol, Small currency symbol */
    "Euro", "Cent",

    /* Int CS, this is the ISO 4217 symbol, followed by the character to
       separate that symbol from the rest of the money. (\x00 for none).
    */
    "EUR",

    /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
    "", SS_NOSPACE, SP_PREC_ALL, CSP_SUCCEEDS,

    /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
    "-", SS_NOSPACE, SP_PREC_ALL, CSP_SUCCEEDS,

    /* Calendar type */
    CT_7MON
};
