#ifndef _ASSERT_H
#define _ASSERT_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI-C header file assert.h
    Lang: english
*/

#define assert(expr) \
    ((void)((expr) || __assert (#expr,__FILE__,__LINE__)))

extern void __assert (const char *, const char *, unsigned int);

#endif /* _ASSERT_H */
