/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <exec/types.h>
#include <proto/mathieeedoubbas.h>
#include <math.h>
#include <stdlib.h>
#include "mathieeedoubbas_intern.h"

#undef double

/*
  Problem (ONLY on Linux/M68K with binary compatibility turned on):
   In order to get binary compatibility with the original Amiga OS 
   we have to return the value in D0/D1. This is NOT automatically
   done by the compiler. The result would be returned in one of the
   FPU registers instead. So we're using the trick with the QUADs.
   See below.
*/

/*
  !!! Fixme !!!
   Problem on i386 etc:
     The functions in the *.arch file (integer-emulation of double
     operations) return QUADs. The protos however say that these functions
     are returning doubles. Unfortunately doubles are not returned like
     QUADs!
 */
//#define UseRegisterArgs 1
#if UseRegisterArgs
  #define RETURN_TYPE QUAD  /* For Linux/M68k & AmigaOS w/ bin. compat. */
#else
  #define RETURN_TYPE double /* for the rest */
#endif

AROS_LHQUAD1(LONG, FPU_IEEEDPFix,
    AROS_LHAQUAD(double, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 5, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT
    return (LONG)y;
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPFix */

AROS_LHQUAD1(RETURN_TYPE, FPU_IEEEDPFlt,
    AROS_LHAQUAD(LONG, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 6, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT
#if UseRegisterArgs
    double d;
    QUAD * Res = (QUAD *)&d; /* this forces the returned value to be in D0/D1 */
    d = (double)y;
    return * Res;
#else
    return (double)y;
#endif
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPFlt */

AROS_LHQUAD2(LONG, FPU_IEEEDPCmp,
    AROS_LHAQUAD(double, y, D0, D1),
    AROS_LHAQUAD(double, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 7, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT
    
    if (y == z) return 0;
    if (y > z)  return 1;
    return -1;
    
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPCmp */

AROS_LHQUAD1(LONG, FPU_IEEEDPTst,
    AROS_LHAQUAD(double, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 8, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT
    
    if (0.0 == y) return 0;
    if (y > 0)    return 1;
    return -1;
    
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPTst */

AROS_LHQUAD1(RETURN_TYPE, FPU_IEEEDPAbs,
    AROS_LHAQUAD(double, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 9, MathIeeeDoubBas
)
{
  AROS_LIBFUNC_INIT

#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=abs(y);
    return * Res;
#else
    return abs(y);
#endif

  AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPAbs */

AROS_LHQUAD1(RETURN_TYPE, FPU_IEEEDPNeg,
AROS_LHAQUAD(double, y, D0, D1),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 10, MathIeeeDoubBas)
{
    AROS_LIBFUNC_INIT
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y; /* this forces the returned value to be in D0/D1 */
    y = -y;
    return * Res;
#else
    return -y;
#endif
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPNeg */

AROS_LHQUAD2(RETURN_TYPE, FPU_IEEEDPAdd,
    AROS_LHAQUAD(double, y, D0, D1),
    AROS_LHAQUAD(double, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 11, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=y+z;
    return * Res;
#else
    return (y+z);
#endif
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPAdd */

AROS_LHQUAD2(RETURN_TYPE, FPU_IEEEDPSub,
AROS_LHAQUAD(double, y, D0, D1),
AROS_LHAQUAD(double, z, D2, D3),
struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 12, MathIeeeDoubBas)
{
    AROS_LIBFUNC_INIT
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y; /* this forces the returned value to be in D0/D1 */
    y=y-z;
    return * Res;
#else
    return (y-z);
#endif
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPSub */

AROS_LHQUAD2(RETURN_TYPE, FPU_IEEEDPMul,
    AROS_LHAQUAD(double, y, D0, D1),
    AROS_LHAQUAD(double, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 13, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y;  /* this forces the returned value to be in D0/D1 */
    y=y*z;
    return * Res;
#else
    return y*z;
#endif
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPMul */

AROS_LHQUAD2(RETURN_TYPE, FPU_IEEEDPDiv,
    AROS_LHAQUAD(double, y, D0, D1),
    AROS_LHAQUAD(double, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 14, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT
#if UseRegisterArgs
    QUAD * Res = (QUAD *)&y; /* this forces the returned value to be in D0/D1 */
    y = y/z;
    return * Res;
#else
    return (y/z);
#endif
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPDiv */

AROS_LHQUAD1(RETURN_TYPE, FPU_IEEEDPFloor,
    AROS_LHAQUAD(double, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 15, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT
    /*  return floor(y); */
    return 0xbadc0deULL;
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPFloor */

AROS_LHQUAD1(RETURN_TYPE, FPU_IEEEDPCeil,
    AROS_LHAQUAD(double, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 16, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT
    /*  return ceil(y); */
    return 0xbadc0deULL;
    AROS_LIBFUNC_EXIT
} /* FPU_IEEEDPCeil */
