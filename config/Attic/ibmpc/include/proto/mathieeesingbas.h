#ifndef PROTO_MATHIEEESINGBAS_H
#define PROTO_MATHIEEESINGBAS_H

#ifndef INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef MATHIEEESINGBAS_BASE_NAME
#define MATHIEEESINGBAS_BASE_NAME MathIeeeSingBasBase
#endif

#define IEEESPAbs(parm) \
	LP1(0x24, FLOAT, IEEESPAbs, FLOAT, parm, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPAdd(leftParm, rightParm) \
	LP2(0x2c, FLOAT, IEEESPAdd, FLOAT, leftParm, FLOAT, rightParm, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPCeil(parm) \
	LP1(0x40, FLOAT, IEEESPCeil, FLOAT, parm, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPCmp(leftParm, rightParm) \
	LP2(0x1c, LONG, IEEESPCmp, FLOAT, leftParm, FLOAT, rightParm, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPDiv(dividend, divisor) \
	LP2(0x38, FLOAT, IEEESPDiv, FLOAT, dividend, FLOAT, divisor, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPFix(parm) \
	LP1(0x14, LONG, IEEESPFix, FLOAT, parm, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPFloor(parm) \
	LP1(0x3c, FLOAT, IEEESPFloor, FLOAT, parm, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPFlt(integer) \
	LP1(0x18, FLOAT, IEEESPFlt, long, integer, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPMul(leftParm, rightParm) \
	LP2(0x34, FLOAT, IEEESPMul, FLOAT, leftParm, FLOAT, rightParm, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPNeg(parm) \
	LP1(0x28, FLOAT, IEEESPNeg, FLOAT, parm, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPSub(leftParm, rightParm) \
	LP2(0x30, FLOAT, IEEESPSub, FLOAT, leftParm, FLOAT, rightParm, \
	, MATHIEEESINGBAS_BASE_NAME)

#define IEEESPTst(parm) \
	LP1(0x20, LONG, IEEESPTst, FLOAT, parm, \
	, MATHIEEESINGBAS_BASE_NAME)

#endif /* PROTO_MATHIEEESINGBAS_H */
