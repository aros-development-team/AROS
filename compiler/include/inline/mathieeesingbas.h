#ifndef _INLINE_MATHIEEESINGBAS_H
#define _INLINE_MATHIEEESINGBAS_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef MATHIEEESINGBAS_BASE_NAME
#define MATHIEEESINGBAS_BASE_NAME MathIeeeSingBasBase
#endif

#define IEEESPAbs(parm) \
	LP1(0x36, FLOAT, IEEESPAbs, FLOAT, parm, d0, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPAdd(leftParm, rightParm) \
	LP2(0x42, FLOAT, IEEESPAdd, FLOAT, leftParm, d0, FLOAT, rightParm, d1, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPCeil(parm) \
	LP1(0x60, FLOAT, IEEESPCeil, FLOAT, parm, d0, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPCmp(leftParm, rightParm) \
	LP2(0x2a, LONG, IEEESPCmp, FLOAT, leftParm, d0, FLOAT, rightParm, d1, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPDiv(dividend, divisor) \
	LP2(0x54, FLOAT, IEEESPDiv, FLOAT, dividend, d0, FLOAT, divisor, d1, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPFix(parm) \
	LP1(0x1e, LONG, IEEESPFix, FLOAT, parm, d0, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPFloor(parm) \
	LP1(0x5a, FLOAT, IEEESPFloor, FLOAT, parm, d0, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPFlt(integer) \
	LP1(0x24, FLOAT, IEEESPFlt, long, integer, d0, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPMul(leftParm, rightParm) \
	LP2(0x4e, FLOAT, IEEESPMul, FLOAT, leftParm, d0, FLOAT, rightParm, d1, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPNeg(parm) \
	LP1(0x3c, FLOAT, IEEESPNeg, FLOAT, parm, d0, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPSub(leftParm, rightParm) \
	LP2(0x48, FLOAT, IEEESPSub, FLOAT, leftParm, d0, FLOAT, rightParm, d1, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPTst(parm) \
	LP1(0x30, LONG, IEEESPTst, FLOAT, parm, d0, \
	, MATHIEEESINGBAS_BASE_NAME)

#endif /* _INLINE_MATHIEEESINGBAS_H */
