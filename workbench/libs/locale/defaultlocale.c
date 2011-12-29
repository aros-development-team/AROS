/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Default Locale Preferences
    Lang: english
*/

#include <exec/types.h>
#include <libraries/locale.h>
#include <libraries/iffparse.h>

#include "locale_intern.h"


/* stegerg: the strings "united_states.country", "english.language" and
   "english" have been verified on the Amiga with a test program which
   does OpenLocale(NULL) and then print out this fields. The Amiga was
   booted "without startup-sequence" and the program then started as first,
   without other programs before" */
   
/* We have to be careful not to try and change any of this */
const struct Locale defLocale =
{
    "united_states.country",        /* Locale Name */
    "english.language",             /* Language Name */
    {   "english", NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL, NULL },   /* Prefered Languages */
    0,                              /* Flags */

    0,                              /* CodeSet */
    MAKE_ID('U','S','A',0), 1,      /* CountryCode, Telephone Code */
    0,                              /* GMT Offset */
    MS_AMERICAN,                    /* MeasuringSystem */
    CT_7SUN,                        /* Calendar Type */
    { 0, 0 },                       /* Reserved */

    "%A %B %e %Y %Q:%M %p",         /* Date/Time Format */
    "%A %B %e",                     /* Date Format */
    "%Q:%M:%S %p",                  /* Time Format */
    "%m/%d/%y %Q:%M %p",            /* Short Date/Time Format */
    "%m/%d/%y",                     /* Short Date Format */
    "%Q:%M %p",                     /* Short Time Format */

    ".", ",", ".",                  /* Decimal Point, Group Separators */
    "\x03\x00", "\x03\x00",

    ".", ",", ",",                  /* Monetary Separators (as above) */
    "\x03\x00", "\x03\x00",         /* Monetary Frac Groupings */

    2, 2, { 0, 0 },                 /* Monetary (Reg/Int) Frac Digits, Resvd */

    "$", "¢", "USD",                /* Currency Symbols */

    /* Positive Representation */
    "", SS_NOSPACE, SP_PREC_ALL, CSP_PRECEDES, 0,

    /* Negative Representation */
    "-", SS_NOSPACE, SP_PREC_ALL, CSP_PRECEDES, 0,
};
