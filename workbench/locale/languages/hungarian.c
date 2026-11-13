/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: hungarian.language description file.
    Char: ISO 8859-2
*/

/*  Collation tables need to be implemented */

#define LANGSTR     "hungarian"         /* String version of above      */
#define NLANGSTR    "magyar"            /* Native version of LANGSTR    */
#define LANGVER     41                  /* Version number of language   */
#define LANGREV     2                   /* Revision number of language  */
#define LANGTAG     "\0$VER: " LANGSTR ".language 41.2 (30.09.2014)"
#define NLANGTAG    "$NLANG:" NLANGSTR

#include "lang_openclose.inc"
#include "lang_getlangstring.inc"

/* -----------------------------------------------------------------------
   Library function table - you will need to alter this
   I have this right here at the end of the library so that I do not
   have to have prototypes for the functions. Although you could do that.
   ----------------------------------------------------------------------- */

void *const functable[] =
{
    &AROS_SLIB_ENTRY(open,language,1),
    &AROS_SLIB_ENTRY(close,language,2),
    &AROS_SLIB_ENTRY(expunge,language,3),
    &AROS_SLIB_ENTRY(null,language,0),
    &AROS_SLIB_ENTRY(mask,language,5),

    /* Note, shorter function table, as only getlangstring is used */

    /* 0 - 3 */
    &AROS_SLIB_ENTRY(null, language, 0),
    &AROS_SLIB_ENTRY(null, language, 0),
    &AROS_SLIB_ENTRY(null, language, 0),
    &AROS_SLIB_ENTRY(getlangstring, language, 9),
    (void *)-1
};

/*
    Note how only the required data structures are kept...

    This is the list of strings. It is an array of pointers to strings,
    although how it is laid out is implementation dependant.
*/
const STRPTR ___strings[] =
{
    /* A blank string */
    "",

    /*  The days of the week. Starts with the first day of the week.
        In English this would be Sunday, this depends upon the settings
        of Locale->CalendarType.
    */
    "H" "\xE9" "tf" "\xF4", "Kedd", "Szerda", "Cs" "\xFC" "t" "\xF6" "rt" "\xF6" "k", "P" "\xE9" "ntek",
    "Szombat", "Vas" "\xE1" "rnap",

    /* Abbreviated days of the week */
    "H", "K", "Sz", "Cs", "P", "Szo", "V",

    /* Months of the year */
    "Janu" "\xE1" "r", "Febru" "\xE1" "r", "M" "\xE1" "rcius",
    "\xC1" "prilis", "M" "\xE1" "jus", "J" "\xFA" "nius",
    "Julius", "Augusztus", "Szeptember",
    "Okt" "\xF3" "ber", "November", "December",

    /* Abbreviated months of the year */
    "I", "II", "III", "IV", "V", "VI",
    "VII", "VIII", "IX", "X", "XI", "XII",

    "Igen", /* Yes, affirmative response */
    "Nem", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "D" "\xE9" "lel" "\xF4" "tt", "D" "\xE9" "lut" "\xE1" "n",

    /* Soft and hard hyphens */
    "\xAD", "-",

    /* Open and close quotes */
    "\"", "\"",

    /* Days: But not actual day names
       Yesterday - the day before the current
       Today - the current day
       Tomorrow - the next day
       Future.
    */
    "Tegnap", "Ma", "Holnap", "J" "\xF6" "v" "\xF4",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
