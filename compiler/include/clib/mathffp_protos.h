#ifndef CLIB_MATHFFP_PROTOS_H
#define CLIB_MATHFFP_PROTOS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for mathffp.library
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
AROS_LP1(float, SPAbs,
    AROS_LPA(float, fnum1, D0),
    struct MathBase *, MathBase, 9, Mathffp)

AROS_LP2(float, SPAdd,
    AROS_LPA(float, fnum1, D1),
    AROS_LPA(float, fnum2, D0),
    struct MathBase *, MathBase, 11, Mathffp)

AROS_LP1(float, SPCeil,
    AROS_LPA(float, y, D0),
    struct MathBase *, MathBase, 16, Mathffp)

AROS_LP2(LONG, SPCmp,
    AROS_LPA(float, fnum1, D0),
    AROS_LPA(float, fnum2, D1),
    struct MathBase *, MathBase, 7, Mathffp)

AROS_LP2(float, SPDiv,
    AROS_LPA(float, fnum1, D1),
    AROS_LPA(float, fnum2, D0),
    struct MathBase *, MathBase, 14, Mathffp)

AROS_LP1(LONG, SPFix,
    AROS_LPA(float, fnum, D0),
    struct MathBase *, MathBase, 5, Mathffp)

AROS_LP1(float, SPFloor,
    AROS_LPA(float, y, D0),
    struct MathBase *, MathBase, 15, Mathffp)

AROS_LP1(float, SPFlt,
    AROS_LPA(LONG, inum, D0),
    struct MathBase *, MathBase, 6, Mathffp)

AROS_LP2(float, SPMul,
    AROS_LPA(float, fnum1, D1),
    AROS_LPA(float, fnum2, D0),
    struct MathBase *, MathBase, 13, Mathffp)

AROS_LP1(float, SPNeg,
    AROS_LPA(float, fnum1, D0),
    struct MathBase *, MathBase, 10, Mathffp)

AROS_LP2(float, SPSub,
    AROS_LPA(float, fnum1, D0),
    AROS_LPA(float, fnum2, D1),
    struct MathBase *, MathBase, 12, Mathffp)

AROS_LP1(LONG, SPTst,
    AROS_LPA(float, fnum, D1),
    struct MathBase *, MathBase, 8, Mathffp)


#endif /* CLIB_MATHFFP_PROTOS_H */
