#ifndef DEFINES_MATHFFP_H
#define DEFINES_MATHFFP_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/

#define SPAbs(FLOAT) \
    AROS_LC1(FLOAT, SPAbs, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct MathffpBase *, MathffpBase, 9, Mathffp)

#define SPAdd(FLOAT, FLOAT) \
    AROS_LC2(FLOAT, SPAdd, \
    AROS_LPA(FLOAT, fnum1, D1), \
    AROS_LPA(FLOAT, fnum2, D0), \
    struct MathffpBase *, MathffpBase, 11, Mathffp)

#define SPCeil(FLOAT) \
    AROS_LC1(FLOAT, SPCeil, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathffpBase *, MathffpBase, 16, Mathffp)

#define SPCmp(FLOAT, FLOAT) \
    AROS_LC2(FLOAT, SPCmp, \
    AROS_LPA(FLOAT, fnum1, D0), \
    AROS_LPA(FLOAT, fnum2, D1), \
    struct MathffpBase *, MathffpBase, 7, Mathffp)

#define SPDiv(FLOAT, FLOAT) \
    AROS_LC2(FLOAT, SPDiv, \
    AROS_LPA(FLOAT, fnum1, D1), \
    AROS_LPA(FLOAT, fnum2, D0), \
    struct MathffpBase *, MathffpBase, 14, Mathffp)

#define SPFix(FLOAT) \
    AROS_LC1(LONG, SPFix, \
    AROS_LPA(FLOAT, fnum, D0), \
    struct MathffpBase *, MathffpBase, 5, Mathffp)

#define SPFloor(FLOAT) \
    AROS_LC1(FLOAT, SPFloor, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathffpBase *, MathffpBase, 15, Mathffp)

#define SPFlt(LONG) \
    AROS_LC1(FLOAT, SPFlt, \
    AROS_LPA(LONG, inum, D0), \
    struct MathffpBase *, MathffpBase, 6, Mathffp)

#define SPMul(FLOAT, FLOAT) \
    AROS_LC2(FLOAT, SPMul, \
    AROS_LPA(FLOAT, fnum1, D1), \
    AROS_LPA(FLOAT, fnum2, D0), \
    struct MathffpBase *, MathffpBase, 13, Mathffp)

#define SPNeg(FLOAT) \
    AROS_LC1(FLOAT, SPNeg, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct MathffpBase *, MathffpBase, 10, Mathffp)

#define SPSub(FLOAT, FLOAT) \
    AROS_LC2(FLOAT, SPSub, \
    AROS_LPA(FLOAT, fnum1, D1), \
    AROS_LPA(FLOAT, fnum2, D0), \
    struct MathffpBase *, MathffpBase, 12, Mathffp)

#define SPTst(FLOAT) \
    AROS_LC1(LONG, SPTst, \
    AROS_LPA(FLOAT, fnum, D1), \
    struct MathffpBase *, MathffpBase, 8, Mathffp)

#endif /* DEFINES_MATHFFP_H */
