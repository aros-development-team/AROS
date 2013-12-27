#ifndef _STDC__STRINGS_H_
#define _STDC__STRINGS_H_

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file <strings.h>, with some extensions
          File is named _strings.h and included from strings.h.
          This allows string.h to include _strings.h without generating
          conflict with catcomp generated strings.h file.
*/
#include <aros/system.h>

#include <aros/types/size_t.h>

__BEGIN_DECLS

/* NOTIMPL int ffs(int); */
int strcasecmp(const char *, const char *);
/* NOTIMPL int strcasecmp_l(const char *, const char *, locale_t); */
int strncasecmp(const char *, const char *, size_t);
/* NOTIMPL int strncasecmp_l(const char *, const char *, size_t, locale_t); */

/* Some name space polution to implement legacy functions as
   inline functions
*/
void *memmove (void * dest, const void * src, size_t n);
int memcmp (const void * s1, const void * s2, size_t n);
void *memset (void * dest, int c, size_t n);
char *strchr (const char * s, int c);
char *strrchr (const char * s, int c);

/* Deprecated, removed in POSIX.1-2008 */
static __inline__ int bcmp(const void * s1, const void * s2, size_t n)
{
    return memcmp(s1, s2, n);
}

static __inline__ void bcopy(const void * src, void * dest, size_t n)
{
    memmove(dest, src, n);
}

static __inline__ void bzero(void * dest, size_t n)
{
    memset(dest, 0, n);
}

static __inline__ char *index(const char * s, int c)
{
    return strchr(s, c);
}

static __inline__ char *rindex(const char * s, int c)
{
    return strrchr(s, c);
}

/* BSD */
char *strcasestr(const char * str, const char * search);

/* Extension */
#define stricmp strcasecmp
#define strnicmp strncasecmp

__END_DECLS

#endif /* _STDC__STRINGS_H_ */
