#ifndef AROS_FREEBSD_H
#define AROS_FREEBSD_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: FreeBSD specific things
    Lang: english
*/

#define HAS_STRING_H
#define __AROS_STRUCTURE_ALIGNMENT  8

/* To handle the slightly different procedure naming */
#define __AROS_SLIB_ENTRY(n,s)          s ## _ ## n
#define AROS_ASMSYMNAME(s)	      s

#endif /* AROS_FREEBSD_H */
