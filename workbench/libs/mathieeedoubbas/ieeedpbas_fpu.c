#include <aros/libcall.h>
#include <proto/mathieeedoubbas.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeedoubbas_intern.h"


AROS_LHQUAD1(LONG, FPU_IEEEDPFix,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 5, MathIeeeDoubBas)
{
  return (LONG)y;
} /* FPU_IEEEDPFix */

AROS_LHQUAD1(double, FPU_IEEEDPFlt,
AROS_LHAQUAD(LONG, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 6, MathIeeeDoubBas)
{
  return (double)y;
} /* FPU_IEEEDPAbs */

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
AROS_LHAQUAD(LONG, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 8, MathIeeeDoubBas)
{
  if (y==0.0)
    return 0;
  if (y > 0)
    return 1;
  return -1;
} /* FPU_IEEEDPTst */

AROS_LHQUAD1(double, FPU_IEEEDPAbs,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 9, MathIeeeDoubBas)
{
  return abs(y);
} /* FPU_IEEEDPAbs */

AROS_LHQUAD1(double, FPU_IEEEDPNeg,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 10, MathIeeeDoubBas)
{
  return -y;
} /* FPU_IEEEDPNeg */

AROS_LHQUAD2(double, FPU_IEEEDPAdd,
AROS_LHAQUAD(double, y, D0, D1),
AROS_LHAQUAD(double, z, D2, D3),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 11, MathIeeeDoubBas)
{
  return y+z;
} /* FPU_IEEEDPAdd */

AROS_LHQUAD2(double, FPU_IEEEDPSub,
AROS_LHAQUAD(double, y, D0, D1),
AROS_LHAQUAD(double, z, D2, D3),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 12, MathIeeeDoubBas)
{
  return y-z;
} /* FPU_IEEEDPSub */

AROS_LHQUAD2(double, FPU_IEEEDPMul,
AROS_LHAQUAD(double, y, D0, D1),
AROS_LHAQUAD(double, z, D2, D3),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 13, MathIeeeDoubBas)
{
  return y*z;
} /* FPU_IEEEDPMul */

AROS_LHQUAD2(double, FPU_IEEEDPDiv,
AROS_LHAQUAD(double, y, D0, D1),
AROS_LHAQUAD(double, z, D2, D3),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 14, MathIeeeDoubBas)
{
  return y/z;
} /* FPU_IEEEDPDiv */


