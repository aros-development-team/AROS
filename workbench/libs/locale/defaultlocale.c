/*
    Copyright (C) 1995-1998 AROS
    $Id$

    Desc: Default Locale Preferences
    Lang: english
*/

#include <exec/types.h>
#include <libraries/locale.h>

#include "locale_intern.h"

/* We have to be careful not to try and change any of this */
const struct Locale defLocale =
{
    NULL,                           /* Locale Name */
    "english",                      /* Language Name */
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
