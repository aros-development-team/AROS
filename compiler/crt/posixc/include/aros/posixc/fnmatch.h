/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 header file fnmatch.h
*/

#ifndef _POSIXC_FNMATCH_H
#define _POSIXC_FNMATCH_H

#include <aros/system.h>

/* Return value of fnmatch() when the string does not match the pattern */
#define FNM_NOMATCH     1

/* Flags for fnmatch() */
#define FNM_NOESCAPE    (1 << 0)   /* Treat backslash as an ordinary character */
#define FNM_PATHNAME    (1 << 1)   /* Wildcards do not match '/' */
#define FNM_PERIOD      (1 << 2)   /* A leading period must be matched explicitly */

/* Commonly used extensions, also provided by other implementations */
#define FNM_LEADING_DIR (1 << 3)   /* Match a leading directory of the string */
#define FNM_CASEFOLD    (1 << 4)   /* Compare without regard to case */
#define FNM_FILE_NAME   FNM_PATHNAME

__BEGIN_DECLS

int fnmatch(const char *pattern, const char *string, int flags);

__END_DECLS

#endif /* _POSIXC_FNMATCH_H */
