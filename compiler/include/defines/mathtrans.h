#ifndef DEFINES_MATHTRANS_H
#define DEFINES_MATHTRANS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/

#define SPAcos(fnum1) \
    AROS_LC1(FLOAT, SPAcos, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 20, Mathtrans)

#define SPAsin(fnum1) \
    AROS_LC1(FLOAT, SPAsin, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 19, Mathtrans)

#define SPAtan(fnum1) \
    AROS_LC1(FLOAT, SPAtan, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 5, Mathtrans)

#define SPCos(fnum1) \
    AROS_LC1(FLOAT, SPCos, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 7, Mathtrans)

#define SPCosh(fnum1) \
    AROS_LC1(FLOAT, SPCosh, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 11, Mathtrans)

#define SPExp(fnum1) \
    AROS_LC1(FLOAT, SPExp, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 13, Mathtrans)

#define SPFieee(ieeenum) \
    AROS_LC1(FLOAT, SPFieee, \
    AROS_LPA(FLOAT, ieeenum, D0), \
    struct Library *, MathTransBase, 18, Mathtrans)

#define SPLog(fnum1) \
    AROS_LC1(FLOAT, SPLog, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 14, Mathtrans)

#define SPLog10(fnum1) \
    AROS_LC1(FLOAT, SPLog10, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 21, Mathtrans)

#define SPPow(fnum1, fnum2) \
    AROS_LC2(FLOAT, SPPow, \
    AROS_LPA(FLOAT, fnum1, D1), \
    AROS_LPA(FLOAT, fnum2, D0), \
    struct Library *, MathTransBase, 15, Mathtrans)

#define SPSin(fnum1) \
    AROS_LC1(FLOAT, SPSin, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 6, Mathtrans)

#define SPSincos(pfnum2, fnum1) \
    AROS_LC2(FLOAT, SPSincos, \
    AROS_LPA(FLOAT*, pfnum2, d1), \
    AROS_LPA(FLOAT, fnum1, d0) \
    struct Library *, MathTransBase, 9, Mathtrans)

#define SPSinh(fnum1) \
    AROS_LC1(FLOAT, SPSinh, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 10, Mathtrans)

#define SPSqrt(fnum1) \
    AROS_LC1(FLOAT, SPSqrt, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 16, Mathtrans)

#define SPTan(fnum1) \
    AROS_LC1(FLOAT, SPTan, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 8, Mathtrans)

#define SPTanh(fnum1) \
    AROS_LC1(FLOAT, SPTanh, \
    AROS_LPA(FLOAT, fnum1, D0), \
    struct Library *, MathTransBase, 12, Mathtrans)

#define SPTieee(fnum) \
    AROS_LC1(FLOAT, SPTieee, \
    AROS_LPA(FLOAT, fnum, D0), \
    struct Library *, MathTransBase, 17, Mathtrans)


#endif /* DEFINES_MATHTRANS_H */
