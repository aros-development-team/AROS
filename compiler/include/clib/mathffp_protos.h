#ifndef CLIB_MATHFFP_PROTOS_H
#define CLIB_MATHFFP_PROTOS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for mathffp.library
    Lang: english
*/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

AROS_LP1(FLOAT, SPAbs,
    AROS_LPA(FLOAT, fnum1, D0),
    struct MathffpBase *, MathffpBase, 9, Mathffp)

AROS_LP2(FLOAT, SPAdd,
    AROS_LPA(FLOAT, fnum1, D1),
    AROS_LPA(FLOAT, fnum2, D0),
    struct MathffpBase *, MathffpBase, 11, Mathffp)

AROS_LP1(FLOAT, SPCeil,
    AROS_LPA(FLOAT, y, D0),
    struct MathffpBase *, MathffpBase, 16, Mathffp)

AROS_LP2(FLOAT, SPCmp,
    AROS_LPA(FLOAT, fnum1, D0),
    AROS_LPA(FLOAT, fnum2, D1),
    struct MathffpBase *, MathffpBase, 7, Mathffp)

AROS_LP2(FLOAT, SPDiv,
    AROS_LPA(FLOAT, fnum1, D1),
    AROS_LPA(FLOAT, fnum2, D0),
    struct MathffpBase *, MathffpBase, 14, Mathffp)

AROS_LP1(LONG, SPFix,
    AROS_LPA(FLOAT, fnum, D0),
    struct MathffpBase *, MathffpBase, 5, Mathffp)

AROS_LP1(FLOAT, SPFloor,
    AROS_LPA(FLOAT, y, D0),
    struct MathffpBase *, MathffpBase, 15, Mathffp)

AROS_LP1(FLOAT, SPFlt,
    AROS_LPA(LONG, inum, D0),
    struct MathffpBase *, MathffpBase, 6, Mathffp)

AROS_LP2(FLOAT, SPMul,
    AROS_LPA(FLOAT, fnum1, D1),
    AROS_LPA(FLOAT, fnum2, D0),
    struct MathffpBase *, MathffpBase, 13, Mathffp)

AROS_LP1(FLOAT, SPNeg,
    AROS_LPA(FLOAT, fnum1, D0),
    struct MathffpBase *, MathffpBase, 10, Mathffp)

AROS_LP2(FLOAT, SPSub,
    AROS_LPA(FLOAT, fnum1, D1),
    AROS_LPA(FLOAT, fnum2, D0),
    struct MathffpBase *, MathffpBase, 12, Mathffp)

AROS_LP1(LONG, SPTst,
    AROS_LPA(FLOAT, fnum, D1),
    struct MathffpBase *, MathffpBase, 8, Mathffp)

#endif /* CLIB_MATHFFP_PROTOS_H */
