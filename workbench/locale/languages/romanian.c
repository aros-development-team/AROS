/*
    Copyright � 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: romanian.language description file.
    Dedicated to Eva C.
*/

#define LANGSTR     "romanian"          /* String version of above      */
#define NLANGSTR    "rom�n�"            /* Native version of LANGSTR    */
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
    "Duminic�", "Luni", "Mar�i", "Miercuri", "Joi",
    "Vineri", "S�mb�t�",

    /* Abbreviated days of the week */
    "Du", "Lu", "Ma", "Mi", "Jo", "Vi", "S�",

    /* Months of the year */
    "Ianuarie", "Februarie", "Martie",
    "Aprilie", "Mai", "Iunie",
    "Iulie", "August", "Septembrie",
    "Octombrie", "Noiembrie", "Decembrie",

    /* Abbreviated months of the year */
    "ian", "feb", "mar", "apr", "mai", "lun",
    "iul", "aug", "sep", "oct", "nov", "dec",

    "Desigur", /* Yes, affirmative response */
    "Nu", /* No/negative response */

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
    "ieri", "ast�zi", "m�ine", "soart�",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
