#ifndef DEFINES_MATHIEEESINGTRANS_H
#define DEFINES_MATHIEEESINGTRANS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/

#define IEEESPAcos(y) \
    AROS_LC1(float, IEEESPAcos, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 20, Mathieeesingtrans)

#define IEEESPAsin(y) \
    AROS_LC1(float, IEEESPAsin, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 19, Mathieeesingtrans)

#define IEEESPCos(y) \
    AROS_LC1(float, IEEESPCos, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 7, Mathieeesingtrans)

#define IEEESPCosh(y) \
    AROS_LC1(float, IEEESPCosh, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 11, Mathieeesingtrans)

#define IEEESPExp(y) \
    AROS_LC1(float, IEEESPExp, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 13, Mathieeesingtrans)

#define IEEESPFieee(y) \
    AROS_LC1(float, IEEESPFieee, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 18, Mathieeesingtrans)

#define IEEESPLog(y) \
    AROS_LC1(float, IEEESPLog, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 14, Mathieeesingtrans)

#define IEEESPLog10(y) \
    AROS_LC1(float, IEEESPLog10, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 21, Mathieeesingtrans)

#define IEEESPPow(x, y) \
    AROS_LC2(float, IEEESPPow, \
    AROS_LCA(float, x, D1), \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 15, Mathieeesingtrans)

#define IEEESPSin(y) \
    AROS_LC1(float, IEEESPSin, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 6, Mathieeesingtrans)

#define IEEESPSinh(y) \
    AROS_LC1(float, IEEESPSinh, \
    AROS_LCA(float, y , D0), \
    struct Library *, MathIeeeSingTransBase, 10, Mathieeesingtrans)

#define IEEESPSqrt(y) \
    AROS_LC1(float, IEEESPSqrt, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 16, Mathieeesingtrans)

#define IEEESPTan(y) \
    AROS_LC1(float, IEEESPTan, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 8, Mathieeesingtrans)

#define IEEESPTanh(y) \
    AROS_LC1(float, IEEESPTanh, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 12, Mathieeesingtrans)

#define IEEESPTieee(y) \
    AROS_LC1(float, IEEESPTieee, \
    AROS_LCA(float, y, D0), \
    struct Library *, MathIeeeSingTransBase, 17, Mathieeesingtrans)


#endif /* DEFINES_MATHIEEESINGTRANS_H */
