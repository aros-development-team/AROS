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
    AROS_LC1(LONG, SPAbs, \
    AROS_LCA(LONG, fnum1, D0), \
    struct MathffpBase *, MathBase, 9, Mathffp)

#define SPAdd(fnum1, fnum2) \
    AROS_LC2(LONG, SPAdd, \
    AROS_LCA(LONG, fnum1, D1), \
    AROS_LCA(LONG, fnum2, D0), \
    struct MathffpBase *, MathBase, 11, Mathffp)

#define SPCeil(y) \
    AROS_LC1(LONG, SPCeil, \
    AROS_LCA(LONG, y, D0), \
    struct MathffpBase *, MathBase, 16, Mathffp)

#define SPCmp(fnum1, fnum2) \
    AROS_LC2(LONG, SPCmp, \
    AROS_LCA(LONG, fnum1, D0), \
    AROS_LCA(LONG, fnum2, D1), \
    struct MathffpBase *, MathBase, 7, Mathffp)

#define SPDiv(fnum1, fnum2) \
    AROS_LC2(LONG, SPDiv, \
    AROS_LCA(LONG, fnum1, D1), \
    AROS_LCA(LONG, fnum2, D0), \
    struct MathffpBase *, MathBase, 14, Mathffp)

#define SPFix(fnum) \
    AROS_LC1(LONG, SPFix, \
    AROS_LCA(LONG, fnum, D0), \
    struct MathffpBase *, MathBase, 5, Mathffp)

#define SPFloor(y) \
    AROS_LC1(LONG, SPFloor, \
    AROS_LCA(LONG, y, D0), \
    struct MathffpBase *, MathBase, 15, Mathffp)

#define SPFlt(inum) \
    AROS_LC1(LONG, SPFlt, \
    AROS_LCA(LONG, inum, D0), \
    struct MathffpBase *, MathBase, 6, Mathffp)

#define SPMul(fnum1, fnum2) \
    AROS_LC2(LONG, SPMul, \
    AROS_LCA(LONG, fnum1, D1), \
    AROS_LCA(LONG, fnum2, D0), \
    struct MathffpBase *, MathBase, 13, Mathffp)

#define SPNeg(fnum1) \
    AROS_LC1(LONG, SPNeg, \
    AROS_LCA(LONG, fnum1, D0), \
    struct MathffpBase *, MathBase, 10, Mathffp)

#define SPSub(fnum1, fnum2) \
    AROS_LC2(LONG, SPSub, \
    AROS_LCA(LONG, fnum1, D0), \
    AROS_LCA(LONG, fnum2, D1), \
    struct MathffpBase *, MathBase, 12, Mathffp)

#define SPTst(fnum) \
    AROS_LC1(LONG, SPTst, \
    AROS_LCA(LONG, fnum, D1), \
    struct MathffpBase *, MathBase, 8, Mathffp)


#endif /* DEFINES_MATHFFP_H */
