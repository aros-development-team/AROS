#ifndef DEFINES_MATHIEEEDOUBBAS_H
#define DEFINES_MATHIEEEDOUBBAS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define IEEEDPAbs(y) \
    AROS_LCQUAD1(QUAD, IEEEDPAbs, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 9, Mathieeedoubbas)

#define IEEEDPAdd(y, z) \
    AROS_LCQUAD2(QUAD, IEEEDPAdd, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    AROS_LCAQUAD(QUAD, z, D2, D3), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 11, Mathieeedoubbas)

#define IEEEDPCeil(y) \
    AROS_LCQUAD1(QUAD, IEEEDPCeil, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 16, Mathieeedoubbas)

#define IEEEDPCmp(y, z) \
    AROS_LCQUAD2(QUAD, IEEEDPCmp, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    AROS_LCAQUAD(QUAD, z, D2, D3), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 7, Mathieeedoubbas)

#define IEEEDPDiv(y, z) \
    AROS_LCQUAD2(QUAD, IEEEDPDiv, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    AROS_LCAQUAD(QUAD, z, D2, D3), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 14, Mathieeedoubbas)

#define IEEEDPFix(y) \
    AROS_LCQUAD1(QUAD, IEEEDPFix, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 5, Mathieeedoubbas)

#define IEEEDPFloor(y) \
    AROS_LCQUAD1(QUAD, IEEEDPFloor, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 15, Mathieeedoubbas)

#define IEEEDPFlt(y) \
    AROS_LCQUAD1(QUAD, IEEEDPFlt, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 6, Mathieeedoubbas)

#define IEEEDPMul(y, z) \
    AROS_LCQUAD2(QUAD, IEEEDPMul, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    AROS_LCAQUAD(QUAD, z, D2, D3), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 13, Mathieeedoubbas)

#define IEEEDPNeg(y) \
    AROS_LCQUAD1(QUAD, IEEEDPNeg, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 10, Mathieeedoubbas)

#define IEEEDPSub(y, z) \
    AROS_LCQUAD2(QUAD, IEEEDPSub, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    AROS_LCAQUAD(QUAD, z, D2, D3), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 12, Mathieeedoubbas)

#define IEEEDPTst(y) \
    AROS_LCQUAD1(QUAD, IEEEDPTst, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 8, Mathieeedoubbas)


#endif /* DEFINES_MATHIEEEDOUBBAS_H */
