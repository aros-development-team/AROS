/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Country data template.
    Lang:
*/

#include <exec/types.h>
#include <libraries/locale.h>
#include <libraries/iffparse.h>
#include <prefs/locale.h>

/*  Country data, it would be nice if the person who fills this in would
    leave at least their name, (the email address is optional), just for
    the record. If you have problems with this, then um...?

    Note: NULL values in strings that actually mean something __MUST__
    be explicitly specified...

    Also: All the data in here is for Australia, you will have to change
    most things. I left it in so people could see what goes where.
*/

/* australia.country: Iain Templeton, iain@ugh.net.au */
struct CountryPrefs australiaPrefs =
{
    /* Reserved */
    { 0, 0, 0, 0 },

    /* The country codes in the past have been rather inconsistant,
       sometimes they are 1 character, 2 chars or 3. It would be nice
       to have some consistency. Maybe use the 3 character name from
       ISO 3166? I (Iain) have a copy of the ISO3166 codes if anyone
       wants them...
    */

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
    "\x00", SS_NOSPACE, SP_PARENS, CSP_PRECEDES,

    /* Calendar type */
    CT_7SUN
};
