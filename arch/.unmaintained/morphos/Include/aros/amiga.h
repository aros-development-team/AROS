#pragma pack(2)
#ifndef AROS_AMIGA_H
#define AROS_AMIGA_H

/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Amiga-specific things
    Lang: english
*/

#if defined(__SASC) || defined(__GNUC__)
#   define HAS_STRING_H
#endif

#define __AROS_STRUCTURE_ALIGNMENT  4

/* To handle the slightly different procedure naming */
#define __AROS_SLIB_ENTRY(n,s)    s ## _ ## n
#define AROS_ASMSYMNAME(s)	(&s##_Gate)

#endif /* AROS_AMIGA_H */

#pragma pack()
