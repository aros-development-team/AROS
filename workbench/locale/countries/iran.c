/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data for Iran.
*/

#include <exec/types.h>
#include <libraries/locale.h>
#include <libraries/iffparse.h>
#include <prefs/locale.h>

/* iran.country: based on this file by Paymaan Jafari */
   
struct CountryPrefs iranPrefs =
{
    /* Reserved */
    { 0, 0, 0, 0 },

    /* Country code (licence plate number), telephone code, measuring system */
    MAKE_ID('I','R',0,0), 98, MS_ISO,

    /* Date time format, date format, time format */
    "%Y %B %e %p %Q:%M %A",
    "%Y %B %e %A",
    "%p %Q:%M:%S",

    /* Short datetime, short date, short time formats */
    "%y/%m/%d %p %Q:%M",
    "%y/%m/%d",
    "%p %Q:%M",

    /* Decimal point, group separator, frac group separator */
    ".", ",", ",",

    /* For grouping rules, see <libraries/locale.h> */

    /* Grouping, Frac Grouping */
    { 3 }, { 0 },

    /* Mon dec pt, mon group sep, mon frac group sep */
    ".", ",", ",",

    /* Mon Grouping, Mon frac grouping */
    { 3 }, { 0 },

    /* Mon Frac digits, Mon IntFrac digits, then number of digits in
       the fractional part of the money value. Most countries that
       use dollars and cents, would have 2 for this value

       (As would many of those you don't).
    */
    2, 2,

    /* Currencs symbol, Small currency symbol */
    "Rl.","",
    
    /* Int CS, this is the ISO 4217 symbol, ...*/
    "IRR",

    /* Mon +ve sign, +ve space sep, +ve sign pos, +ve cs pos */
    "", SS_SPACE, SP_PREC_ALL, CSP_PRECEDES,

    /* Mon -ve sign, -ve space sep, -ve sign pos, -ve cs pos */
    "-", SS_SPACE, SP_PREC_ALL, CSP_PRECEDES,

    /* Calendar type */
    CT_7MON
};
