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
#define SPAbs(fnum1) \
    AROS_LC1(float, SPAbs, \
    AROS_LCA(float, fnum1, D0), \
    struct MathBase *, MathBase, 9, Mathffp)

#define SPAdd(fnum1, fnum2) \
    AROS_LC2(float, SPAdd, \
    AROS_LCA(float, fnum1, D1), \
    AROS_LCA(float, fnum2, D0), \
    struct MathBase *, MathBase, 11, Mathffp)

#define SPCeil(y) \
    AROS_LC1(float, SPCeil, \
    AROS_LCA(float, y, D0), \
    struct MathBase *, MathBase, 16, Mathffp)

#define SPCmp(fnum1, fnum2) \
    AROS_LC2(LONG, SPCmp, \
    AROS_LCA(float, fnum1, D0), \
    AROS_LCA(float, fnum2, D1), \
    struct MathBase *, MathBase, 7, Mathffp)

#define SPDiv(fnum1, fnum2) \
    AROS_LC2(float, SPDiv, \
    AROS_LCA(float, fnum1, D1), \
    AROS_LCA(float, fnum2, D0), \
    struct MathBase *, MathBase, 14, Mathffp)

#define SPFix(fnum) \
    AROS_LC1(LONG, SPFix, \
    AROS_LCA(float, fnum, D0), \
    struct MathBase *, MathBase, 5, Mathffp)

#define SPFloor(y) \
    AROS_LC1(float, SPFloor, \
    AROS_LCA(float, y, D0), \
    struct MathBase *, MathBase, 15, Mathffp)

#define SPFlt(inum) \
    AROS_LC1(float, SPFlt, \
    AROS_LCA(LONG, inum, D0), \
    struct MathBase *, MathBase, 6, Mathffp)

#define SPMul(fnum1, fnum2) \
    AROS_LC2(float, SPMul, \
    AROS_LCA(float, fnum1, D1), \
    AROS_LCA(float, fnum2, D0), \
    struct MathBase *, MathBase, 13, Mathffp)

#define SPNeg(fnum1) \
    AROS_LC1(float, SPNeg, \
    AROS_LCA(float, fnum1, D0), \
    struct MathBase *, MathBase, 10, Mathffp)

#define SPSub(fnum1, fnum2) \
    AROS_LC2(float, SPSub, \
    AROS_LCA(float, fnum1, D0), \
    AROS_LCA(float, fnum2, D1), \
    struct MathBase *, MathBase, 12, Mathffp)

#define SPTst(fnum) \
    AROS_LC1(LONG, SPTst, \
    AROS_LCA(float, fnum, D1), \
    struct MathBase *, MathBase, 8, Mathffp)


#endif /* DEFINES_MATHFFP_H */
