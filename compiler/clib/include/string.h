#ifndef _STRING_H
#define _STRING_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file string.h
    Lang: english
*/

#ifndef _SYS_TYPES_H
#   include <sys/types.h>
#endif

size_t strlen (const char * str);
int    strcasecmp (const char * s1, const char * s2);
int    strcmp (const char * s1, const char * s2);
int    strncasecmp (const char *str1, const char * str2, size_t n);
int    strncmp (const char * s1, const char * s2, size_t n);
char * strdup (const char * str);
char * strcpy (char * dest, const char * src);
char * strcat (char * dest, const char * src);
size_t strcspn (const char *s1, const char *s2);
char * strerror (size_t n);
char * strncat (char * dest, const char * src, size_t n);
char * strncpy (char * dest, const char * src, size_t n);
char * strpbrk (const char * s1, const char * s2);
char * strchr (const char * s, int c);
char * strrchr (const char * s, int c);
size_t strspn (const char * s1, const char * s2);
char * strstr (const char * buf, const char * str);
char * strtok (char * s1, const char * s2);
char * stpcpy (char * dest, const char * src);

/* Supplemental (not ANSI C) */
size_t stccpy(char *str1_ptr, const char *str2_ptr, size_t max);
char *stpsym(char *str_ptr, char *dest_ptr, int dest_size);


#define index(s, c)  strchr(s, c)
#define rindex(s, c) strrchr(s, c)

char * stpblk( const char * str );   /* Supplemental (not ANSI C) */

#define stpbrk(s,cs) strpbrk(s,cs)   /* Supplemental (not ANSI C) */
#define stpchr(s,c)  strchr(s,c)     /* Supplemental (not ANSI C) */

char * strrev(char * s); /* SAS C */
int stch_l(const char * in, long * lvalue); /* SAS C */
int stcu_d(char * out, unsigned uivalue); /* SAS C */

void * memchr (const void * dest, int c, size_t n);
int memcmp (const void * s1, const void * s2, size_t n);
void * memcpy (void * dest, const void * src, size_t n);
void * memmove (void * dest, const void * src, size_t n);
void * memset (void * dest, int c, size_t n);
int bcmp (const void * s1, const void * s2, int n);
void bcopy (const void * src, void * dst, int n);
void bzero (void * src, int n);

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
