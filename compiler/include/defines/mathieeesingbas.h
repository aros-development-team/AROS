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
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 9, Mathieeespbas)

#define IEEESPAdd(y, z) \
    AROS_LC2(LONG, IEEESPAdd, \
    AROS_LCA(LONG, y, D0), \
    AROS_LCA(LONG, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 11, Mathieeespbas)

#define IEEESPCeil(y) \
    AROS_LC1(LONG, IEEESPCeil, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 16, Mathieeespbas)

#define IEEESPCmp(y, z) \
    AROS_LC2(LONG, IEEESPCmp, \
    AROS_LCA(LONG, y, D0), \
    AROS_LCA(LONG, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 7, Mathieeespbas)

#define IEEESPDiv(y, z) \
    AROS_LC2(LONG, IEEESPDiv, \
    AROS_LCA(LONG, y, D0), \
    AROS_LCA(LONG, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 14, Mathieeespbas)

#define IEEESPFix(y) \
    AROS_LC1(LONG, IEEESPFix, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 5, Mathieeespbas)

#define IEEESPFloor(y) \
    AROS_LC1(LONG, IEEESPFloor, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 15, Mathieeespbas)

#define IEEESPFlt(y) \
    AROS_LC1(LONG, IEEESPFlt, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 6, Mathieeespbas)

#define IEEESPMul(y, z) \
    AROS_LC2(LONG, IEEESPMul, \
    AROS_LCA(LONG, y, D0), \
    AROS_LCA(LONG, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 13, Mathieeespbas)

#define IEEESPNeg(y) \
    AROS_LC1(LONG, IEEESPNeg, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 10, Mathieeespbas)

#define IEEESPSub(y, z) \
    AROS_LC2(LONG, IEEESPSub, \
    AROS_LCA(LONG, y, D0), \
    AROS_LCA(LONG, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 12, Mathieeespbas)

#define IEEESPTst(y) \
    AROS_LC1(LONG, IEEESPTst, \
    AROS_LCA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 8, Mathieeespbas)


#endif /* DEFINES_MATHIEEESINGBAS_H */
