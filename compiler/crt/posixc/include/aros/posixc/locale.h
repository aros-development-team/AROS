#ifndef _POSIXC_LOCALE_H_
#define _POSIXC_LOCALE_H_

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    POSIX-2008.1 header file locale.h
*/

/* C99 */
#include <aros/stdc/locale.h>


#define LC_MESSAGES         6

#define _LC_LAST            7 /* marks end */

#define LC_COLLATE_MASK     (1 << LC_COLLATE)
#define LC_CTYPE_MASK       (1 << LC_CTYPE)
#define LC_MONETARY_MASK    (1 << LC_MONETARY)
#define LC_NUMERIC_MASK     (1 << LC_NUMERIC)
#define LC_TIME_MASK        (1 << LC_TIME)
#define LC_MESSAGES_MASK    (1 << LC_MESSAGES)
#define LC_ALL_MASK         (LC_COLLATE_MASK | LC_CTYPE_MASK | LC_MONETARY_MASK | LC_NUMERIC_MASK | LC_TIME_MASK | LC_MESSAGES_MASK)

__BEGIN_DECLS

/* NOTIMPL locale_t      duplocale(locale_t); */
/* NOTIMPL void          freelocale(locale_t); */
/* NOTIMPL locale_t      newlocale(int, const char *, locale_t); */
locale_t      uselocale (locale_t);

__END_DECLS

#endif /* _POSIXC_LOCALE_H_ */
