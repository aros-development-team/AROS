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
      Calculate arcussin of the given number

    RESULT
      Motorola fast floating point number

      flags:
        zero     : result is zero
        negative : result is negative
        overflow : fnum &lt; -1  or  fnum &gt; 1

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LH1(float, SPAsin,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 19, MathTrans
)
{
    AROS_LIBFUNC_INIT

    /* 1> |x| >= 0.5 */
    LONG t,w,p,q,c,r,s,ix;
    ix = fnum1 & (FFPMantisse_Mask | FFPExponent_Mask); /* ix = |fnum| */
    
    if ((LONG)one == ix) /* |fnum1| = 1 -> result = +-(pi/2) */
    {
        return (pio2 | (fnum1 & FFPSign_Mask  ));
    }
    
    if (1 == SPCmp(ix,one)) /* |fnum1| > 1 */
    {
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return -1;
    }
    
    /* error: 1 ulp (unit in the last place)*/
    if (-1 == SPCmp(ix,onehalf)) /* |fnum1| < 0.5 */
    {
        if (-1 == SPCmp(ix,0xb89b6736)) /* |fnum1| < 70422/10000000 */
        {
            return fnum1;
        }
        
        t = SPMul(fnum1, fnum1);
        p = SPMul(t, SPAdd(pS0,
            SPMul(t, SPAdd(pS1,
            SPMul(t, SPAdd(pS2,
            SPMul(t, SPAdd(pS3,
            SPMul(t, SPAdd(pS4,
            SPMul(t, pS5)))))))))));
        q = SPAdd(one,
            SPMul(t, SPAdd(qS1,
            SPMul(t, SPAdd(qS2,
            SPMul(t, SPAdd(qS3,
            SPMul(t, qS4))))))));
        w = SPDiv(q, p);
        
        return SPAdd(fnum1, SPMul(fnum1, w));
    }
    
    
    w = SPSub(ix, one) ; /* w = 1 - fnum ; y = 1-x */
    t = SPMul(w, onehalf);  /* t = w / 2    ; z = y/2 */
    p = SPMul(t,SPAdd(pS0,
        SPMul(t,SPAdd(pS1,
        SPMul(t,SPAdd(pS2,
        SPMul(t,SPAdd(pS3,
        SPMul(t,SPAdd(pS4,
        SPMul(t,pS5)))))))))));
    q = SPAdd(one,
        SPMul(t,SPAdd(qS1,
        SPMul(t,SPAdd(qS2,
        SPMul(t,SPAdd(qS3,
        SPMul(t,qS4))))))));
    s = SPSqrt(t);      /* s = sqrt(t)  ; s = sqrt(z) */
    
    if(1 == SPCmp(ix, 0xf9999a40 /*0.975*/ )) /* |fnum| > 0.975 */
    {
        /*error: 2 ulp (4 ulp when |fnum| close to 1) */
        w = SPDiv(q,p); /* w = p / q; */
        /* res = pi/2-(2*(s+s*w)) */
        t = SPSub(SPMul(two,SPAdd(s,SPMul(s,w))),pio2);
        t = SPAdd(t, 0x8000002b); /* for better accuracy */
    }
    else
    {
        /* error: 2 ulp */
        w = s;
        c = SPDiv(SPAdd(s,w),SPSub(SPMul(w,w),t)); /* c=(t-w*w)/(s+w) */
        r = SPDiv(q,p);
        p = SPAdd(SPAdd(c,c),SPMul(SPAdd(s,s),r));
        q = SPSub(SPAdd(w,w) ,pio4);
        t = SPSub(SPSub(q,p) ,pio4);   
    }
    
    return (t | (fnum1 & FFPSign_Mask )) ;
    
    AROS_LIBFUNC_EXIT
}
