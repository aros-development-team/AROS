#ifndef _ASSERT_H_
#define _ASSERT_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 header file assert.h
*/

#include <aros/system.h>

#ifdef assert
#undef assert
#endif

#ifdef NDEBUG
/* According to POSIX.1-2001, assert() must generate no code
 * if NDEBUG is defined.
 */
#define assert(expr)	((void)0)
#else
#define assert(expr)	(((expr)) ? (void)0 : __assert(#expr,__FILE__,__LINE__))
#endif

__BEGIN_DECLS

void __assert(const char *, const char *, unsigned int);

__END_DECLS

#endif /* _ASSERT_H_ */
