#include <aros/libcall.h>
#include <math.h>
#include "mathieeedoubtrans_intern.h"

AROS_LHQUAD1(double, FPU_IEEEDPAtan,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 5, MathIeeeDoubTrans)
{
  return atan(y);
} /* FPU_IEEEDPAtan */

AROS_LHQUAD1(double, FPU_IEEEDPSin,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 6, MathIeeeDoubTrans)
{
  return sin(y);
} /* FPU_IEEEDPSin */

AROS_LHQUAD1(LONG, FPU_IEEEDPCos,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 7, MathIeeeDoubTrans)
{
  return cos(y);
} /* FPU_IEEEDPCos */

AROS_LHQUAD1(LONG, FPU_IEEEDPTan,
AROS_LHAQUAD(LONG, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 8, MathIeeeDoubTrans)
{
  return tan(y);
} /* FPU_IEEEDPTan */

AROS_LHQUAD2(double, FPU_IEEEDPSincos,
AROS_LHAQUAD(double *, pf2, A0, D1),
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 9, MathIeeeDoubTrans)
{
  *pf2 = cos(y);
  return sin(y);
} /* FPU_IEEEDPSincos */

AROS_LHQUAD1(double, FPU_IEEEDPSinh,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 10, MathIeeeDoubTrans)
{
  return sinh(y);
} /* FPU_IEEEDPSinh */

AROS_LHQUAD1(double, FPU_IEEEDPCosh,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 11, MathIeeeDoubTrans)
{
  return cosh(y);
} /* FPU_IEEEDPCosh */

AROS_LHQUAD1(double, FPU_IEEEDPTanh,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 12, MathIeeeDoubTrans)
{
  return tanh(y);
} /* FPU_IEEEDPTanh */

AROS_LHQUAD1(double, FPU_IEEEDPExp,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 13, MathIeeeDoubTrans)
{
  return exp(y);
} /* FPU_IEEEDPExp */

AROS_LHQUAD1(double, FPU_IEEEDPLog,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 14, MathIeeeDoubTrans)
{
  return log(y);
} /* FPU_IEEEDPLog */

AROS_LHQUAD2(double, FPU_IEEEDPPow,
AROS_LHAQUAD(double, y, D2, D3),
AROS_LHAQUAD(double, z, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 15, MathIeeeDoubTrans)
{
  return pow(y,z);
} /* FPU_IEEEDPPow */

AROS_LHQUAD1(double, FPU_IEEEDPSqrt,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 16, MathIeeeDoubTrans)
{
  return sqrt(y);
} /* FPU_IEEEDPSqrt */

AROS_LHQUAD1(double, FPU_IEEEDPTieee,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 17, MathIeeeDoubTrans)
{
  return y;
} /* FPU_IEEEDPTieee */

AROS_LHQUAD1(double, FPU_IEEEDPFieee,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 18, MathIeeeDoubTrans)
{
  return y;
} /* FPU_IEEEDPFieee */

AROS_LHQUAD1(double, FPU_IEEEDPAsin,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 19, MathIeeeDoubTrans)
{
  return asin(y);
} /* FPU_IEEEDPAsin */

AROS_LHQUAD1(double, FPU_IEEEDPAcos,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 20, MathIeeeDoubTrans)
{
  return acos(y);
} /* FPU_IEEEDPAcos */

AROS_LHQUAD1(double, FPU_IEEEDPLog10,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 21, MathIeeeDoubTrans)
{
  return log10(y);
} /* FPU_IEEEDPLog10 */








