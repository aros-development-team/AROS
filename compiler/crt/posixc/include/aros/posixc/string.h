#ifndef _POSIXC_STRING_H_
#define _POSIXC_STRING_H_

/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: C99/POSIX.1-2008 header file string.h
*/

#include <aros/posixc/locale.h>
#include <aros/stdc/string.h>

__BEGIN_DECLS

char * strerror (int n);
int strcoll_l(const char *s1, const char *s2, locale_t loc);
size_t strxfrm_l(char *dest, const char *src, size_t n, locale_t loc);
int strcoll(const char *s1, const char *s2);
int strerror_r(int errnum, char *buf, size_t buflen);

__END_DECLS

#endif /* _POSIXC_STRING_H_ */
