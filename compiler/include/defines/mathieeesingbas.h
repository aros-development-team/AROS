#ifndef DEFINES_MATHIEEESINGBAS_H
#define DEFINES_MATHIEEESINGBAS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define IEEESPAbs(y) \
    AROS_LC1(LONG, IEEESPAbs, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 9, Mathieeesingbas)

#define IEEESPAdd(y, z) \
    AROS_LC2(LONG, IEEESPAdd, \
    AROS_LCA(LONG, y, D0), \
    AROS_LCA(LONG, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 11, Mathieeesingbas)

#define IEEESPCeil(y) \
    AROS_LC1(LONG, IEEESPCeil, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 16, Mathieeesingbas)

#define IEEESPCmp(y, z) \
    AROS_LC2(LONG, IEEESPCmp, \
    AROS_LCA(LONG, y, D0), \
    AROS_LCA(LONG, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 7, Mathieeesingbas)

#define IEEESPDiv(y, z) \
    AROS_LC2(LONG, IEEESPDiv, \
    AROS_LCA(LONG, y, D0), \
    AROS_LCA(LONG, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 14, Mathieeesingbas)

#define IEEESPFix(y) \
    AROS_LC1(LONG, IEEESPFix, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 5, Mathieeesingbas)

#define IEEESPFloor(y) \
    AROS_LC1(LONG, IEEESPFloor, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 15, Mathieeesingbas)

#define IEEESPFlt(y) \
    AROS_LC1(LONG, IEEESPFlt, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 6, Mathieeesingbas)

#define IEEESPMul(y, z) \
    AROS_LC2(LONG, IEEESPMul, \
    AROS_LCA(LONG, y, D0), \
    AROS_LCA(LONG, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 13, Mathieeesingbas)

#define IEEESPNeg(y) \
    AROS_LC1(LONG, IEEESPNeg, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 10, Mathieeesingbas)

#define IEEESPSub(y, z) \
    AROS_LC2(LONG, IEEESPSub, \
    AROS_LCA(LONG, y, D0), \
    AROS_LCA(LONG, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 12, Mathieeesingbas)

#define IEEESPTst(y) \
    AROS_LC1(LONG, IEEESPTst, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 8, Mathieeesingbas)


#endif /* DEFINES_MATHIEEESINGBAS_H */
