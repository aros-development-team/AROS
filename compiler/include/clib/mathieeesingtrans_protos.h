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
    struct Library *, MathIeeeSingTransBase, 7, Mathieeesingtrans)

AROS_LP1(LONG, IEEESPCosh,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 11, Mathieeesingtrans)

AROS_LP1(LONG, IEEESPExp,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 13, Mathieeesingtrans)

AROS_LP1(LONG, IEEESPFieee,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 18, Mathieeesingtrans)

AROS_LP1(LONG, IEEESPLog,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 14, Mathieeesingtrans)

AROS_LP1(LONG, IEEESPLog10,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 21, Mathieeesingtrans)

AROS_LP2(LONG, IEEESPPow,
    AROS_LPA(LONG, x, D1),
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 15, Mathieeesingtrans)

AROS_LP1(LONG, IEEESPSin,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 6, Mathieeesingtrans)

AROS_LP1(LONG, IEEESPSinh,
    AROS_LPA(LONG, y , D0),
    struct Library *, MathIeeeSingTransBase, 10, Mathieeesingtrans)

AROS_LP1(LONG, IEEESPTan,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 8, Mathieeesingtrans)

AROS_LP1(LONG, IEEESPTanh,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 12, Mathieeesingtrans)

AROS_LP1(LONG, IEEESPTieee,
    AROS_LPA(LONG, y, D0),
    struct Library *, MathIeeeSingTransBase, 17, Mathieeesingtrans)

#endif /* CLIB_MATHIEEESINGTRANS_PROTOS_H */
