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

#define IEEEIEEESPAbs(FLOAT) \
    AROS_LC1(FLOAT, IEEEIEEESPAbs, \
    AROS_LPA(FLOAT, y, D0), \
    struct Library *, MathIEEEIEEESPBasBase, 9, MathieeeIEEESPbas)

#define IEEESPAdd(FLOAT, FLOAT) \
    AROS_LC2(FLOAT, IEEESPAdd, \
    AROS_LPA(FLOAT, y, D0), \
    AROS_LPA(FLOAT, z, D1), \
    struct Library *, MathIEEEIEEESPBasBase, 11, MathieeeIEEESPbas)

#define IEEESPCeil(FLOAT) \
    AROS_LC1(FLOAT, IEEESPCeil, \
    AROS_LPA(FLOAT, y, D0), \
    struct Library *, MathIEEEIEEESPBasBase, 16, MathieeeIEEESPbas)

#define IEEESPCmp(FLOAT, FLOAT) \
    AROS_LC2(FLOAT, IEEESPCmp, \
    AROS_LPA(FLOAT, y, D0), \
    AROS_LPA(FLOAT, z, D1), \
    struct Library *, MathIEEEIEEESPBasBase, 7, MathieeeIEEESPbas)

#define IEEESPDiv(FLOAT, FLOAT) \
    AROS_LC2(FLOAT, IEEESPDiv, \
    AROS_LPA(FLOAT, y, D1), \
    AROS_LPA(FLOAT, z, D0), \
    struct Library *, MathIEEEIEEESPBasBase, 14, MathieeeIEEESPbas)

#define IEEESPFix(FLOAT) \
    AROS_LC1(LONG, IEEESPFix, \
    AROS_LPA(FLOAT, y, D0), \
    struct Library *, MathIEEEIEEESPBasBase, 5, MathieeeIEEESPbas)

#define IEEESPFloor(FLOAT) \
    AROS_LC1(FLOAT, IEEESPFloor, \
    AROS_LPA(FLOAT, y, D0), \
    struct Library *, MathIEEEIEEESPBasBase, 15, MathieeeIEEESPbas)

#define IEEESPFlt(LONG) \
    AROS_LC1(FLOAT, IEEESPFlt, \
    AROS_LPA(LONG, y, D0), \
    struct Library *, MathIEEEIEEESPBasBase, 6, MathieeeIEEESPbas)

#define IEEESPMul(FLOAT, FLOAT) \
    AROS_LC2(FLOAT, IEEESPMul, \
    AROS_LPA(FLOAT, y, D0), \
    AROS_LPA(FLOAT, z, D1), \
    struct Library *, MathIEEEIEEESPBasBase, 13, MathieeeIEEESPbas)

#define IEEESPNeg(FLOAT) \
    AROS_LC1(FLOAT, IEEESPNeg, \
    AROS_LPA(FLOAT, y, D0), \
    struct Library *, MathIEEEIEEESPBasBase, 10, MathieeeIEEESPbas)

#define IEEESPSub(FLOAT, FLOAT) \
    AROS_LC2(FLOAT, IEEESPSub, \
    AROS_LPA(FLOAT, y, D0), \
    AROS_LPA(FLOAT, z, D1), \
    struct Library *, MathIEEEIEEESPBasBase, 12, MathieeeIEEESPbas)

#define IEEESPTst(FLOAT) \
    AROS_LC1(LONG, IEEESPTst, \
    AROS_LPA(FLOAT, y, D0), \
    struct Library *, MathIEEEIEEESPBasBase, 8, MathieeeIEEESPbas)

#endif /* DEFINES_MathieeeIEEESPbas_H */



#endif /* DEFINES_MATHIEEESINGBAS_H */
