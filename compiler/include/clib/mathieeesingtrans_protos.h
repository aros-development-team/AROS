#ifndef CLIB_MATHIEEESINGTRANS_PROTOS_H
#define CLIB_MATHIEEESINGTRANS_PROTOS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for mathieeesptrans.library
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP1(LONG, IEEESPCos,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 7, Mathieeesptrans)

AROS_LP1(LONG, IEEESPCosh,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 11, Mathieeesptrans)

AROS_LP1(LONG, IEEESPLog,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 14, Mathieeesptrans)

AROS_LP1(LONG, IEEESPLog10,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 21, Mathieeesptrans)

AROS_LP2(LONG, IEEESPPow,
    AROS_LPA(LONG, x, D1),
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 15, Mathieeesptrans)

AROS_LP1(LONG, IEEESPSin,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 6, Mathieeesptrans)

AROS_LP1(LONG, IEEESPSinh,
    AROS_LPA(LONG, y , D0),
    struct Library *, MathIeeeSingTransBase, 10, Mathieeesptrans)

AROS_LP1(LONG, IEEESPTan,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 8, Mathieeesptrans)

AROS_LP1(LONG, IEEESPTanh,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 12, Mathieeesptrans)


#endif /* CLIB_MATHIEEESINGTRANS_PROTOS_H */
