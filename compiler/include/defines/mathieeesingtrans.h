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
#define IEEESPCos(y) \
    AROS_LC1(LONG, IEEESPCos, \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 7, Mathieeesingtrans)

#define IEEESPCosh(y) \
    AROS_LC1(LONG, IEEESPCosh, \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 11, Mathieeesingtrans)

#define IEEESPExp(y) \
    AROS_LC1(LONG, IEEESPExp, \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 13, Mathieeesingtrans)

#define IEEESPFieee(y) \
    AROS_LC1(LONG, IEEESPFieee, \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 18, Mathieeesingtrans)

#define IEEESPLog(y) \
    AROS_LC1(LONG, IEEESPLog, \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 14, Mathieeesingtrans)

#define IEEESPLog10(y) \
    AROS_LC1(LONG, IEEESPLog10, \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 21, Mathieeesingtrans)

#define IEEESPPow(x, y) \
    AROS_LC2(LONG, IEEESPPow, \
    AROS_LCA(LONG, x, D1), \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 15, Mathieeesingtrans)

#define IEEESPSin(y) \
    AROS_LC1(LONG, IEEESPSin, \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 6, Mathieeesingtrans)

#define IEEESPSinh(y) \
    AROS_LC1(LONG, IEEESPSinh, \
    AROS_LCA(LONG, y , D0), \
    struct Library *, MathIeeeSingTransBase, 10, Mathieeesingtrans)

#define IEEESPTan(y) \
    AROS_LC1(LONG, IEEESPTan, \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 8, Mathieeesingtrans)

#define IEEESPTanh(y) \
    AROS_LC1(LONG, IEEESPTanh, \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 12, Mathieeesingtrans)

#define IEEESPTieee(y) \
    AROS_LC1(LONG, IEEESPTieee, \
    AROS_LCA(LONG, y, D0), \
    struct Library *, MathIeeeSingTransBase, 17, Mathieeesingtrans)


#endif /* DEFINES_MATHIEEESINGTRANS_H */
