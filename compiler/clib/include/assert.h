#ifndef _ASSERT_H_
#define _ASSERT_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file assert.h
    Lang: english
*/

#include <sys/cdefs.h>

#undef assert

#ifdef NDEBUG
#define assert(expr)	((void)0)
#else
#define assert(expr)	(((expr)) ? (void)0 : __assert(#expr,__FILE__,__LINE__))
#endif

__BEGIN_DECLS

extern void __assert (const char *, const char *, unsigned int);

__END_DECLS

#endif /* _ASSERT_H_ */
