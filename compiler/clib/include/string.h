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

size_t strlen (const char * str);
int strcasecmp (const char * s1, const char * s2);
int stricmp (const char * s1, const char * s2);
int strcmp (const char * s1, const char * s2);
char * strdup (const char * str);
char * strcpy (char * dest, const char * src);

#endif /* _STRING_H */
