#ifndef PROTO_MATHFFP_H
#define PROTO_MATHFFP_H

#ifndef INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef MATHFFP_BASE_NAME
#define MATHFFP_BASE_NAME MathBase
#endif

#define SPAbs(fnum1) \
        LP1(0x24, FLOAT, SPAbs, FLOAT, fnum1, \
        , MATHFFP_BASE_NAME)

#define SPAdd(fnum1, fnum2) \
        LP2(0x2c, FLOAT, SPAdd, FLOAT, fnum1, FLOAT, fnum2, \
        , MATHFFP_BASE_NAME)

#define SPCeil(y) \
        LP1(0x40, FLOAT, SPCeil, FLOAT, y, \
        , MATHFFP_BASE_NAME)

#define SPCmp(fnum1, fnum2) \
        LP2(0x1c, LONG, SPCmp, FLOAT, fnum1, FLOAT, fnum2, \
        , MATHFFP_BASE_NAME)

#define SPDiv(fnum1, fnum2) \
        LP2(0x38, FLOAT, SPDiv, FLOAT, fnum1, FLOAT, fnum2, \
        , MATHFFP_BASE_NAME)

#define SPFix(fnum) \
        LP1(0x14, LONG, SPFix, FLOAT, fnum, \
        , MATHFFP_BASE_NAME)

#define SPFloor(y) \
        LP1(0x3c, FLOAT, SPFloor, FLOAT, y, \
        , MATHFFP_BASE_NAME)

#define SPFlt(inum) \
        LP1(0x18, FLOAT, SPFlt, LONG, inum, \
        , MATHFFP_BASE_NAME)

#define SPMul(fnum1, fnum2) \
        LP2(0x34, FLOAT, SPMul, FLOAT, fnum1, FLOAT, fnum2, \
        , MATHFFP_BASE_NAME)

#define SPNeg(fnum1) \
        LP1(0x28, FLOAT, SPNeg, FLOAT, fnum1, \
        , MATHFFP_BASE_NAME)

#define SPSub(fnum1, fnum2) \
        LP2(0x30, FLOAT, SPSub, FLOAT, fnum1, FLOAT, fnum2, \
        , MATHFFP_BASE_NAME)

#define SPTst(fnum) \
        LP1(0x20, LONG, SPTst, FLOAT, fnum, \
        , MATHFFP_BASE_NAME)

#endif /* PROTO_MATHFFP_H */
