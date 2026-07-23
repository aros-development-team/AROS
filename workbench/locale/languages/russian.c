/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: russian.language description file.
    Char: Amiga-1251
*/

/*  Collation tables need to be implemented */

#define LANGSTR     "russian"           /* String version of above      */
#define NLANGSTR    "russian"           /* Native version of LANGSTR    */
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
    "\xC2\xEE\xF1\xEA\xF0\xE5\xF1\xE5\xED\xFC\xE5",   "\xCF\xEE\xED\xE5\xE4\xE5\xEB\xFC\xED\xE8\xEA",   "\xC2\xF2\xEE\xF0\xED\xE8\xEA",  "\xD1\xF0\xE5\xE4\xE0",
    "\xD7\xE5\xF2\xE2\xE5\xF0\xE3", "\xCF\xFF\xF2\xED\xE8\xF6\xE0",   "\xD1\xF3\xE1\xE1\xEE\xF2\xE0",

    /* Abbreviated days of the week */
    "\xC2\xF1", "\xCF\xED", "\xC2\xF2", "\xD1\xF0", "\xD7\xF2", "\xCF\xF2", "\xD1\xE1",

    /* Months of the year */
    "\xDF\xED\xE2\xE0\xF0\xFC",  "\xD4\xE5\xE2\xF0\xE0\xEB\xFC", "\xCC\xE0\xF0\xF2",
    "\xC0\xEF\xF0\xE5\xEB\xFC",  "\xCC\xE0\xE9",     "\xC8\xFE\xED\xFC",
    "\xC8\xFE\xEB\xFC",    "\xC0\xE2\xE3\xF3\xF1\xF2",  "\xD1\xE5\xED\xF2\xFF\xE1\xF0\xFC",
    "\xCE\xEA\xF2\xFF\xE1\xF0\xFC", "\xCD\xEE\xFF\xE1\xF0\xFC",  "\xC4\xE5\xEA\xE0\xE1\xF0\xFC",

    /* Abbreviated months of the year */
    "\xDF\xED\xE2", "\xD4\xE5\xE2", "\xCC\xE0\xF0", "\xC0\xEF\xF0", "\xCC\xE0\xE9", "\xC8\xFE\xED",
    "\xC8\xFE\xEB", "\xC0\xE2\xE3", "\xD1\xE5\xED", "\xCE\xEA\xF2", "\xCD\xEE\xFF", "\xC4\xE5\xEA",

    "\xC4\xE0", /* Yes, affirmative response */
    "\xCD\xE5\xF2", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "am", "pm",
    /* Soft and hard hyphens */
    "\xAD", "-",

    /* Open and close quotes */
    "\xAB", "\xBB",

    /* Days: But not actual day names
       Yesterday - the day before the current
       Today - the current day
       Tomorrow - the next day
       Future.
    */
    "\xC2\xF7\xE5\xF0\xE0", "\xD1\xE5\xE3\xEE\xE4\xED\xFF", "\xC7\xE0\xE2\xF2\xF0\xE0", "\xC1\xF3\xE4\xF3\xF9\xE5\xE5",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
