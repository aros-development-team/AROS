#ifndef _LANGINFO_H
#define _LANGINFO_H
/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
*/

#include <locale.h>  // For locale_t
#include <aros/posixc/nl_types.h>  // For nl_item

#ifdef __cplusplus
extern "C" {
#endif

/* Type definition */
typedef int nl_item;

/* Constants for nl_langinfo() */
#define ABDAY_1     0x20000  /* Abbreviated Sunday name */
#define ABDAY_2     0x20001  /* Abbreviated Monday name */
#define ABDAY_3     0x20002
#define ABDAY_4     0x20003
#define ABDAY_5     0x20004
#define ABDAY_6     0x20005
#define ABDAY_7     0x20006

#define DAY_1       0x20007  /* Full Sunday name */
#define DAY_2       0x20008
#define DAY_3       0x20009
#define DAY_4       0x2000A
#define DAY_5       0x2000B
#define DAY_6       0x2000C
#define DAY_7       0x2000D

#define ABMON_1     0x2000E  /* Abbreviated January name */
#define ABMON_2     0x2000F
#define ABMON_3     0x20010
#define ABMON_4     0x20011
#define ABMON_5     0x20012
#define ABMON_6     0x20013
#define ABMON_7     0x20014
#define ABMON_8     0x20015
#define ABMON_9     0x20016
#define ABMON_10    0x20017
#define ABMON_11    0x20018
#define ABMON_12    0x20019

#define MON_1       0x2001A  /* Full January name */
#define MON_2       0x2001B
#define MON_3       0x2001C
#define MON_4       0x2001D
#define MON_5       0x2001E
#define MON_6       0x2001F
#define MON_7       0x20020
#define MON_8       0x20021
#define MON_9       0x20022
#define MON_10      0x20023
#define MON_11      0x20024
#define MON_12      0x20025

#define D_T_FMT     0x20026  /* Date & time format string */
#define D_FMT       0x20027  /* Date format string */
#define T_FMT       0x20028  /* Time format string */
#define AM_STR      0x20029
#define PM_STR      0x2002A

#define CODESET     0x2002B  /* Character encoding (e.g., UTF-8) */
#define CRNCYSTR    0x2002C  /* Currency symbol formatting */
#define RADIXCHAR   0x2002D  /* Decimal point string */
#define THOUSEP     0x2002E  /* Thousands separator string */

#define YESEXPR     0x2002F  /* Regex matching "yes" */
#define NOEXPR      0x20030  /* Regex matching "no" */
#define YESSTR      0x20031  /* Locale-specific "yes" string (obsolete) */
#define NOSTR       0x20032  /* Locale-specific "no" string (obsolete) */

/* Function prototype */
char *nl_langinfo(nl_item item);
char *nl_langinfo_l(nl_item, locale_t);

#ifdef __cplusplus
}
#endif

#endif /* _LANGINFO_H */
