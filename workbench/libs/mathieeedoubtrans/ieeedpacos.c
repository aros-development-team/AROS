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

        AROS_LHQUAD1(double, IEEEDPAcos,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, x, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 20, MathIeeeDoubTrans)

/*  FUNCTION
        Calculate the arcus cosine of the IEEE double precision number

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

    QUAD z,p,q,r,w,s,c,df, tmp;
    int hx,ix;
    
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
    if (ix >= 0x3ff00000)
    {    /* |x| >= 1 */
        if (((ix-0x3ff00000) | Get_Low32of64(x)) == 0)
        {  /* |x|==1 */
            if(hx>0)
            {
                /* 
                    if this is a 32bit-compiler we need to define some 
                    variables, otherwise these are available as 64bit 
                    constants
                */
#               if defined AROS_64BIT_TYPE || defined __GNUC__
#               else
#                   undef zero_64
                    QUAD zero_64 = zero;
#               endif
    
                return zero_64;
            }   /* acos(1) = 0  */
            else
            {
                /*
                    if this is a 32bit-compiler we need to define some 
                    variables, otherwise these are available as 64bit constants
                */
#               if defined AROS_64BIT_TYPE || defined __GNUC__
#               else
#                   undef pi_64
                    QUAD pi_64 = pi;
#               endif
                
                return pi_64;	  /* acos(-1)= pi */
            }
        }
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
            
            return IEEEDPNAN_64; /* acos(|x|>1) is NaN */
        }
    }
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
            if(ix <= 0x3c600000) return pio2_hi_64;/*if|x|<2**-57*/
    
            Set_Value64(z, IEEEDPMul(x, x));
            Set_Value64
            (
                p, 
                IEEEDPMul(z, IEEEDPAdd(pS0_64,
                IEEEDPMul(z, IEEEDPAdd(pS1_64,
                IEEEDPMul(z, IEEEDPAdd(pS2_64,
                IEEEDPMul(z, IEEEDPAdd(pS3_64,
                IEEEDPMul(z, IEEEDPAdd(pS4_64,
                IEEEDPMul(z, pS5_64)))))))))))
            );
            
            Set_Value64
            (
                q,
                IEEEDPAdd(one_64,
                IEEEDPMul(z, IEEEDPAdd(qS1_64,
                IEEEDPMul(z, IEEEDPAdd(qS2_64,
                IEEEDPMul(z, IEEEDPAdd(qS3_64,
                IEEEDPMul(z, qS4_64))))))))
            );
            Set_Value64(r, IEEEDPDiv(p, q));
            
            Set_Value64
            (
                tmp, 
                IEEEDPSub(pio2_hi_64,
                IEEEDPSub(x,
                IEEEDPSub(pio2_lo_64, IEEEDPMul(x, r))))
            );
            return tmp;
        }
        else
        {
            /* 
                if this is a 32bit -compiler we need to define some variables,
                otherwise these are available as 64bit constants
            */
#           if defined AROS_64BIT_TYPE || defined __GNUC__
#           else
#               undef onehalf_64
                const QUAD onehalf_64 = onehalf;
#           endif
            
            if (hx < 0)
            { 	    /* x < -0.5 */
                /*
                    if this is a 32bit-compiler we need to define some 
                    variables, otherwise these are available as 64bit constants
                */
#               if defined AROS_64BIT_TYPE || defined __GNUC__
#               else
#                   undef pi_64
                    const QUAD pi_64 = pi;
#               endif
                
                Set_Value64(z, IEEEDPMul(IEEEDPAdd(one_64,x),onehalf_64));
                Set_Value64(p, IEEEDPMul(z, IEEEDPAdd(pS0_64,
                               IEEEDPMul(z, IEEEDPAdd(pS1_64,
                               IEEEDPMul(z, IEEEDPAdd(pS2_64,
                               IEEEDPMul(z, IEEEDPAdd(pS3_64,
                               IEEEDPMul(z, IEEEDPAdd(pS4_64,
                               IEEEDPMul(z, pS5_64))))))))))));
                Set_Value64(q, IEEEDPAdd(one_64,
                               IEEEDPMul(z, IEEEDPAdd(qS1_64,
                               IEEEDPMul(z, IEEEDPAdd(qS2_64,
                               IEEEDPMul(z, IEEEDPAdd(qS3_64,
                               IEEEDPMul(z,qS4_64)))))))));
                Set_Value64(s, IEEEDPSqrt(z));
                Set_Value64(r, IEEEDPDiv(p,q));
                Set_Value64(w, IEEEDPSub(IEEEDPMul(r,s),pio2_lo_64));
                return IEEEDPSub(pi_64, IEEEDPMul(two_64, IEEEDPAdd(s,w)));
            }
            else
            {  /* x > 0.5 */                
                Set_Value64(z, IEEEDPMul(IEEEDPSub(one_64,x),onehalf_64));
                Set_Value64(s, IEEEDPSqrt(z));
                Set_Value64(df,s);
                AND64QC(df,0xffffffff, 0x0);
                Set_Value64(c, IEEEDPDiv(IEEEDPSub(z,IEEEDPMul(df,df)),IEEEDPAdd(s,df)));
                Set_Value64(p, IEEEDPMul(z, IEEEDPAdd(pS0_64,
                               IEEEDPMul(z, IEEEDPAdd(pS1_64,
                               IEEEDPMul(z, IEEEDPAdd(pS2_64,
                               IEEEDPMul(z, IEEEDPAdd(pS3_64,
                               IEEEDPMul(z, IEEEDPAdd(pS4_64,
                               IEEEDPMul(z,pS5_64))))))))))));
                Set_Value64(q, IEEEDPAdd(one_64,
                               IEEEDPMul(z, IEEEDPAdd(qS1_64,
                               IEEEDPMul(z, IEEEDPAdd(qS2_64,
                               IEEEDPMul(z, IEEEDPAdd(qS3_64,
                               IEEEDPMul(z,qS4_64)))))))));
                Set_Value64(r, IEEEDPDiv(p,q));
                Set_Value64(w, IEEEDPAdd(IEEEDPMul(r,s),c));
                return IEEEDPMul(two_64,IEEEDPAdd(df,w));
            }
        }
    }

    AROS_LIBFUNC_EXIT
}
