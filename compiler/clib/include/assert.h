#ifndef _ASSERT_H
#define _ASSERT_H

/*
    Copyright (C) 1995-1997 AROS - The Amiga Research OS
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
