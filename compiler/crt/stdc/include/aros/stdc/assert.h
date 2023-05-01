#ifndef _STDC_ASSERT_H_
#define _STDC_ASSERT_H_

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
/* C99 conformance when NDEBUG is defined */
#define assert(expr)	((void)0)
#else
#define assert(expr)	(((expr)) ? (void)0 : __assert(#expr,__FILE__,__LINE__))
#endif

__BEGIN_DECLS

void __assert(const char *, const char *, unsigned int);

__END_DECLS

#endif /* _STDC_ASSERT_H_ */
