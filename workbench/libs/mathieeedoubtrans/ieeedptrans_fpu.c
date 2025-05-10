/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.
*/

#include <aros/libcall.h>
#include <math.h>
#include "mathieeedoubtrans_intern.h"

#undef double

/*
  Problem (ONLY on Linux/M68K with binary compatibility turned on):
   In order to get binary compatibility with the original Amiga OS
   we have to return the value in D0/D1. This is NOT automatically
   done by the compiler. The result would be returned in one of the
   FPU registers instead. So we're using the trick with the QUADs.
   See below.
*/
#if UseRegisterArgs
  #define RETURN_TYPE QUAD  /* For Linux/M68k & AmigaOS w/ bin. compat. */
#else
  #define RETURN_TYPE double /* for the rest */
#endif


AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPAtan,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 5, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=atan(y);
    return * Res;
#else
    return atan(y);
#endif
} /* FPU_IEEEDPAtan */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPSin,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 6, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=sin(y);
    return * Res;
#else
    return sin(y);
#endif
} /* FPU_IEEEDPSin */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPCos,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 7, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=cos(y);
    return * Res;
#else
    return cos(y);
#endif
} /* FPU_IEEEDPCos */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPTan,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 8, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=tan(y);
    return * Res;
#else
    return tan(y);
#endif
} /* FPU_IEEEDPTan */

AROS_LHDOUBLE2(RETURN_TYPE, FPU_IEEEDPSincos,
    AROS_LHA2(double *, pf2, A0, D1),
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 9, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    *pf2 = cos(y);
    y=sin(y);
    return * Res;
#else
    *pf2 = cos(y);
    return sin(y);
#endif
} /* FPU_IEEEDPSincos */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPSinh,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 10, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=sinh(y);
    return * Res;
#else
    return sinh(y);
#endif
} /* FPU_IEEEDPSinh */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPCosh,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 11, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=cosh(y);
    return * Res;
#else
    return cosh(y);
#endif
} /* FPU_IEEEDPCosh */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPTanh,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 12, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=tanh(y);
    return * Res;
#else
    return tanh(y);
#endif
} /* FPU_IEEEDPTanh */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPExp,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 13, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=exp(y);
    return * Res;
#else
    return exp(y);
#endif
} /* FPU_IEEEDPExp */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPLog,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 14, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=log(y);
    return * Res;
#else
    return log(y);
#endif
} /* FPU_IEEEDPLog */

AROS_LHDOUBLE2(RETURN_TYPE, FPU_IEEEDPPow,
    AROS_LHA2(double, y, D2, D3),
    AROS_LHA2(double, z, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 15, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=pow(y,z);
    return * Res;
#else
    return pow(y,z);
#endif
} /* FPU_IEEEDPPow */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPSqrt,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 16, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=sqrt(y);
    return * Res;
#else
    return sqrt(y);
#endif
} /* FPU_IEEEDPSqrt */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPTieee,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 17, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    return * Res;
#else
    return y;
#endif
} /* FPU_IEEEDPTieee */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPFieee,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 18, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    return * Res;
#else
    return y;
#endif
} /* FPU_IEEEDPFieee */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPAsin,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 19, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=asin(y);
    return * Res;
#else
    return asin(y);
#endif
} /* FPU_IEEEDPAsin */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPAcos,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 20, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=acos(y);
    return * Res;
#else
    return acos(y);
#endif
} /* FPU_IEEEDPAcos */

AROS_LHDOUBLE1(RETURN_TYPE, FPU_IEEEDPLog10,
    AROS_LHA2(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 21, MathIeeeDoubTrans
)
{
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=log10(y);
    return * Res;
#else
    return log10(y);
#endif
} /* FPU_IEEEDPLog10 */
