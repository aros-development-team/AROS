#ifndef _ASSERT_H
#define _ASSERT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file assert.h
    Lang: english
*/

#ifdef assert
#undef assert
#endif

#ifdef NDEBUG
#define assert(expr)	/* eps */
#else
#define assert(expr)	((expr)) ? (void)0 : __assert(#expr,__FILE__,__LINE__)
#endif

extern void __assert (const char *, const char *, unsigned int);

#endif /* _ASSERT_H */
