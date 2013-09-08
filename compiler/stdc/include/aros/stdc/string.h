#ifndef _STDC_STRING_H_
#define _STDC_STRING_H_

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 & POSIX.1-2008 header file string.h with extra SAS/C and other extensions
    Also the POSIX.1-2008 extension to string.h will be included in
    stdc.library. This avoids opening of posixc.library by programs
    that only use few simple POSIX string functions.
*/
#include <aros/system.h>


/* C99 */
#include <aros/types/null.h>
#include <aros/types/size_t.h>

__BEGIN_DECLS

/* Copying functions */
void *memcpy (void * restrict dest, const void * restrict src, size_t n);
void *memmove (void * dest, const void * src, size_t n);
char *strcpy (char * restrict dest, const char * restrict src);
char *strncpy (char * restrict dest, const char * restrict src, size_t n);

/* Concatenation functions */
char *strcat (char * restrict dest, const char * restrict src);
char *strncat (char * restrict dest, const char * restrict src, size_t n);

/* Comparison functions */
int memcmp (const void * s1, const void * s2, size_t n);
int strcmp (const char * s1, const char * s2);
int strcoll (const char *s1, const char *s2);
int strncmp (const char * s1, const char * s2, size_t n);
size_t strxfrm (char * restrict s1, const char * restrict s2, size_t n);

/* Search functions */
void *memchr (const void * dest, int c, size_t n);
char *strchr (const char * s, int c);
size_t strcspn (const char *s1, const char *s2);
char *strpbrk (const char * s1, const char * s2);
char *strrchr (const char * s, int c);
size_t strspn (const char * s1, const char * s2);
char *strstr (const char * buf, const char * str);
char *strtok (char * str, const char * sep);

/* Miscellaneous functions */
void *memset (void * dest, int c, size_t n);
char *strerror (int n);
size_t strlen (const char * str);

/* AROS specific function */
char *__stdc_strerror(int n); /* This will be aliased to strerror() */

__END_DECLS


/* POSIX.1-2008 */

/* Code often includes string.h to get POSIX strings.h functions */
#include <_strings.h>
/* NOTIMPL locale_t */

__BEGIN_DECLS

void *memccpy(void *restrict, const void *restrict, int, size_t);
char *stpcpy(char *restrict, const char *restrict);
/* NOTIMPL char *stpncpy(char *restrict, const char *restrict, size_t); */
/* NOTIMPL int strcoll_l(const char *, const char *, locale_t); */
char *strdup(const char *);
/* NOTIMPL char *strerror_l(int, locale_t); */
/* NOTIMPL int strerror_r(int, char *, size_t); */
char *strndup(const char *, size_t);
/* NOTIMPL size_t strnlen(const char *, size_t); */
/* NOTIMPL char *strsignal(int); */
/* NOTIMPL char *strtok_r(char *restrict, const char *restrict, char **restrict); */
/* NOTIMPL size_t strxfrm_l(char *restrict, const char *restrict,
       size_t, locale_t); */

/* BSD/other UNIX */
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);
char *strsep(char **, const char *);
char *strlwr(char *);
char *strupr(char *);

/* SAS/C */
char * strrev(char * s);
int stch_l(const char * in, long * lvalue);
int stcd_l(const char * in, long * lvalue);
int stco_l(const char * in, long * lvalue);
int stcl_d(char * out, long lvalue);
int stcl_h(char * out, long lvalue);
int stcl_o(char * out, long lvalue);
int stcu_d(char * out, unsigned uivalue);

/* Supplemental (not std C) */
size_t stccpy(char *str1_ptr, const char *str2_ptr, size_t max);
char *stpsym(char *str_ptr, char *dest_ptr, int dest_size);
char * stpblk( const char * str );
#define stpbrk(s,cs) strpbrk(s,cs)
#define stpchr(s,c)  strchr(s,c)

__END_DECLS

#endif /* _STDC_STRING_H_ */
