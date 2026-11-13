/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: czech.language description file.
    Char: ISO 8859-2
*/

#define LANGSTR     "czech"             /* String version of above      */
#define NLANGSTR    "czech"             /* Native version of LANGSTR    */
#define LANGVER     41                  /* Version number of language   */
#define LANGREV     4                   /* Revision number of language  */
#define LANGTAG     "\0$VER: " LANGSTR ".language 41.4 (30.09.2014)"
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
    &AROS_SLIB_ENTRY(null, language,0),
    &AROS_SLIB_ENTRY(null, language,0),
    &AROS_SLIB_ENTRY(null, language,0),
    &AROS_SLIB_ENTRY(getlangstring, language,9),
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

    // NOTICE: stegerg: I think this must always start with Sunday and not what comment above says

    "ned" "\xEC" "le", "pond" "\xEC" "l" "\xED", "\xFA" "ter" "\xFD", "st" "\xF8" "eda", "\xE8" "tvrtek",
    "p" "\xE1" "tek", "sobota",

    /* Abbreviated days of the week */
    "ne", "po", "\xFA" "t", "st", "\xE8" "t", "p" "\xE1", "so",

    /* Months of the year */
    "leden", "\xFA" "nor", "b" "\xF8" "ezen",
    "duben", "kv" "\xEC" "ten", "\xE8" "erven",
    "\xE8" "ervenec", "srpen", "z" "\xE1\xF8\xED",
    "\xF8\xED" "jen", "listopad", "prosinec",

    /* Abbreviated months of the year */
    "led", "\xFA" "no", "b" "\xF8" "e", "dub", "kv" "\xEC", "\xE8" "en",
    "\xE8" "ec", "srp", "z" "\xE1\xF8", "\xF8\xED" "j", "lis", "pro",

    "Ano", /* Yes, affirmative response */
    "Ne",  /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "dop.", "odp.",

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
    "v" "\xE8" "era", "dnes", "z" "\xED" "tra", "budoucnost",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
