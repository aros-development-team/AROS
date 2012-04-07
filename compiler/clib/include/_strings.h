#ifndef __STRINGS_H_
#define __STRINGS_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file <strings.h>, with some extesions
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

/* Deprecated, removed in POSIX.1-2008 */
int bcmp(const void *, const void *, size_t);
void bcopy(const void *, void *, size_t);
void bzero(void *, size_t);
char *index(const char * s, int c);
char *rindex(const char * s, int c);

/* BSD */
char *strcasestr(const char * str, const char * search);

/* Extension */
#define stricmp strcasecmp
#define strnicmp strncasecmp

__END_DECLS

#endif /* __STRINGS_H_ */
