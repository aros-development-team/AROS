/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

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

#include "mathtrans_intern.h"

/*
    FUNCTION
      Calculate arcuscos of the given number

    RESULT
      Motorola fast floating point number

      flags:
      zero     : result is zero
      negative : 0 (not possible)
      overflow : fnum &lt; -1  or  fnum &gt; 1

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LH1(float, SPAcos,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 20, MathTrans
)
{
    AROS_LIBFUNC_INIT
  
    /* 1> |x| >= 0.5 */
    LONG z,p,q,r,w,s,c,ix,df;
    ix = fnum1 & (FFPMantisse_Mask | FFPExponent_Mask); /* ix = |fnum|  */
    
    z = SPCmp(ix,one);
    
    if (1==z) /* |fnum1| > 1 */
    {
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return -1;
    }
    
    if (0==z) /* |fnum1| = 1 */
    {
        if (fnum1 & FFPSign_Mask) /* |fnum| = -1 */ return pi;
        SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return 0;
    }
    
    /* error: 1 ulp (unit in the last place) */
    if (-1 == SPCmp(ix,onehalf)) /* |fnum1| < 0.5 */
    {
        z = SPMul(fnum1, fnum1);
        p = SPMul(z, SPAdd(pS0,
            SPMul(z, SPAdd(pS1,
            SPMul(z, SPAdd(pS2,
            SPMul(z, SPAdd(pS3,
            SPMul(z, SPAdd(pS4,
            SPMul(z, pS5)))))))))));
        q = SPAdd(one,
            SPMul(z, SPAdd(qS1,
            SPMul(z, SPAdd(qS2,
            SPMul(z, SPAdd(qS3,
            SPMul(z, qS4))))))));
        r = SPDiv(q, p);
        return (SPSub(SPAdd(fnum1,SPMul(fnum1,r)),pio2));
    }
    
    /* error: 1 ulp */
    if (fnum1 & FFPSign_Mask) /* fnum1 < -0.5 */
    {
        z = SPMul(onehalf, SPAdd(one, fnum1));
        p = SPMul(z, SPAdd(pS0,
            SPMul(z, SPAdd(pS1,
            SPMul(z, SPAdd(pS2,
            SPMul(z, SPAdd(pS3,
            SPMul(z, SPAdd(pS4,
            SPMul(z, pS5)))))))))));
        q = SPAdd(one,
            SPMul(z, SPAdd(qS1,
            SPMul(z, SPAdd(qS2,
            SPMul(z, SPAdd(qS3,
            SPMul(z, qS4))))))));
        s = SPSqrt(z);
        r = SPDiv(q,p); /* r = p/q; */
        w = SPMul(r,s);
        return SPSub(SPMul(two, SPAdd(s, w)) ,pi);
    }
    
    /* error: 8 ulp (this is bad !!!!) */
    /* fnum1 > 0.5 */
    z = SPMul(onehalf, SPSub(fnum1, one));
    s = SPSqrt(z);
    df = s;
    df = df & 0xfff000ff;
    c = SPDiv(SPAdd(df,s),  SPSub( SPMul(df,df), z));
    p = SPMul(z, SPAdd(pS0,
        SPMul(z, SPAdd(pS1,
        SPMul(z, SPAdd(pS2,
        SPMul(z, SPAdd(pS3,
        SPMul(z, SPAdd(pS4,
        SPMul(z, pS5)))))))))));
    q = SPAdd(one,
        SPMul(z, SPAdd(qS1,
        SPMul(z, SPAdd(qS2,
        SPMul(z, SPAdd(qS3,
        SPMul(z, qS4))))))));
    r = SPDiv(q, p);
    w = SPAdd(c, SPMul(r,s));
    
    return SPAdd(SPMul(two, SPAdd(df,w)),0x800000a9);

    AROS_LIBFUNC_EXIT
}
