#ifndef _INLINE_MATHFFP_H
#define _INLINE_MATHFFP_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef MATHFFP_BASE_NAME
#define MATHFFP_BASE_NAME MathffpBase
#endif

#define SPAbs(fnum1) \
        LP1(0x36, FLOAT, SPAbs, FLOAT, fnum1, d0, \
        , MATHFFP_BASE_NAME)

#define SPAdd(fnum1, fnum2) \
        LP2(0x42, FLOAT, SPAdd, FLOAT, fnum1, d1, FLOAT, fnum2, d0, \
        , MATHFFP_BASE_NAME)

#define SPCeil(y) \
        LP1(0x60, FLOAT, SPCeil, FLOAT, y, d0, \
        , MATHFFP_BASE_NAME)

#define SPCmp(fnum1, fnum2) \
        LP2(0x2a, LONG, SPCmp, FLOAT, fnum1, d0, FLOAT, fnum2, d1, \
        , MATHFFP_BASE_NAME)

#define SPDiv(fnum1, fnum2) \
        LP2(0x54, FLOAT, SPDiv, FLOAT, fnum1, d1, FLOAT, fnum2, d0, \
        , MATHFFP_BASE_NAME)

#define SPFix(fnum) \
        LP1(0x1e, LONG, SPFix, FLOAT, fnum, d0, \
        , MATHFFP_BASE_NAME)

#define SPFloor(y) \
        LP1(0x5a, FLOAT, SPFloor, FLOAT, y, d0, \
        , MATHFFP_BASE_NAME)

#define SPFlt(inum) \
        LP1(0x24, FLOAT, SPFlt, LONG, inum, d0, \
        , MATHFFP_BASE_NAME)

#define SPMul(fnum1, fnum2) \
        LP2(0x4e, FLOAT, SPMul, FLOAT, fnum1, d1, FLOAT, fnum2, d0, \
        , MATHFFP_BASE_NAME)

#define SPNeg(fnum1) \
        LP1(0x3c, FLOAT, SPNeg, FLOAT, fnum1, d0, \
        , MATHFFP_BASE_NAME)

#define SPSub(fnum1, fnum2) \
        LP2(0x48, FLOAT, SPSub, FLOAT, fnum1, d1, FLOAT, fnum2, d0, \
        , MATHFFP_BASE_NAME)

#define SPTst(fnum) \
        LP1(0x30, LONG, SPTst, FLOAT, fnum, d1, \
        , MATHFFP_BASE_NAME)

#endif /* _INLINE_MATHFFP_H */
