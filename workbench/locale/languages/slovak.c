/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.

    Desc: slovak.language description file.
    Credits: Jaroslav Pokorný & ATO-CZ
*/

#define LANGSTR     "slovak"            /* String version of above      */
#define NLANGSTR    "slovenèina"        /* Native version of LANGSTR    */
#define LANGVER     41                  /* Version number of language   */
#define LANGREV     0                   /* Revision number of language  */
#define LANGTAG     "\0$VER: " LANGSTR ".language 41.0 (09.07.2015)"
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
    "nedeãa", "pondelok", "utorok", "streda", "îtvrtok",
    "piatok", "sobota",

    /* Abbreviated days of the week */
    "Ne", "Po", "Ut", "St", "Ît", "Pi", "So",

    /* Months of the year */
    "január", "február", "marec",
    "apríl", "máj", "jún",
    "júl", "august", "september",
    "október", "november", "december",

    /* Abbreviated months of the year */
    "jan", "feb", "mar", "apr", "máj", "jún",
    "júl", "aug", "sep", "okt", "nov", "dec",

    "Áno", /* Yes, affirmative response */
    "Nie", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "doob.", "poob.",

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
    "vçera", "dnes", "zajtra", "budúcnosï",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
