/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    C99 header file assert.h
*/

/*
    Per C99 7.2, assert is redefined on EVERY inclusion of <assert.h>,
    according to the current state of NDEBUG - so this part is
    deliberately outside the include guard.
*/
#ifdef assert
#undef assert
#endif

#ifdef NDEBUG
/* C99 conformance when NDEBUG is defined */
#define assert(expr)	((void)0)
#else
#define assert(expr)	(((expr)) ? (void)0 : __assert(#expr,__FILE__,__LINE__))
#endif

#ifndef _STDC_ASSERT_H_
#define _STDC_ASSERT_H_

#include <aros/system.h>

#if __STDC_VERSION__ >= 201112L
#ifndef static_assert
#define static_assert _Static_assert
#endif
#endif

__BEGIN_DECLS

void __assert(const char *, const char *, unsigned int);

__END_DECLS

#endif /* _STDC_ASSERT_H_ */
