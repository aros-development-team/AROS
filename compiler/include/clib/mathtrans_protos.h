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
    struct Library *, MathtransBase, 20, Mathtrans)

AROS_LP1(LONG, SPAsin,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathtransBase, 19, Mathtrans)

AROS_LP1(LONG, SPAtan,
    AROS_LPA(LONG , fnum1 , D0),
    struct Library *, MathtransBase, 5, Mathtrans)

AROS_LP1(LONG, SPCos,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathtransBase, 7, Mathtrans)

AROS_LP1(LONG, SPCosh,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathtransBase, 11, Mathtrans)

AROS_LP1(LONG, SPExp,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathtransBase, 13, Mathtrans)

AROS_LP1(LONG, SPFieee,
    AROS_LPA(LONG, ieeenum, D0),
    struct Library *, MathtransBase, 18, Mathtrans)

AROS_LP1(LONG, SPLog,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathtransBase, 14, Mathtrans)

AROS_LP1(LONG, SPLog10,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathtransBase, 21, Mathtrans)

AROS_LP2(LONG, SPPow,
    AROS_LPA(LONG, fnum1, D1),
    AROS_LPA(LONG, fnum2, D0),
    struct Library *, MathtransBase, 15, Mathtrans)

AROS_LP1(LONG, SPSin,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathtransBase, 6, Mathtrans)

AROS_LP2(LONG, SPSincos,
    AROS_LPA(IPTR*, pfnum2, D1),
    AROS_LPA(LONG , fnum1 , D0),
    struct Library *, MathtransBase, 9, Mathtrans)

AROS_LP1(LONG, SPSinh,
    AROS_LPA(LONG, fnum1 , D0),
    struct Library *, MathtransBase, 10, Mathtrans)

AROS_LP1(LONG, SPSqrt,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathtransBase, 16, Mathtrans)

AROS_LP1(LONG, SPTan,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathtransBase, 8, Mathtrans)

AROS_LP1(LONG, SPTanh,
    AROS_LPA(LONG, fnum1, D0),
    struct Library *, MathtransBase, 12, Mathtrans)

AROS_LP1(LONG, SPTieee,
    AROS_LPA(LONG, fnum, D0),
    struct Library *, MathtransBase, 17, Mathtrans)


#endif /* CLIB_MATHTRANS_PROTOS_H */
