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
    AROS_LC1(FLOAT, IEEESPAbs, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 9, Mathieeespbas)

#define IEEESPAdd(y, z) \
    AROS_LC2(FLOAT, IEEESPAdd, \
    AROS_LPA(FLOAT, y, D0), \
    AROS_LPA(FLOAT, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 11, Mathieeespbas)

#define IEEESPCeil(y) \
    AROS_LC1(FLOAT, IEEESPCeil, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 16, Mathieeespbas)

#define IEEESPCmp(y, z) \
    AROS_LC2(FLOAT, IEEESPCmp, \
    AROS_LPA(FLOAT, y, D0), \
    AROS_LPA(FLOAT, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 7, Mathieeespbas)

#define IEEESPDiv(y, z) \
    AROS_LC2(FLOAT, IEEESPDiv, \
    AROS_LPA(FLOAT, y, D1), \
    AROS_LPA(FLOAT, z, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 14, Mathieeespbas)

#define IEEESPFix(y) \
    AROS_LC1(LONG, IEEESPFix, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 5, Mathieeespbas)

#define IEEESPFloor(y) \
    AROS_LC1(FLOAT, IEEESPFloor, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 15, Mathieeespbas)

#define IEEESPFlt(y) \
    AROS_LC1(FLOAT, IEEESPFlt, \
    AROS_LPA(LONG, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 6, Mathieeespbas)

#define IEEESPMul(y, z) \
    AROS_LC2(FLOAT, IEEESPMul, \
    AROS_LPA(FLOAT, y, D0), \
    AROS_LPA(FLOAT, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 13, Mathieeespbas)

#define IEEESPNeg(y) \
    AROS_LC1(FLOAT, IEEESPNeg, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 10, Mathieeespbas)

#define IEEESPSub(y, z) \
    AROS_LC2(FLOAT, IEEESPSub, \
    AROS_LPA(FLOAT, y, D0), \
    AROS_LPA(FLOAT, z, D1), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 12, Mathieeespbas)

#define IEEESPTst(y) \
    AROS_LC1(LONG, IEEESPTst, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 8, Mathieeespbas)


#endif /* DEFINES_MATHIEEESINGBAS_H */
