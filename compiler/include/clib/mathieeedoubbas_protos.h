#ifndef CLIB_MATHIEEEDOUBBAS_PROTOS_H
#define CLIB_MATHIEEEDOUBBAS_PROTOS_H

/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Prototypes for mathieeedpbas.library
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
AROS_LPQUAD1(QUAD, IEEEDPAbs,
    AROS_LPAQUAD(LONG, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 9, Mathieeedoubbas)

AROS_LPQUAD2(QUAD, IEEEDPAdd,
    AROS_LPAQUAD(QUAD, y, D0, D1),
    AROS_LPAQUAD(QUAD, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 11, Mathieeedoubbas)

AROS_LPQUAD1(QUAD, IEEEDPCeil,
    AROS_LPAQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 16, Mathieeedoubbas)

AROS_LPQUAD2(LONG, IEEEDPCmp,
    AROS_LPAQUAD(QUAD, y, D0, D1),
    AROS_LPAQUAD(QUAD, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 7, Mathieeedoubbas)

AROS_LPQUAD2(QUAD, IEEEDPDiv,
    AROS_LPAQUAD(QUAD, y, D0, D1),
    AROS_LPAQUAD(QUAD, z, D1, D2),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 14, Mathieeedoubbas)

AROS_LPQUAD1(LONG, IEEEDPFix,
    AROS_LPAQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 5, Mathieeedoubbas)

AROS_LPQUAD1(QUAD, IEEEDPFloor,
    AROS_LPAQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 15, Mathieeedoubbas)

AROS_LPQUAD1(QUAD, IEEEDPFlt,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 6, Mathieeedoubbas)

AROS_LPQUAD2(QUAD, IEEEDPMul,
    AROS_LPAQUAD(QUAD, y, D0, D1),
    AROS_LPAQUAD(QUAD, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 13, Mathieeedoubbas)

AROS_LPQUAD1(QUAD, IEEEDPNeg,
    AROS_LPAQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 10, Mathieeedoubbas)

AROS_LPQUAD2(QUAD, IEEEDPSub,
    AROS_LPAQUAD(QUAD, y, D0, D1),
    AROS_LPAQUAD(QUAD, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 12, Mathieeedoubbas)

AROS_LPQUAD1(LONG, IEEEDPTst,
    AROS_LPAQUAD(QUAD, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 8, Mathieeedoubbas)


#endif /* CLIB_MATHIEEEDOUBBAS_PROTOS_H */
