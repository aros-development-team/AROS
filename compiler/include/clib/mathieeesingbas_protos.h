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
AROS_LP1(LONG, IEEESPAbs,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 9, Mathieeesingbas)

AROS_LP2(LONG, IEEESPAdd,
    AROS_LPA(LONG, y, D0),
    AROS_LPA(LONG, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 11, Mathieeesingbas)

AROS_LP1(LONG, IEEESPCeil,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 16, Mathieeesingbas)

AROS_LP2(LONG, IEEESPCmp,
    AROS_LPA(LONG, y, D0),
    AROS_LPA(LONG, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 7, Mathieeesingbas)

AROS_LP2(LONG, IEEESPDiv,
    AROS_LPA(LONG, y, D0),
    AROS_LPA(LONG, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 14, Mathieeesingbas)

AROS_LP1(LONG, IEEESPFix,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 5, Mathieeesingbas)

AROS_LP1(LONG, IEEESPFloor,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 15, Mathieeesingbas)

AROS_LP1(LONG, IEEESPFlt,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 6, Mathieeesingbas)

AROS_LP2(LONG, IEEESPMul,
    AROS_LPA(LONG, y, D0),
    AROS_LPA(LONG, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 13, Mathieeesingbas)

AROS_LP1(LONG, IEEESPNeg,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 10, Mathieeesingbas)

AROS_LP2(LONG, IEEESPSub,
    AROS_LPA(LONG, y, D0),
    AROS_LPA(LONG, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 12, Mathieeesingbas)

AROS_LP1(LONG, IEEESPTst,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 8, Mathieeesingbas)


#endif /* CLIB_MATHIEEESINGBAS_PROTOS_H */
