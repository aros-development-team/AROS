/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/


#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/mathieeesingtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesingtrans_intern.h"


/*****************************************************************************

    NAME */

      AROS_LH1(float, IEEESPAcos,

/*  SYNOPSIS */

      AROS_LHA(float, y, D0),

/*  LOCATION */

      struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 20, Mathieeesingtrans)

/*  FUNCTION

      Calculate arcuscos of the given number

    INPUTS

      y - IEEE single precision floating point number

    RESULT

      IEEE single precision floating point number


      flags:
      zero     : result is zero
      negative : 0 (not possible)
      overflow : y < -1  or  y > 1

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.

 * ====================================================
*/
  /* 1> |x| >= 0.5 */
  LONG z,p,q,r,w,s,c,ix,df;
  ix = y & (IEEESPMantisse_Mask | IEEESPExponent_Mask); /* ix = |y|  */

  z = IEEESPCmp(ix,one);

   if (1==z) /* |y| > 1 */
   {
     SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
     return 0x7fffffff;
   }

   if (0==z) /* |y| = 1 */
   {
     if (y & IEEESPSign_Mask) /* y = -1 */
       return pi;
     SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
     return 0;
   }

  /* error: 1 ulp (unit in the last place) */
  if (-1 == IEEESPCmp(ix,onehalf)) /* |fnum1| < 0.5 */
  {
    z = IEEESPMul(y, y);
    p = IEEESPMul(z, IEEESPAdd(pS0,
        IEEESPMul(z, IEEESPAdd(pS1,
        IEEESPMul(z, IEEESPAdd(pS2,
        IEEESPMul(z, IEEESPAdd(pS3,
        IEEESPMul(z, IEEESPAdd(pS4,
        IEEESPMul(z, pS5)))))))))));
    q = IEEESPAdd(one,
        IEEESPMul(z, IEEESPAdd(qS1,
        IEEESPMul(z, IEEESPAdd(qS2,
        IEEESPMul(z, IEEESPAdd(qS3,
        IEEESPMul(z, qS4))))))));
    r = IEEESPDiv(p, q);
    return (IEEESPSub(pio2,IEEESPAdd(y,IEEESPMul(y,r))));
  }

  /* error: 1 ulp */
  if (y & IEEESPSign_Mask) /* y < -0.5 */
  {
    z = IEEESPMul(onehalf, IEEESPAdd(one, y));
    p = IEEESPMul(z, IEEESPAdd(pS0,
        IEEESPMul(z, IEEESPAdd(pS1,
        IEEESPMul(z, IEEESPAdd(pS2,
        IEEESPMul(z, IEEESPAdd(pS3,
        IEEESPMul(z, IEEESPAdd(pS4,
        IEEESPMul(z, pS5)))))))))));
    q = IEEESPAdd(one,
        IEEESPMul(z, IEEESPAdd(qS1,
        IEEESPMul(z, IEEESPAdd(qS2,
        IEEESPMul(z, IEEESPAdd(qS3,
        IEEESPMul(z, qS4))))))));
    s = IEEESPSqrt(z);
    r = IEEESPDiv(p,q); /* r = p/q; */
    w = IEEESPMul(r,s);
    return IEEESPSub(pi, IEEESPMul(two, IEEESPAdd(s, w)));
  }

  /* fnum1 > 0.5 */
  /* error : 1 ulp */
  z = IEEESPMul(onehalf, IEEESPSub(one, y));
  s = IEEESPSqrt(z);
  df = s;
  //df = df & 0xfff000ff;
  c = IEEESPDiv(IEEESPSub(z, IEEESPMul(df,df)), IEEESPAdd(df,s) );
  p = IEEESPMul(z, IEEESPAdd(pS0,
      IEEESPMul(z, IEEESPAdd(pS1,
      IEEESPMul(z, IEEESPAdd(pS2,
      IEEESPMul(z, IEEESPAdd(pS3,
      IEEESPMul(z, IEEESPAdd(pS4,
      IEEESPMul(z, pS5)))))))))));
  q = IEEESPAdd(one,
      IEEESPMul(z, IEEESPAdd(qS1,
      IEEESPMul(z, IEEESPAdd(qS2,
      IEEESPMul(z, IEEESPAdd(qS3,
      IEEESPMul(z, qS4))))))));
  r = IEEESPDiv(p, q);
  w = IEEESPAdd(c, IEEESPMul(r,s));
  return IEEESPMul(two, IEEESPAdd(df,w));
AROS_LIBFUNC_EXIT
} /* IEEESPAcos */
