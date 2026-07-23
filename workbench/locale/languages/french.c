/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: french.language description file.
*/

#define LANGSTR     "french"            /* String version of above      */
#define NLANGSTR    "fran" ISOASCII_ccedil "ais"          /* Native version of LANGSTR    */
#define LANGVER     41          /* Version number of language   */
#define LANGREV     2           /* Revision number of language  */
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

    "Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi",
    "Vendredi", "Samedi",

    /* Abbreviated days of the week */
    "Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam",

    /* Months of the year */
    "Janvier", "F" ISOASCII_eacute "vrier", "Mars",
    "Avril", "Mai", "Juin",
    "Juillet", "Ao" ISOASCII_ucirc "t", "Septembre",
    "Octobre", "Novembre", "D" ISOASCII_eacute "cembre",

    /* Abbreviated months of the year */
    "Jan", "F" ISOASCII_eacute "v", "Mars", "Avr", "Mai", "Juin",
    "Juil", "Ao" ISOASCII_ucirc "t", "Sep", "Oct", "Nov", "D" ISOASCII_eacute "c",

    "Oui", /* Yes, affirmative response */
    "Non", /* No/negative response */

    /* AM/PM strings AM 0000 -> 1159, PM 1200 -> 2359 */
    "AM", "PM",

    /* Soft and hard hyphens */
     "\xAD", "-",

    /* Open and close quotes */
     "\xAB\xA0", "\xA0\xBB",

    /* Days: But not actual day names
       Yesterday - the day before the current
       Today - the current day
       Tomorrow - the next day
       Future.
    */
    "Hier", "Aujourd'hui", "Demain", "Futur",

    /* Native language name */
    NLANGSTR
};

/* This is the end of ROMtag marker. */
const char end=0;
