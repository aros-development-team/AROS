#ifndef CLIB_MATHIEEEDOUBTRANS_PROTOS_H
#define CLIB_MATHIEEEDOUBTRANS_PROTOS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for mathieeedptrans.library
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
AROS_LPQUAD1(QUAD, IEEEDPAcos,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 20, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPAsin,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 19, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPCos,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 7, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPCosh,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 11, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPExp,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 13, Mathieeedoubtrans)

AROS_LP1(QUAD, IEEEDPFieee,
    AROS_LPA(LONG, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 18, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPLog,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 14, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPLog10,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 21, Mathieeedoubtrans)

AROS_LPQUAD2(QUAD, IEEEDPPow,
    AROS_LPQUAD(QUAD, x, D2, D3),
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 15, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPSin,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 6, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPSinh,
    AROS_LPQUAD(QUAD, y , D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 10, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPSqrt,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 16, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPTan,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 8, Mathieeedoubtrans)

AROS_LPQUAD1(QUAD, IEEEDPTanh,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 12, Mathieeedoubtrans)

AROS_LPQUAD1(LONG, IEEEDPTieee,
    AROS_LPQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 17, Mathieeedoubtrans)

#endif /* CLIB_MATHIEEEDOUBTRANS_PROTOS_H */
