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
AROS_LP1(float, IEEESPAcos,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 20, Mathieeesingtrans)

AROS_LP1(float, IEEESPAsin,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 19, Mathieeesingtrans)

AROS_LP1(float, IEEESPCos,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 7, Mathieeesingtrans)

AROS_LP1(float, IEEESPCosh,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 11, Mathieeesingtrans)

AROS_LP1(float, IEEESPExp,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 13, Mathieeesingtrans)

AROS_LP1(float, IEEESPFieee,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 18, Mathieeesingtrans)

AROS_LP1(float, IEEESPLog,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 14, Mathieeesingtrans)

AROS_LP1(float, IEEESPLog10,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 21, Mathieeesingtrans)

AROS_LP2(float, IEEESPPow,
    AROS_LPA(float, x, D1),
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 15, Mathieeesingtrans)

AROS_LP1(float, IEEESPSin,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 6, Mathieeesingtrans)

AROS_LP1(float, IEEESPSinh,
    AROS_LPA(float, y , D0),
    struct Library *, MathIeeeSingTransBase, 10, Mathieeesingtrans)

AROS_LP1(float, IEEESPSqrt,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 16, Mathieeesingtrans)

AROS_LP1(float, IEEESPTan,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 8, Mathieeesingtrans)

AROS_LP1(float, IEEESPTanh,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 12, Mathieeesingtrans)

AROS_LP1(float, IEEESPTieee,
    AROS_LPA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 17, Mathieeesingtrans)

#endif /* CLIB_MATHIEEESINGTRANS_PROTOS_H */
