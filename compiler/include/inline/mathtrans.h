#ifndef _INLINE_MATHTRANS_H
#define _INLINE_MATHTRANS_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef MATHTRANS_BASE_NAME
#define MATHTRANS_BASE_NAME MathTransBase
#endif

#define SPAcos(parm) \
        LP1(0x78, FLOAT, SPAcos, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPAsin(parm) \
        LP1(0x72, FLOAT, SPAsin, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPAtan(parm) \
        LP1(0x1e, FLOAT, SPAtan, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPCos(parm) \
        LP1(0x2a, FLOAT, SPCos, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPCosh(parm) \
        LP1(0x42, FLOAT, SPCosh, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPExp(parm) \
        LP1(0x4e, FLOAT, SPExp, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPFieee(parm) \
        LP1(0x6c, FLOAT, SPFieee, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPLog(parm) \
        LP1(0x54, FLOAT, SPLog, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPLog10(parm) \
        LP1(0x7e, FLOAT, SPLog10, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPPow(exp, arg) \
        LP2(0x5a, FLOAT, SPPow, FLOAT, exp, d1, FLOAT, arg, d0, \
        , MATHTRANS_BASE_NAME)

#define SPSin(parm) \
        LP1(0x24, FLOAT, SPSin, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPSincos(cosptr, parm) \
        LP2(0x36, FLOAT, SPSincos, FLOAT *, cosptr, a0, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPSinh(parm) \
        LP1(0x3c, FLOAT, SPSinh, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPSqrt(parm) \
        LP1(0x60, FLOAT, SPSqrt, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPTan(parm) \
        LP1(0x30, FLOAT, SPTan, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPTanh(parm) \
        LP1(0x48, FLOAT, SPTanh, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#define SPTieee(parm) \
        LP1(0x66, FLOAT, SPTieee, FLOAT, parm, d0, \
        , MATHTRANS_BASE_NAME)

#endif /* _INLINE_MATHTRANS_H */
