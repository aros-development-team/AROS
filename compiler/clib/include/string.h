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
int strncmp (const char * s1, const char * s2);
char * strdup (const char * str);
char * strcpy (char * dest, const char * src);

void *memchr(const void *dest, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *dest, int c, size_t n);
void bcopy (const void *src, void *dst, size_t len);

#endif /* _STRING_H */
