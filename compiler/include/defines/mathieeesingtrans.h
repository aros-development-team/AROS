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
    AROS_LC1(FLOAT, IEEESPAcos, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 20, Mathieeesptrans)

#define IEEESPAsin(y) \
    AROS_LC1(FLOAT, IEEESPAsin, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 19, Mathieeesptrans)

#define IEEESPAtan(y) \
    AROS_LC1(FLOAT, IEEESPAtan, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 5, Mathieeesptrans)

#define IEEESPCos(y) \
    AROS_LC1(FLOAT, IEEESPCos, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 7, Mathieeesptrans)

#define IEEESPCosh(y) \
    AROS_LC1(FLOAT, IEEESPCosh, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 11, Mathieeesptrans)

#define IEEESPExp(y) \
    AROS_LC1(FLOAT, IEEESPExp, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 13, Mathieeesptrans)

#define IEEESPFieee(y) \
    AROS_LC1(FLOAT, IEEESPFieee, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 18, Mathieeesptrans)

#define IEEESPLog(y) \
    AROS_LC1(FLOAT, IEEESPLog, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 14, Mathieeesptrans)

#define IEEESPLog10(y) \
    AROS_LC1(FLOAT, IEEESPLog10, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 21, Mathieeesptrans)

#define IEEESPPow(x, y) \
    AROS_LC2(FLOAT, IEEESPPow, \
    AROS_LPA(FLOAT, x, D0), \
    AROS_LPA(FLOAT, y, D1), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 15, Mathieeesptrans)

#define IEEESPSin(y) \
    AROS_LC1(FLOAT, IEEESPSin, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 6, Mathieeesptrans)

#define IEEESPSincos(z, y) \
    AROS_LC2(FLOAT, IEEESPSincos, \
    AROS_LPA(FLOAT, z, a0), \
    AROS_LPA(FLOAT, y, d0)
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 9, Mathieeesptrans)

#define IEEESPSinh(y) \
    AROS_LC1(FLOAT, IEEESPSinh, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 10, Mathieeesptrans)

#define IEEESPSqrt(y) \
    AROS_LC1(FLOAT, IEEESPSqrt, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 16, Mathieeesptrans)

#define IEEESPTan(y) \
    AROS_LC1(FLOAT, IEEESPTan, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 8, Mathieeesptrans)

#define IEEESPTanh(y) \
    AROS_LC1(FLOAT, IEEESPTanh, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 12, Mathieeesptrans)

#define IEEESPTieee(y) \
    AROS_LC1(FLOAT, IEEESPTieee, \
    AROS_LPA(FLOAT, y, D0), \
    struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 17, Mathieeesptrans)


#endif /* DEFINES_MATHIEEESINGTRANS_H */
