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
AROS_LP1(LONG, SPAbs,
    AROS_LPA(LONG, fnum1, D0),
    struct MathffpBase *, MathBase, 9, Mathffp)

AROS_LP2(LONG, SPAdd,
    AROS_LPA(LONG, fnum1, D1),
    AROS_LPA(LONG, fnum2, D0),
    struct MathffpBase *, MathBase, 11, Mathffp)

AROS_LP1(LONG, SPCeil,
    AROS_LPA(LONG, y, D0),
    struct MathffpBase *, MathBase, 16, Mathffp)

AROS_LP2(LONG, SPCmp,
    AROS_LPA(LONG, fnum1, D0),
    AROS_LPA(LONG, fnum2, D1),
    struct MathffpBase *, MathBase, 7, Mathffp)

AROS_LP2(LONG, SPDiv,
    AROS_LPA(LONG, fnum1, D1),
    AROS_LPA(LONG, fnum2, D0),
    struct MathffpBase *, MathBase, 14, Mathffp)

AROS_LP1(LONG, SPFix,
    AROS_LPA(LONG, fnum, D0),
    struct MathffpBase *, MathBase, 5, Mathffp)

AROS_LP1(LONG, SPFloor,
    AROS_LPA(LONG, y, D0),
    struct MathffpBase *, MathBase, 15, Mathffp)

AROS_LP1(LONG, SPFlt,
    AROS_LPA(LONG, inum, D0),
    struct MathffpBase *, MathBase, 6, Mathffp)

AROS_LP2(LONG, SPMul,
    AROS_LPA(LONG, fnum1, D1),
    AROS_LPA(LONG, fnum2, D0),
    struct MathffpBase *, MathBase, 13, Mathffp)

AROS_LP1(LONG, SPNeg,
    AROS_LPA(LONG, fnum1, D0),
    struct MathffpBase *, MathBase, 10, Mathffp)

AROS_LP2(LONG, SPSub,
    AROS_LPA(LONG, fnum1, D0),
    AROS_LPA(LONG, fnum2, D1),
    struct MathffpBase *, MathBase, 12, Mathffp)

AROS_LP1(LONG, SPTst,
    AROS_LPA(LONG, fnum, D1),
    struct MathffpBase *, MathBase, 8, Mathffp)

#endif /* CLIB_MATHFFP_PROTOS_H */
