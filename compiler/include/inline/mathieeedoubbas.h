#ifndef _INLINE_MATHIEEEDOUBBAS_H
#define _INLINE_MATHIEEEDOUBBAS_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef MATHIEEEDOUBBAS_BASE_NAME
#define MATHIEEEDOUBBAS_BASE_NAME MathIeeeDoubBasBase
#endif

#define IEEEDPAbs(parm) \
	LP1(0x36, DOUBLE, IEEEDPAbs, DOUBLE, parm, d0, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPAdd(leftParm, rightParm) \
	LP2(0x42, DOUBLE, IEEEDPAdd, DOUBLE, leftParm, d0, DOUBLE, rightParm, d2, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPCeil(parm) \
	LP1(0x60, DOUBLE, IEEEDPCeil, DOUBLE, parm, d0, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPCmp(leftParm, rightParm) \
	LP2(0x2a, LONG, IEEEDPCmp, DOUBLE, leftParm, d0, DOUBLE, rightParm, d2, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPDiv(dividend, divisor) \
	LP2(0x54, DOUBLE, IEEEDPDiv, DOUBLE, dividend, d0, DOUBLE, divisor, d2, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPFix(parm) \
	LP1(0x1e, LONG, IEEEDPFix, DOUBLE, parm, d0, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPFloor(parm) \
	LP1(0x5a, DOUBLE, IEEEDPFloor, DOUBLE, parm, d0, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPFlt(integer) \
	LP1(0x24, DOUBLE, IEEEDPFlt, long, integer, d0, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPMul(factor1, factor2) \
	LP2(0x4e, DOUBLE, IEEEDPMul, DOUBLE, factor1, d0, DOUBLE, factor2, d2, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPNeg(parm) \
	LP1(0x3c, DOUBLE, IEEEDPNeg, DOUBLE, parm, d0, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPSub(leftParm, rightParm) \
	LP2(0x48, DOUBLE, IEEEDPSub, DOUBLE, leftParm, d0, DOUBLE, rightParm, d2, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#define IEEEDPTst(parm) \
	LP1(0x30, LONG, IEEEDPTst, DOUBLE, parm, d0, \
	, MATHIEEEDOUBBAS_BASE_NAME)

#endif /* _INLINE_MATHIEEEDOUBBAS_H */
