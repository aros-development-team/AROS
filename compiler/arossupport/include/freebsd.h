#ifndef AROS_FREEBSD_H
#define AROS_FREEBSD_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1997/03/27 01:13:52  ldp
    libaros.a -> libarossupport.a

    Revision 1.1  1997/01/09 18:21:16  digulla
    Accidentially deleted :-( Shouldn't work when it's getting late

    Revision 1.3  1996/11/25 02:33:54  aros
    Changed for object files in a.out format to work again.

    Revision 1.2  1996/11/16 12:05:27  aros
    Overdue... changed __AROS_SLIB_ENTRY to AROS_SLIB_ENTRY.

    Revision 1.1  1996/10/14 02:04:45  iaint
    Added aros/freebsd.h for FreeBSD.


    Desc: FreeBSD specific things
    Lang: english
*/

#define HAS_STRING_H
#define __AROS_STRUCTURE_ALIGNMENT  8

/* To handle the slightly different procedure naming */
#define AROS_SLIB_ENTRY(n,s)          s##_##n
#define AROS_ASMSYMNAME(s)	      s


#endif /* AROS_FREEBSD_H */
