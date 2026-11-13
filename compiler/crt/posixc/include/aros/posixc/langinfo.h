#ifndef _LANGINFO_H
#define _LANGINFO_H
/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
*/

#include <locale.h>                 // For locale_t
#include <aros/posixc/nl_types.h>   // For nl_item

#ifdef __cplusplus
extern "C" {
#endif

/* Type definition */
typedef int nl_item;

/*
   Constants for nl_langinfo()
 */

/* Include the common definitions */
#include <libraries/localestd.h>

/* Posix online defines */
#define D_T_FMT    0x20026  /* Date & time format string */
#define D_FMT      0x20027  /* Date format string */
#define T_FMT      0x20028  /* Time format string */
#define CODESET    0x2002B  /* Character encoding (e.g., UTF-8) */
#define CRNCYSTR   0x2002C  /* Currency symbol formatting */
#define RADIXCHAR  0x2002D  /* Decimal point string */
#define THOUSEP    0x2002E  /* Thousands separator string */
#define YESEXPR    0x2002F  /* Regex matching "yes" */
#define NOEXPR     0x20030  /* Regex matching "no" */

/* Function prototype */
char *nl_langinfo(nl_item item);
char *nl_langinfo_l(nl_item, locale_t);

#ifdef __cplusplus
}
#endif

#endif /* _LANGINFO_H */
