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
AROS_LP1(float, SPAcos,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 20, MathTrans)

AROS_LP1(float, SPAsin,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 19, MathTrans)

AROS_LP1(float, SPAtan,
    AROS_LPA(float , fnum1 , D0),
    struct Library *, MathTransBase, 5, MathTrans)

AROS_LP1(float, SPCos,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 7, MathTrans)

AROS_LP1(float, SPCosh,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 11, MathTrans)

AROS_LP1(float, SPExp,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 13, MathTrans)

AROS_LP1(float, SPFieee,
    AROS_LPA(float, ieeenum, D0),
    struct Library *, MathTransBase, 18, MathTrans)

AROS_LP1(float, SPLog,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 14, MathTrans)

AROS_LP1(float, SPLog10,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 21, MathTrans)

AROS_LP2(float, SPPow,
    AROS_LPA(float, fnum1, D1),
    AROS_LPA(float, fnum2, D0),
    struct Library *, MathTransBase, 15, MathTrans)

AROS_LP1(float, SPSin,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 6, MathTrans)

AROS_LP2(float, SPSincos,
    AROS_LPA(IPTR*, pfnum2, D1),
    AROS_LPA(float , fnum1 , D0),
    struct Library *, MathTransBase, 9, MathTrans)

AROS_LP1(float, SPSinh,
    AROS_LPA(float, fnum1 , D0),
    struct Library *, MathTransBase, 10, MathTrans)

AROS_LP1(float, SPSqrt,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 16, MathTrans)

AROS_LP1(float, SPTan,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 8, MathTrans)

AROS_LP1(float, SPTanh,
    AROS_LPA(float, fnum1, D0),
    struct Library *, MathTransBase, 12, MathTrans)

AROS_LP1(float, SPTieee,
    AROS_LPA(float, fnum, D0),
    struct Library *, MathTransBase, 17, MathTrans)


#endif /* CLIB_MATHTRANS_PROTOS_H */
