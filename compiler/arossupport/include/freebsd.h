#ifndef AROS_FREEBSD_H
#define AROS_FREEBSD_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: FreeBSD specific things
    Lang: english
*/

#define HAS_STRING_H
#define __AROS_STRUCTURE_ALIGNMENT  8

/*
    To handle the slightly different procedure naming under different
    version of the FreeBSD compiler. Thankfully we have the very helpful
    preprocessor symbol __ELF__ defined for us. 

    If we don't have these, then we can use the defaults.
*/
#if !defined(__ELF__)
#define __AROS_SLIB_ENTRY(n,s)          s ## _ ## n
#define AROS_ASMSYMNAME(s)	      s
#endif

#endif /* AROS_FREEBSD_H */
