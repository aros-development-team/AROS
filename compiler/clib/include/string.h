#ifndef _STRING_H_
#define _STRING_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file string.h
    Lang: english
*/
#include <aros/systypes.h>
#include <sys/_posix.h>

/* string.h is not allowed to include <sys/types.h> */
#ifdef _AROS_SIZE_T_
typedef _AROS_SIZE_T_	size_t;
#undef	_AROS_SIZE_T_
#endif

#ifndef NULL
#define NULL	    0
#endif

__BEGIN_DECLS

size_t strlen (const char * str);
int    strcasecmp (const char * s1, const char * s2);
int    strcmp (const char * s1, const char * s2);
int    strncasecmp (const char *str1, const char * str2, size_t n);
int    strncmp (const char * s1, const char * s2, size_t n);
char * strdup (const char * str);
char * strcpy (char * restrict dest, const char * restrict src);
char * strcat (char * restrict dest, const char * restrict src);
size_t strcspn (const char *s1, const char *s2);
char * strerror (size_t n);
char * strncat (char * restrict dest, const char * restrict src, size_t n);
char * strncpy (char * restrict dest, const char * restrict src, size_t n);
char * strpbrk (const char * s1, const char * s2);
char * strchr (const char * s, int c);
char * strrchr (const char * s, int c);
size_t strspn (const char * s1, const char * s2);
char * strstr (const char * buf, const char * str);
char * strtok (char * str, const char * sep);
char * strtok_r (char * str, const char * sep, char ** last);
char * stpcpy (char * dest, const char * src);

int    strcoll (const char *s1, const char *s2);

size_t strxfrm (char * restrict s1, const char * restrict s2, size_t n);

void * memchr (const void * dest, int c, size_t n);
int    memcmp (const void * s1, const void * s2, size_t n);
void * memcpy (void * restrict dest, const void * restrict src, size_t n);
void * memmove (void * dest, const void * src, size_t n);
void * memset (void * dest, int c, size_t n);

/* Supplemental (not ANSI C) */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
size_t stccpy(char *str1_ptr, const char *str2_ptr, size_t max);
char *stpsym(char *str_ptr, char *dest_ptr, int dest_size);

#ifdef __STDC__
#    define index(s, c)  strchr(s, c)
#    define rindex(s, c) strrchr(s, c)
#else
    static char * index (const char * s, int c)  { return strchr(s, c); }
    static char * rindex (const char * s, int c) { return strrchr(s, c); }
#endif

char * stpblk( const char * str );   /* Supplemental (not ANSI C) */

#define stpbrk(s,cs) strpbrk(s,cs)   /* Supplemental (not ANSI C) */
#define stpchr(s,c)  strchr(s,c)     /* Supplemental (not ANSI C) */

char * strrev(char * s); /* SAS C */
int stch_l(const char * in, long * lvalue); /* SAS C */
int stcu_d(char * out, unsigned uivalue); /* SAS C */

void * memccpy (void * restrict dest, const void * restrict src,
		    int c, size_t n);

int bcmp (const void * s1, const void * s2, int n);
void bcopy (const void * src, void * dst, int n);
void bzero (void * src, int n);

/* Macros */
#define stricmp  strcasecmp
#define strnicmp strncasecmp

#endif /* !_ANSI_SOURCE && !_POSIX_SOURCE */

__END_DECLS

/* Amiga native libraries need inline versions of string functions
 * because they arn't linked with libc.
 */
#ifdef _AMIGA
#include <inline/strsup.h>
#endif

#endif /* _STRING_H_ */
