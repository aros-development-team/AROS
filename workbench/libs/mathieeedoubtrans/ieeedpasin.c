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

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPAsin,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, x, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 19, MathIeeeDoubTrans)

/*  FUNCTION
        Calculate the arcus sine of the IEEE double precision number

    INPUTS

    RESULT
        IEEE double precision floating point number

        flags:
	zero	 : result is zero
	negative : result is negative
	overflow : argument is out of range

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    QUAD t,w,p,q,c,r,s;
    int hx,ix;
    
    /*
        if this is a 32bit -compiler we need to define some variables, otherwise
        these are available as 64bit constants
    */
#   if defined AROS_64BIT_TYPE || defined __GNUC__
#   else
#       undef two_64
#       undef pio2_lo_64
#       undef one_64
        const QUAD two_64     = two,
                   pio2_lo_64 = pio2_lo,
                   one_64     = oneC;
#   endif
    
    hx = Get_High32of64(x);
    ix = hx & 0x7fffffff;
    if(ix >= 0x3ff00000)
    {    /* |x| >= 1 */
        if(( (ix-0x3ff00000) | Get_Low32of64(x) ) == 0)
        {  /* |x|==1 -> asin(1) = pi/2 */
            /* 
                if this is a 32bit -compiler we need to define some variables, otherwise
                these are available as 64bit constants
            */
            if (hx > 0)
            {
#               if defined AROS_64BIT_TYPE || defined __GNUC__
#               else
#                   undef pio2_hi_64
                    QUAD pio2_hi_64 = pio2_hi;
#               endif
                
                return pio2_hi_64;
            }
            else
            {
#               if defined AROS_64BIT_TYPE || defined __GNUC__
#               else
#                   undef neg_pio2_hi_64
                    QUAD neg_pio2_hi_64 = neg_pio2_hi;
#               endif
                
                return neg_pio2_hi_64;
            }
        }
        else  /* asin(>1) = NAN  */
        {
            /* 
                if this is a 32bit -compiler we need to define some variables,
                otherwise these are available as 64bit constants
            */
#           if defined AROS_64BIT_TYPE || defined __GNUC__
#           else
#               undef IEEEDPNAN_64
                QUAD IEEEDPNAN_64;
                Set_Value64C(IEEEDPNAN_64, IEEEDPNAN_Hi, IEEEDPNAN_Lo);
#           endif
            
            return IEEEDPNAN_64;	 /* acos(>1)= NAN */
        } /* else */
    } /* if */
    
    {
        /*
            if this is a 32bit -compiler we need to define some variables,
            otherwise these are available as 64bit constants
        */
        
#       if defined AROS_64BIT_TYPE || defined __GNUC__
#       else
#           undef pS0_64
#           undef pS1_64
#           undef pS2_64
#           undef pS3_64
#           undef pS4_64
#           undef pS5_64
#           undef qS1_64
#           undef qS2_64
#           undef qS3_64
#           undef qS4_64
        
#           undef pio2_hi_64
        
            /* Ok, let's define some constants */
            const QUAD pS0_64 = pS0,
                       pS1_64 = pS1,
                       pS2_64 = pS2,
                       pS3_64 = pS3,
                       pS4_64 = pS4,
                       pS5_64 = pS5;
            const QUAD qS1_64 = qS1,
                       qS2_64 = qS2,
                       qS3_64 = qS3,
                       qS4_64 = qS4;
            const QUAD pio2_hi_64 = pio2_hi;
#       endif
        
        if(ix < 0x3fe00000)
        {	  /* |x| < 0.5 */
            if(ix <= 0x3e400000) return x;/* if|x|<2**-27 -> asin(x)=x */
            
            Set_Value64(t, IEEEDPMul(x, x));
            Set_Value64
            (
                p, 
                IEEEDPMul(t, IEEEDPAdd(pS0_64,
                IEEEDPMul(t, IEEEDPAdd(pS1_64,
                IEEEDPMul(t, IEEEDPAdd(pS2_64,
                IEEEDPMul(t, IEEEDPAdd(pS3_64,
                IEEEDPMul(t, IEEEDPAdd(pS4_64,
                IEEEDPMul(t, pS5_64)))))))))))
            );
            
            Set_Value64
            (
                q, 
                IEEEDPAdd(one_64,
                IEEEDPMul(t, IEEEDPAdd(qS1_64,
                IEEEDPMul(t, IEEEDPAdd(qS2_64,
                IEEEDPMul(t, IEEEDPAdd(qS3_64,
                IEEEDPMul(t, qS4_64))))))))
            );
            Set_Value64(w, IEEEDPDiv(p, q));
            Set_Value64(t, IEEEDPAdd(x, IEEEDPMul(x, w)));
            
            return t;
        } /* if */
        {
            /*
                if this is a 32bit -compiler we need to define some variables,
                otherwise these are available as 64bit constants
            */
#           if defined AROS_64BIT_TYPE || defined __GNUC__
#           else
#               undef one_64
#               undef onehalf_64
#               undef two_64
#               undef pio2_hi_64
#               undef pio2_lo_64
#               undef pio4_hi_64
                const QUAD one_64     = oneC,
                           onehalf_64 = onehalf,
                           two_64     = two,
                           pio2_hi_64 = pio2_hi,
                           pio2_lo_64 = pio2_lo,
                           pio4_hi_64 = pio4_hi;
#           endif
            /* 1 > |x| >= 0.5 */
            AND64QC
            (
                x, 
                (IEEEDPMantisse_Mask_Hi | IEEEDPExponent_Mask_Hi),
                (IEEEDPMantisse_Mask_Lo | IEEEDPExponent_Mask_Lo)
            );
            Set_Value64(w, IEEEDPSub(one_64, x));
            Set_Value64(t, IEEEDPMul(w, onehalf_64));
            Set_Value64
            (
                p, 
                IEEEDPMul(t, IEEEDPAdd(pS0_64,
                IEEEDPMul(t, IEEEDPAdd(pS1_64,
                IEEEDPMul(t, IEEEDPAdd(pS2_64,
                IEEEDPMul(t, IEEEDPAdd(pS3_64,
                IEEEDPMul(t, IEEEDPAdd(pS4_64,
                IEEEDPMul(t, pS5_64)))))))))))
            );
            
            Set_Value64
            (
                q,
                IEEEDPAdd(one_64,
                IEEEDPMul(t, IEEEDPAdd(qS1_64,
                IEEEDPMul(t, IEEEDPAdd(qS2_64,
                IEEEDPMul(t, IEEEDPAdd(qS3_64,
                IEEEDPMul(t, qS4_64))))))))
            );
            Set_Value64(s, IEEEDPSqrt(t));
            
            if(ix >= 0x3fef3333) /* if |x| > 0.975 */
            {
                Set_Value64(w, IEEEDPDiv(p,q));
                Set_Value64
                (
                    t, 
                    IEEEDPSub
                    (
                        pio2_hi_64, IEEEDPSub
                        (
                            IEEEDPMul
                            (
                                two_64, IEEEDPAdd
                                (
                                    s, IEEEDPMul(s,w)
                                )
                            ),
                            pio2_lo_64
                        )
                    )
                );
            }
            else
            {
                Set_Value64(w,s);
                AND64QC(w,0xffffffff,0x0);
                Set_Value64
                (
                    c, IEEEDPDiv(IEEEDPSub(t, IEEEDPMul(w, w)), IEEEDPAdd(s,w))
                );
                Set_Value64(r, IEEEDPDiv(p,q));
                Set_Value64
                (
                    p, 
                    IEEEDPSub
                    (
                        IEEEDPMul(IEEEDPMul(two_64, s), r),
                        IEEEDPSub(pio2_lo_64, IEEEDPMul(two_64, c))
                    )
                );
                Set_Value64(q, IEEEDPSub(pio4_hi_64,IEEEDPMul(two_64,w)));
                Set_Value64(t, IEEEDPSub(pio4_hi_64,IEEEDPSub(p,q)));
            } /* else */
            if (hx > 0)
            {
                return t;
            }
            else
            {
                OR64QC(t, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
                return t;
            } /* else */
        }
    }

    AROS_LIBFUNC_EXIT
}
