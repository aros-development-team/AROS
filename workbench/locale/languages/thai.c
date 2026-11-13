/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: thai.language description file.
    Char: ISO 8859-11
*/

#define LANGSTR     "thai"              /* String version of above      */
#define NLANGSTR    "thai"              /* Native version of LANGSTR    */
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

    // NOTICE: stegerg: I think this must always start with Sunday and not what comment above says

    "\xC7\xD1\xB9\xCD\xD2\xB7\xD4\xB5\xC2\xEC", "\xC7\xD1\xB9\xA8\xD1\xB9\xB7\xC3\xEC", "\xC7\xD1\xB9\xCD\xD1\xA7\xA4\xD2\xC3", "\xC7\xD1\xB9\xBE\xD8\xB8", "\xC7\xD1\xB9\xBE\xC4\xCB\xD1\xCA\xBA\xB4\xD5",
    "\xC7\xD1\xB9\xC8\xD8\xA1\xC3\xEC", "\xC7\xD1\xB9\xE0\xCA\xD2\xC3\xEC",

    /* Abbreviated days of the week */
    "\xCD\xD2" ".", "\xA8" ".", "\xCD" ".", "\xBE" ".", "\xBE\xC4" ".", "\xC8" ".", "\xCA" ".",

    /* Months of the year */
    "\xC1\xA1\xC3\xD2\xA4\xC1", "\xA1\xD8\xC1\xC0\xD2\xBE\xD1\xB9\xB8\xEC", "\xC1\xD5\xB9\xD2\xA4\xC1",
    "\xE0\xC1\xC9\xD2\xC2\xB9", "\xBE\xC4\xC9\xC0\xD2\xA4\xC1", "\xC1\xD4\xB6\xD8\xB9\xD2\xC2\xB9",
    "\xA1\xC3\xA1\xAE\xD2\xA4\xC1", "\xCA\xD4\xA7\xCB\xD2\xA4\xC1", "\xA1\xD1\xB9\xC2\xD2\xC2\xB9",
    "\xB5\xD8\xC5\xD2\xA4\xC1", "\xBE\xC4\xC8\xA8\xD4\xA1\xD2\xC2\xB9", "\xB8\xD1\xB9\xC7\xD2\xA4\xC1",

    /* Abbreviated months of the year */
    "\xC1" "." "\xA4" ".", "\xA1" "." "\xBE" ".", "\xC1\xD5" "." "\xA4" ".", "\xE0\xC1" "." "\xC2" ".", "\xBE" "." "\xA4" ".", "\xC1\xD4" "." "\xC2" ".",
    "\xA1" "." "\xA4" ".", "\xCA" "." "\xA4" ".", "\xA1" "." "\xC2" ".", "\xB5" "." "\xA4" ".", "\xBE" "." "\xC2" ".", "\xB8" "." "\xA4" ".",

    "\xE3\xAA\xE8", /* Yes, affirmative response */
    "\xE4\xC1\xE8", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "AM", "PM",

    /* Soft and hard hyphens */
    "", "-",

    /* Open and close quotes */
    "\"", "\"",

    /* Days: But not actual day names
       Yesterday - the day before the current
       Today - the current day
       Tomorrow - the next day
       Future.
    */
    "\xE0\xC1\xD7\xE8\xCD\xC7\xD2\xB9\xB9\xD5\xE9", "\xC7\xD1\xB9\xB9\xD5\xE9", "\xC7\xD1\xB9\xBE\xC3\xD8\xE8\xA7\xB9\xD5\xE9", "\xCD\xB9\xD2\xA4\xB5",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
