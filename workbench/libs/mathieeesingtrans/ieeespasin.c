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

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
      Calculate arcussin of the given number

    RESULT
      IEEE single precision floating point number


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

AROS_LH1(float, IEEESPAsin,
    AROS_LHA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 19, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT

    /* 1> |x| >= 0.5 */
    LONG t,w,p,q,c,r,s,ix;
    ix = y & (IEEESPMantisse_Mask | IEEESPExponent_Mask); /* ix = |fnum| */
    
    if (one == ix) /* |y| = 1 -> result = +-(pi/2) */
    {
        return (pio2 | (y & IEEESPSign_Mask  ));
    }
    
    if (1 == IEEESPCmp(ix,one)) /* |y| > 1 */
    {
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return 0x7fffffff;
    }
    
    /* error: 1 ulp (unit in the last place)*/
    if ( -1 == IEEESPCmp(ix, onehalf)) /* |y| < 0.5 */
    {
        t = IEEESPMul(y, y);
        p = IEEESPMul(t, IEEESPAdd(pS0,
            IEEESPMul(t, IEEESPAdd(pS1,
            IEEESPMul(t, IEEESPAdd(pS2,
            IEEESPMul(t, IEEESPAdd(pS3,
            IEEESPMul(t, IEEESPAdd(pS4,
            IEEESPMul(t, pS5) ))))))))));
        q = IEEESPAdd(one,
            IEEESPMul(t, IEEESPAdd(qS1,
            IEEESPMul(t, IEEESPAdd(qS2,
            IEEESPMul(t, IEEESPAdd(qS3,
            IEEESPMul(t, qS4))))))));
        w = IEEESPDiv(p, q);
        
        return IEEESPAdd(y, IEEESPMul(y, w));
    }
    
    
    w = IEEESPSub(one, ix) ; /* w = 1 - fnum ; y = 1-x */
    t = IEEESPMul(w, onehalf);  /* t = w / 2    ; z = y/2 */
    p = IEEESPMul(t,IEEESPAdd(pS0,
        IEEESPMul(t,IEEESPAdd(pS1,
        IEEESPMul(t,IEEESPAdd(pS2,
        IEEESPMul(t,IEEESPAdd(pS3,
        IEEESPMul(t,IEEESPAdd(pS4,
        IEEESPMul(t,pS5)))))))))));
    q = IEEESPAdd(one,
        IEEESPMul(t,IEEESPAdd(qS1,
        IEEESPMul(t,IEEESPAdd(qS2,
        IEEESPMul(t,IEEESPAdd(qS3,
        IEEESPMul(t,qS4))))))));
    s = IEEESPSqrt(t);      /* s = sqrt(t)  ; s = sqrt(z) */
    
    if(1 == IEEESPCmp(ix, 0x3f79999a /*0.975*/ )) /* |fnum| > 0.975 */
    {
        /*error: 2 ulp (4 ulp when |fnum| close to 1) */
        w = IEEESPDiv(p, q); /* w = p / q; */
        /* res = pi/2-(2*(s+s*w)) */
        t = IEEESPSub(pio2, IEEESPMul(two,IEEESPAdd(s,IEEESPMul(s,w))));
    }
    else
    {
        /* error: 2 ulp */
        w = s;
        c = IEEESPDiv(IEEESPSub(t,IEEESPMul(w,w)),IEEESPAdd(s,w)); /* c=(t-w*w)/(s+w) */
        r = IEEESPDiv(p,q);
        p = IEEESPAdd(IEEESPAdd(c,c),IEEESPMul(IEEESPAdd(s,s),r));
        q = IEEESPSub(pio4, IEEESPAdd(w,w));
        t = IEEESPSub(pio4, IEEESPSub(p,q));
    }
    
    return (t | (y & IEEESPSign_Mask )) ;

    AROS_LIBFUNC_EXIT
}
