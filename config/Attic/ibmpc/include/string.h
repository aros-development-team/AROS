#ifndef _STRING_H
#define _STRING_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI-C header file string.h
    Lang: english
*/
#ifndef _SYS_TYPES_H
#   include <sys/types.h>
#endif

int strcmp (const char * s1, const char * s2);
char * strcpy (char * dest, const char * src);
void * memset (void * dest, int c, size_t n);

/* Macros */
#define stricmp  strcasecmp
#define strnicmp strncasecmp

/* Amiga native libraries need inline versions of string functions
 * because they arn't linked with libc.
 */
#ifdef _AMIGA
#include <inline/strsup.h>
#endif

#endif /* _STRING_H */
