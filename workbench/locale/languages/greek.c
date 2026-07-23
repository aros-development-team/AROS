/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    Desc: greek.language description file.
    Char: ISO 8859-7
*/

#define LANGSTR     "greek"             /* String version of above      */
#define NLANGSTR    "greek"           /* Native version of LANGSTR    */
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
    "\xCA\xF5\xF1\xE9\xE1\xEA\xDE", "\xC4\xE5\xF5\xF4\xDD\xF1\xE1", "\xD4\xF1\xDF\xF4\xE7", "\xD4\xE5\xF4\xDC\xF1\xF4\xE7", "\xD0\xDD\xEC\xF0\xF4\xE7",
    "\xD0\xE1\xF1\xE1\xF3\xEA\xE5\xF5\xDE", "\xD3\xDC\xE2\xE2\xE1\xF4\xEF",

    /* Abbreviated days of the week */
    "\xCA\xF5", "\xC4\xE5", "\xD4\xF1", "\xD4\xE5", "\xD0\xE5", "\xD0\xE1", "\xD3\xE1",

    /* Months of the year */
    "\xC9\xE1\xED\xEF\xF5\xDC\xF1\xE9\xEF\xF2", "\xD6\xE5\xE2\xF1\xEF\xF5\xDC\xF1\xE9\xEF\xF2", "\xCC\xDC\xF1\xF4\xE9\xEF\xF2",
    "\xC1\xF0\xF1\xDF\xEB\xE9\xEF\xF2", "\xCC\xDC\xE9\xEF\xF2", "\xC9\xEF\xFD\xED\xE9\xEF\xF2",
    "\xC9\xEF\xFD\xEB\xE9\xEF\xF2", "\xC1\xFD\xE3\xEF\xF5\xF3\xF4\xEF\xF2", "\xD3\xE5\xF0\xF4\xDD\xEC\xE2\xF1\xE9\xEF\xF2",
    "\xCF\xEA\xF4\xFE\xE2\xF1\xE9\xEF\xF2", "\xCD\xEF\xDD\xEC\xE2\xF1\xE9\xEF\xF2", "\xC4\xE5\xEA\xDD\xEC\xE2\xF1\xE9\xEF\xF2",

    /* Abbreviated months of the year */
    "\xC9\xE1\xED", "\xD6\xE5\xE2", "\xCC\xE1\xF1", "\xC1\xF0\xF1", "\xCC\xE1\xE9", "\xC9\xEF\xF5",
    "\xC9\xEF\xF5", "\xC1\xF5\xE3", "\xD3\xE5\xF0", "\xCF\xEA\xF4", "\xCD\xEF\xE5", "\xC4\xE5\xEA",

    "\xCD\xE1\xE9", /* Yes, affirmative response */
    "\xBC\xF7\xE9", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "\xF0\xEC", "\xEC\xEC",

    /* Soft and hard hyphens */
    "-", "-",

    /* Open and close quotes */
    "\"", "\"",

    /* Days: But not actual day names
       Yesterday - the day before the current
       Today - the current day
       Tomorrow - the next day
       Future.
    */
    "\xD7\xE8\xE5\xF2", "\xD3\xDE\xEC\xE5\xF1\xE1", "\xC1\xFD\xF1\xE9\xEF", "\xCC\xE5\xEB\xEB\xEF\xED\xF4\xE9\xEA\xDC",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
