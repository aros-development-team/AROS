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
    AROS_LC1(LONG, SPAcos, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 20, Mathtrans)

#define SPAsin(fnum1) \
    AROS_LC1(LONG, SPAsin, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 19, Mathtrans)

#define SPAtan(fnum1) \
    AROS_LC1(LONG, SPAtan, \
    AROS_LCA(LONG , fnum1 , D0), \
    struct Library *, MathTransBase, 5, Mathtrans)

#define SPCos(fnum1) \
    AROS_LC1(LONG, SPCos, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 7, Mathtrans)

#define SPCosh(fnum1) \
    AROS_LC1(LONG, SPCosh, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 11, Mathtrans)

#define SPExp(fnum1) \
    AROS_LC1(LONG, SPExp, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 13, Mathtrans)

#define SPFieee(ieeenum) \
    AROS_LC1(LONG, SPFieee, \
    AROS_LCA(LONG, ieeenum, D0), \
    struct Library *, MathTransBase, 18, Mathtrans)

#define SPLog(fnum1) \
    AROS_LC1(LONG, SPLog, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 14, Mathtrans)

#define SPLog10(fnum1) \
    AROS_LC1(LONG, SPLog10, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 21, Mathtrans)

#define SPPow(fnum1, fnum2) \
    AROS_LC2(LONG, SPPow, \
    AROS_LCA(LONG, fnum1, D1), \
    AROS_LCA(LONG, fnum2, D0), \
    struct Library *, MathTransBase, 15, Mathtrans)

#define SPSin(fnum1) \
    AROS_LC1(LONG, SPSin, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 6, Mathtrans)

#define SPSincos(pfnum2, fnum1) \
    AROS_LC2(LONG, SPSincos, \
    AROS_LCA(IPTR*, pfnum2, D1), \
    AROS_LCA(LONG , fnum1 , D0), \
    struct Library *, MathTransBase, 9, Mathtrans)

#define SPSinh(fnum1) \
    AROS_LC1(LONG, SPSinh, \
    AROS_LCA(LONG, fnum1 , D0), \
    struct Library *, MathTransBase, 10, Mathtrans)

#define SPSqrt(fnum1) \
    AROS_LC1(LONG, SPSqrt, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 16, Mathtrans)

#define SPTan(fnum1) \
    AROS_LC1(LONG, SPTan, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 8, Mathtrans)

#define SPTanh(fnum1) \
    AROS_LC1(LONG, SPTanh, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 12, Mathtrans)

#define SPTieee(fnum) \
    AROS_LC1(LONG, SPTieee, \
    AROS_LCA(LONG, fnum, D0), \
    struct Library *, MathTransBase, 17, Mathtrans)


#endif /* DEFINES_MATHTRANS_H */
