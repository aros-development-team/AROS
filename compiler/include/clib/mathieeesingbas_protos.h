#ifndef CLIB_MATHIEEESINGBAS_PROTOS_H
#define CLIB_MATHIEEESINGBAS_PROTOS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for mathieeespbas.library
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
AROS_LP1(float, IEEESPAbs,
    AROS_LPA(float, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 9, Mathieeesingbas)

AROS_LP2(float, IEEESPAdd,
    AROS_LPA(float, y, D0),
    AROS_LPA(float, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 11, Mathieeesingbas)

AROS_LP1(float, IEEESPCeil,
    AROS_LPA(float, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 16, Mathieeesingbas)

AROS_LP2(LONG, IEEESPCmp,
    AROS_LPA(float, y, D0),
    AROS_LPA(float, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 7, Mathieeesingbas)

AROS_LP2(float, IEEESPDiv,
    AROS_LPA(float, y, D0),
    AROS_LPA(float, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 14, Mathieeesingbas)

AROS_LP1(LONG, IEEESPFix,
    AROS_LPA(float, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 5, Mathieeesingbas)

AROS_LP1(float, IEEESPFloor,
    AROS_LPA(float, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 15, Mathieeesingbas)

AROS_LP1(float, IEEESPFlt,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 6, Mathieeesingbas)

AROS_LP2(float, IEEESPMul,
    AROS_LPA(float, y, D0),
    AROS_LPA(float, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 13, Mathieeesingbas)

AROS_LP1(float, IEEESPNeg,
    AROS_LPA(float, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 10, Mathieeesingbas)

AROS_LP2(float, IEEESPSub,
    AROS_LPA(float, y, D0),
    AROS_LPA(float, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 12, Mathieeesingbas)

AROS_LP1(LONG, IEEESPTst,
    AROS_LPA(float, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 8, Mathieeesingbas)


#endif /* CLIB_MATHIEEESINGBAS_PROTOS_H */
