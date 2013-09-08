#ifndef _POSIXC_LOCALE_H_
#define _POSIXC_LOCALE_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX-2008.1 header file locale.h
*/

/* C99 */
#include <aros/stdc/locale.h>


#define LC_MESSAGES	    6
#define _LC_LAST	    7 /* marks end */


__BEGIN_DECLS

/* NOTIMPL locale_t      duplocale(locale_t); */
/* NOTIMPL void          freelocale(locale_t); */
/* NOTIMPL locale_t      newlocale(int, const char *, locale_t); */
/* NOTIMPL locale_t      uselocale (locale_t); */

__END_DECLS

#endif /* _POSIXC_LOCALE_H_ */
