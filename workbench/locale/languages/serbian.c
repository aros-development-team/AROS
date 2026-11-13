/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: serbian.language description file.
    Char: ISO 8859-5
*/

#define LANGSTR     "serbian"           /* String version of above      */
#define NLANGSTR    "srpski"            /* Native version of LANGSTR    */
#define LANGVER     41                  /* Version number of language   */
#define LANGREV     3                   /* Revision number of language  */
#define LANGTAG     "\0$VER: " LANGSTR ".language 41.3 (19.05.2016)"
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
    "\xDD\xD5\xD4\xD5\xF9\xD0", "\xDF\xDE\xDD\xD5\xD4\xD5\xF9\xD0\xDA", "\xE3\xE2\xDE\xE0\xD0\xDA", "\xE1\xE0\xD5\xD4\xD0", "\xE7\xD5\xE2\xD2\xE0\xE2\xD0\xDA",
    "\xDF\xD5\xE2\xD0\xDA", "\xE1\xE3\xD1\xDE\xE2\xD0",

    /* Abbreviated days of the week */
    "\xDD\xD5\xD4", "\xDF\xDE\xDD", "\xE3\xE2\xDE", "\xE1\xE0\xD5", "\xE7\xD5\xE2", "\xDF\xD5\xE2", "\xE1\xE3\xD1",

    /* Months of the year */
    "\xF8\xD0\xDD\xE3\xD0\xE0", "\xE4\xD5\xD1\xE0\xE3\xD0\xE0", "\xDC\xD0\xE0\xE2",
    "\xD0\xDF\xE0\xD8\xDB", "\xDC\xD0\xF8", "\xF8\xE3\xDD",
    "\xF8\xE3\xDB", "\xD0\xD2\xD3\xE3\xE1\xE2", "\xE1\xD5\xDF\xE2\xD5\xDC\xD1\xD0\xE0",
    "\xDE\xDA\xE2\xDE\xD1\xD0\xE0", "\xDD\xDE\xD2\xD5\xDC\xD1\xD0\xE0", "\xD4\xD5\xE6\xD5\xDC\xD1\xD0\xE0",

    /* Abbreviated months of the year */
    "\xF8\xD0\xDD", "\xE4\xD5\xD1", "\xDC\xD0\xE0", "\xD0\xDF\xE0", "\xDC\xD0\xF8", "\xF8\xE3\xDD",
    "\xF8\xE3\xDB", "\xD0\xD2\xD3", "\xE1\xD5\xDF", "\xDE\xDA\xE2", "\xDD\xDE\xD2", "\xD4\xD5\xE6",

    "\xB4\xD0", /* Yes, affirmative response */
    "\xBD\xD5", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "\xF8\xE3\xE2\xE0\xDE", "\xDF\xDE\xDF\xDE\xD4\xDD\xD5",

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
    "\xF8\xE3\xE7\xD5", "\xD4\xD0\xDD\xD0\xE1", "\xE1\xE3\xE2\xE0\xD0", "\xD1\xE3\xD4\xE3\xFB\xDD\xDE\xE1\xE2",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
