/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: maltese.language description file.
    Char: ISO 8859-3
*/

#define LANGSTR     "maltese"    /* String version of above      */
#define NLANGSTR    "malti"             /* Native version of LANGSTR    */
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
    "il-" "\xA1" "add", "it-Tnejn", "it-Tlieta", "l-Erbg" "\xB1" "a", "il-" "\xA1" "amis",
    "il-" "\xD5" "img" "\xB1" "a0", "is-Sibt",

    /* Abbreviated days of the week */
    "\xA1" "ad", "Tne", "Tli", "Erb", "\xA1" "am", "\xD5" "im", "Sib",

    /* Months of the year */
    "Jannar",  "Frar",     "Marzu",
    "April",   "Mejju",    "\xD5" "unju",
    "Lulju",   "Awissu",   "Settembru",
    "Ottubru", "Novembru", "Di" "\xE5" "embru",

    /* Abbreviated months of the year */
    "Jan", "Fra", "Mar", "Apr", "Mej", "\xD5" "un",
    "Lul", "Awi", "Set", "Ott", "Nov", "Di" "\xE5",

    "Iva", /* Yes, affirmative response */
    "Le",  /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "AM", "PM",

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
    "Ilbiera" "\xB1", "Illum", "G" "\xB1" "ada", "Fil-futur",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
