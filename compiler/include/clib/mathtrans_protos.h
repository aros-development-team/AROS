#ifndef CLIB_MATHTRANS_PROTOS_H
#define CLIB_MATHTRANS_PROTOS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for mathtrans.library
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
AROS_LP1(LONG, SPAcos,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 20, MathTrans)

AROS_LP1(LONG, SPAsin,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 19, MathTrans)

AROS_LP1(LONG, SPAtan,
    AROS_LPA(LONG , fnum1 , D0),
    struct Library *, MathTransBase, 5, MathTrans)

AROS_LP1(LONG, SPCos,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 7, MathTrans)

AROS_LP1(LONG, SPCosh,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 11, MathTrans)

AROS_LP1(LONG, SPExp,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 13, MathTrans)

AROS_LP1(LONG, SPFieee,
    AROS_LPA(LONG, ieeenum, D0),
    struct Library *, MathTransBase, 18, MathTrans)

AROS_LP1(LONG, SPLog,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 14, MathTrans)

AROS_LP1(LONG, SPLog10,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 21, MathTrans)

AROS_LP2(LONG, SPPow,
    AROS_LPA(LONG, fnum1, D1),
    AROS_LPA(LONG, fnum2, D0),
    struct Library *, MathTransBase, 15, MathTrans)

AROS_LP1(LONG, SPSin,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 6, MathTrans)

AROS_LP2(LONG, SPSincos,
    AROS_LPA(IPTR*, pfnum2, D1),
    AROS_LPA(LONG , fnum1 , D0),
    struct Library *, MathTransBase, 9, MathTrans)

AROS_LP1(LONG, SPSinh,
    AROS_LPA(LONG, fnum1 , D0),
    struct Library *, MathTransBase, 10, MathTrans)

AROS_LP1(LONG, SPSqrt,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 16, MathTrans)

AROS_LP1(LONG, SPTan,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 8, MathTrans)

AROS_LP1(LONG, SPTanh,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathTransBase, 12, MathTrans)

AROS_LP1(LONG, SPTieee,
    AROS_LPA(LONG, fnum, D0),
    struct Library *, MathTransBase, 17, MathTrans)


#endif /* CLIB_MATHTRANS_PROTOS_H */
