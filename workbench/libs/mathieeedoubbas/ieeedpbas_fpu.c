#include <aros/libcall.h>
#include <exec/types.h>
#include <proto/mathieeedoubbas.h>
#include <math.h>
#include "mathieeedoubbas_intern.h"

#undef double

AROS_LHQUAD1(LONG, FPU_IEEEDPFix,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 5, MathIeeeDoubBas)
{
  return (LONG)y;
} /* FPU_IEEEDPFix */

AROS_LHQUAD1(QUAD, FPU_IEEEDPFlt,
AROS_LHAQUAD(LONG, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 6, MathIeeeDoubBas)
{
  double d;
  QUAD * Res = (QUAD *)&d; /* this forces the returned value to be in D0/D1 */
  d = (double)y;
  return * Res;
} /* FPU_IEEEDPFlt */

AROS_LHQUAD2(LONG, FPU_IEEEDPCmp,
AROS_LHAQUAD(double, y, D0, D1),
AROS_LHAQUAD(double, z, D2, D3),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 7, MathIeeeDoubBas)
{
  if (y == z)
    return 0;
  if (y > z)
    return 1;
  return -1;
} /* FPU_IEEEDPCmp */

AROS_LHQUAD1(LONG, FPU_IEEEDPTst,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 8, MathIeeeDoubBas)
{
  if (y==0.0)
    return 0;
  if (y > 0)
    return 1;
  return -1;
} /* FPU_IEEEDPTst */

AROS_LHQUAD1(QUAD, FPU_IEEEDPAbs,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 9, MathIeeeDoubBas)
{
  QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
  y=abs(y);
  return * Res;
} /* FPU_IEEEDPAbs */

AROS_LHQUAD1(QUAD, FPU_IEEEDPNeg,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 10, MathIeeeDoubBas)
{
  QUAD * Res = (QUAD *)&y; /* this forces the returned value to be in D0/D1 */
  y = -y;
  return * Res;
} /* FPU_IEEEDPNeg */

AROS_LHQUAD2(QUAD, FPU_IEEEDPAdd,
AROS_LHAQUAD(double, y, D0, D1),
AROS_LHAQUAD(double, z, D2, D3),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 11, MathIeeeDoubBas)
{
  QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
  y=y+z;
  return * Res;
} /* FPU_IEEEDPAdd */

AROS_LHQUAD2(QUAD, FPU_IEEEDPSub,
AROS_LHAQUAD(double, y, D0, D1),
AROS_LHAQUAD(double, z, D2, D3),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 12, MathIeeeDoubBas)
{
  QUAD * Res = (QUAD *)&y; /* this forces the returned value to be in D0/D1 */
  y=y-z;
  return * Res;
} /* FPU_IEEEDPSub */

AROS_LHQUAD2(QUAD, FPU_IEEEDPMul,
AROS_LHAQUAD(double, y, D0, D1),
AROS_LHAQUAD(double, z, D2, D3),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 13, MathIeeeDoubBas)
{
  QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
  y=y*z;
  return * Res;
} /* FPU_IEEEDPMul */

AROS_LHQUAD2(QUAD, FPU_IEEEDPDiv,
AROS_LHAQUAD(double, y, D0, D1),
AROS_LHAQUAD(double, z, D2, D3),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 14, MathIeeeDoubBas)
{
  QUAD * Res = (QUAD *)&y; /* this forces the returned value to be in D0/D1 */
  y = y/z;
  return * Res;
} /* FPU_IEEEDPDiv */

AROS_LHQUAD1(QUAD, FPU_IEEEDPFloor,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 15, MathIeeeDoubBas)
{
 /*  return floor(y); */
  return 0xbadc0deULL;
} /* FPU_IEEEDPFloor */

AROS_LHQUAD1(QUAD, FPU_IEEEDPCeil,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 16, MathIeeeDoubBas)
{
 /*  return ceil(y); */
  return 0xbadc0deULL;
} /* FPU_IEEEDPCeil */

