#ifndef CLIB_MATHIEEESPBAS_PROTOS_H
#define CLIB_MATHIEEESPBAS_PROTOS_H

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
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 9, Mathieeespbas)

AROS_LP2(LONG, IEEESPAdd,
    AROS_LPA(LONG, y, D0),
    AROS_LPA(LONG, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 11, Mathieeespbas)

AROS_LP1(LONG, IEEESPCeil,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 16, Mathieeespbas)

AROS_LP2(LONG, IEEESPCmp,
    AROS_LPA(LONG, y, D0),
    AROS_LPA(LONG, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 7, Mathieeespbas)

AROS_LP2(LONG, IEEESPDiv,
    AROS_LPA(LONG, y, D0),
    AROS_LPA(LONG, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 14, Mathieeespbas)

AROS_LP1(LONG, IEEESPFix,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 5, Mathieeespbas)

AROS_LP1(LONG, IEEESPFloor,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 15, Mathieeespbas)

AROS_LP1(LONG, IEEESPFlt,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 6, Mathieeespbas)

AROS_LP2(LONG, IEEESPMul,
    AROS_LPA(LONG, y, D0),
    AROS_LPA(LONG, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 13, Mathieeespbas)

AROS_LP1(LONG, IEEESPNeg,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 10, Mathieeespbas)

AROS_LP2(LONG, IEEESPSub,
    AROS_LPA(LONG, y, D0),
    AROS_LPA(LONG, z, D1),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 12, Mathieeespbas)

AROS_LP1(LONG, IEEESPTst,
    AROS_LPA(LONG, y, D0),
    struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 8, Mathieeespbas)


#endif /* CLIB_MATHIEEESPBAS_PROTOS_H */
