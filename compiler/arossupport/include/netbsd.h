#ifndef AROS_NETBSD_H
#define AROS_NETBSD_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NetBSD specific hings
    Lang: english
*/

#define HAS_STRING_H
#define __AROS_STRUCTURE_ALIGNMENT 8 

/* To handle the slightly different procedure naming */
#define __AROS_SLIB_ENTRY(n,s)    s ## _ ## n
#define AROS_ASMSYMNAME(s)      s

#endif /* AROS_NETBSD_H */
