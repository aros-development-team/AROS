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
    struct Library *, MathTransBase, 20, MathTrans)

#define SPAsin(fnum1) \
    AROS_LC1(LONG, SPAsin, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 19, MathTrans)

#define SPAtan(fnum1) \
    AROS_LC1(LONG, SPAtan, \
    AROS_LCA(LONG , fnum1 , D0), \
    struct Library *, MathTransBase, 5, MathTrans)

#define SPCos(fnum1) \
    AROS_LC1(LONG, SPCos, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 7, MathTrans)

#define SPCosh(fnum1) \
    AROS_LC1(LONG, SPCosh, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 11, MathTrans)

#define SPExp(fnum1) \
    AROS_LC1(LONG, SPExp, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 13, MathTrans)

#define SPFieee(ieeenum) \
    AROS_LC1(LONG, SPFieee, \
    AROS_LCA(LONG, ieeenum, D0), \
    struct Library *, MathTransBase, 18, MathTrans)

#define SPLog(fnum1) \
    AROS_LC1(LONG, SPLog, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 14, MathTrans)

#define SPLog10(fnum1) \
    AROS_LC1(LONG, SPLog10, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 21, MathTrans)

#define SPPow(fnum1, fnum2) \
    AROS_LC2(LONG, SPPow, \
    AROS_LCA(LONG, fnum1, D1), \
    AROS_LCA(LONG, fnum2, D0), \
    struct Library *, MathTransBase, 15, MathTrans)

#define SPSin(fnum1) \
    AROS_LC1(LONG, SPSin, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 6, MathTrans)

#define SPSincos(pfnum2, fnum1) \
    AROS_LC2(LONG, SPSincos, \
    AROS_LCA(IPTR*, pfnum2, D1), \
    AROS_LCA(LONG , fnum1 , D0), \
    struct Library *, MathTransBase, 9, MathTrans)

#define SPSinh(fnum1) \
    AROS_LC1(LONG, SPSinh, \
    AROS_LCA(LONG, fnum1 , D0), \
    struct Library *, MathTransBase, 10, MathTrans)

#define SPSqrt(fnum1) \
    AROS_LC1(LONG, SPSqrt, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 16, MathTrans)

#define SPTan(fnum1) \
    AROS_LC1(LONG, SPTan, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 8, MathTrans)

#define SPTanh(fnum1) \
    AROS_LC1(LONG, SPTanh, \
    AROS_LCA(LONG, fnum1, D0), \
    struct Library *, MathTransBase, 12, MathTrans)

#define SPTieee(fnum) \
    AROS_LC1(LONG, SPTieee, \
    AROS_LCA(LONG, fnum, D0), \
    struct Library *, MathTransBase, 17, MathTrans)


#endif /* DEFINES_MATHTRANS_H */
