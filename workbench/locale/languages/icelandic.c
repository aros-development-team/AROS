/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: icelandic.language description file.
*/

#define LANGSTR     "icelandic"         /* String version of above      */
#define NLANGSTR    ISOASCII_iacute "slenska"          /* Native version of LANGSTR    */
#define LANGVER     41                  /* Version number of language   */
#define LANGREV     2                   /* Revision number of language  */
#define LANGTAG     "\0$VER: " LANGSTR ".language 41.2 (30.09.2014)"
#define NLANGTAG    "$NLANG:" NLANGSTR

#include <aros/isoascii.h>
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

    "Sunnudagur", "M" ISOASCII_aacute "nudagur", ISOASCII_thorn "ri" ISOASCII_eth "jundagur", "Mi" ISOASCII_eth "vikudagur", "Fimmtudagur",
    "F" ISOASCII_ouml "studagur", "Laugardagur",

    /* Abbreviated days of the week */
    "Sun", "M" ISOASCII_aacute "n", ISOASCII_thorn "ri", "Mi" ISOASCII_eth, "Fim", "F" ISOASCII_ouml "s", "Lau",

    /* Months of the year */
    "Jan" ISOASCII_uacute "ar", "Febr" ISOASCII_uacute "ar", "Mars",
    "Apr" ISOASCII_iacute "l", "Ma" ISOASCII_iacute, "J" ISOASCII_uacute "n" ISOASCII_iacute,
    "J" ISOASCII_uacute "l" ISOASCII_iacute, ISOASCII_Aacute "g" ISOASCII_uacute "st", "September",
    "Okt" ISOASCII_oacute "ber", "N" ISOASCII_oacute "vember", "Desember",

    /* Abbreviated months of the year */
    "Jan", "Feb", "Mar", "Apr", "Ma" ISOASCII_iacute, "J" ISOASCII_uacute "n",
    "J" ISOASCII_uacute "l", ISOASCII_Aacute "g", "Sep", "Okt", "N" ISOASCII_oacute "v", "Des",

    "J" ISOASCII_aacute, /* Yes, affirmative response */
    "Nei", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "fh.", "eh.",

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
    ISOASCII_Iacute " g" ISOASCII_aelig "r", ISOASCII_Iacute " dag", ISOASCII_Aacute " morgun", "Framt" ISOASCII_iacute ISOASCII_eth,

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
