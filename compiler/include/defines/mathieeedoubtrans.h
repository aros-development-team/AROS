#ifndef DEFINES_MATHIEEEDOUBTRANS_H
#define DEFINES_MATHIEEEDOUBTRANS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/

#define IEEEDPAcos(y) \
    AROS_LCQUAD1(QUAD, IEEEDPAcos, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 20, Mathieeedoubtrans)

#define IEEEDPAsin(y) \
    AROS_LCQUAD1(QUAD, IEEEDPAsin, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 19, Mathieeedoubtrans)

#define IEEEDPCos(y) \
    AROS_LCQUAD1(QUAD, IEEEDPCos, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 7, Mathieeedoubtrans)

#define IEEEDPCosh(y) \
    AROS_LCQUAD1(QUAD, IEEEDPCosh, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 11, Mathieeedoubtrans)

#define IEEEDPExp(y) \
    AROS_LCQUAD1(QUAD, IEEEDPExp, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 13, Mathieeedoubtrans)

#define IEEEDPFieee(y) \
    AROS_LCQUAD1(QUAD, IEEEDPFieee, \
    AROS_LCAQUAD(LONG, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 18, Mathieeedoubtrans)

#define IEEEDPLog(y) \
    AROS_LCQUAD1(QUAD, IEEEDPLog, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 14, Mathieeedoubtrans)

#define IEEEDPLog10(y) \
    AROS_LCQUAD1(QUAD, IEEEDPLog10, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 21, Mathieeedoubtrans)

#define IEEEDPPow(x, y) \
    AROS_LC2(QUAD, IEEEDPPow, \
    AROS_LCAQUAD(QUAD, x, D2, D3), \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 15, Mathieeedoubtrans)

#define IEEEDPSin(y) \
    AROS_LCQUAD1(QUAD, IEEEDPSin, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 6, Mathieeedoubtrans)

#define IEEEDPSinh(y) \
    AROS_LCQUAD1(QUAD, IEEEDPSinh, \
    AROS_LCAQUAD(QUAD, y , D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 10, Mathieeedoubtrans)

#define IEEEDPSqrt(y) \
    AROS_LCQUAD1(QUAD, IEEEDPSqrt, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 16, Mathieeedoubtrans)

#define IEEEDPTan(y) \
    AROS_LCQUAD1(QUAD, IEEEDPTan, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 8, Mathieeedoubtrans)

#define IEEEDPTanh(y) \
    AROS_LCQUAD1(QUAD, IEEEDPTanh, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 12, Mathieeedoubtrans)

#define IEEEDPTieee(y) \
    AROS_LCQUAD1(LONG, IEEEDPTieee, \
    AROS_LCAQUAD(QUAD, y, D0, D1), \
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 17, Mathieeedoubtrans)


#endif /* DEFINES_MATHIEEEDOUBTRANS_H */
