#ifndef AROS_64BIT_H
#define AROS_64BIT_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1999/02/12 13:57:59  schulz
    New includes for ibmpc.

    Revision 1.1  1997/03/27 01:13:51  ldp
    libaros.a -> libarossupport.a

    Revision 1.1  1997/01/09 18:21:14  digulla
    Accidentially deleted :-( Shouldn't work when it's getting late

    Revision 1.1  1996/09/13 17:54:34  digulla
    Overworked the way systems are recognised and added three sample systems


    Desc: Work on 64bit data types
    Lang: english
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#if defined(AROS_64BIT_TYPE) || defined(__GNUC__)
#   define LOW32OF64(val64)     ((val64) & 0xFFFFFFFF)
#   define HIGH32OF64(val64)    ((val64) >> 32L)
#else
#   define LOW32OF64(val64)     ((val64).low)
#   define HIGH32OF64(val64)    ((val64).high)
#endif

#endif /* AROS_64BIT_H */
