/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.

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
    "недеља", "понедељак", "уторак", "среда", "четвртак",
    "петак", "субота",

    /* Abbreviated days of the week */
    "нед", "пон", "уто", "сре", "чет", "пет", "суб",

    /* Months of the year */
    "јануар", "фебруар", "март",
    "април", "мај", "јун",
    "јул", "август", "септембар",
    "октобар", "новембар", "децембар",

    /* Abbreviated months of the year */
    "јан", "феб", "мар", "апр", "мај", "јун",
    "јул", "авг", "сеп", "окт", "нов", "дец",

    "Да", /* Yes, affirmative response */
    "Не", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "јутро", "поподне",

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
    "јуче", "данас", "сутра", "будућност",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
