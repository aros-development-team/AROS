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
AROS_LP1(LONG, IEEESPAcos,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 20, Mathieeesptrans)

AROS_LP1(LONG, IEEESPAsin,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 19, Mathieeesptrans)

AROS_LP1(LONG, IEEESPAtan,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 5, Mathieeesptrans)

AROS_LP1(LONG, IEEESPCos,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 7, Mathieeesptrans)

AROS_LP1(LONG, IEEESPCosh,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 11, Mathieeesptrans)

AROS_LP1(LONG, IEEESPExp,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 13, Mathieeesptrans)

AROS_LP1(LONG, IEEESPFieee,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 18, Mathieeesptrans)

AROS_LP1(LONG, IEEESPLog,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 14, Mathieeesptrans)

AROS_LP1(LONG, IEEESPLog10,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 21, Mathieeesptrans)

AROS_LP2(LONG, IEEESPPow,
    AROS_LPA(LONG, x, D1),
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 15, Mathieeesptrans)

AROS_LP1(LONG, IEEESPSin,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 6, Mathieeesptrans)

AROS_LP2(LONG, IEEESPSincos,
    AROS_LPA(LONG, z, a0),
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 9, Mathieeesptrans)

AROS_LP1(LONG, IEEESPSinh,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 10, Mathieeesptrans)

AROS_LP1(LONG, IEEESPSqrt,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 16, Mathieeesptrans)

AROS_LP1(LONG, IEEESPTan,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 8, Mathieeesptrans)

AROS_LP1(LONG, IEEESPTanh,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 12, Mathieeesptrans)

AROS_LP1(LONG, IEEESPTieee,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 17, Mathieeesptrans)

#endif /* CLIB_MATHIEEESINGTRANS_PROTOS_H */
