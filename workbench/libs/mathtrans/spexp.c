/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*
    FUNCTION
      Calculate e^x

    RESULT
      Motorola fast floating point number

      flags:
        zero     : result is zero
        negative : 0
        overflow : the result was out of range for the ffp-format

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
     <code>
      e^(>= 44): return FFP_Pinfty;
     </code>

    HISTORY
*/

AROS_LH1(float, SPExp,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 13, MathTrans
)
{
    AROS_LIBFUNC_INIT
    
    const LONG ExpTable[] =
    { 
        0x8fa1fe6f,  /* e^32  */
        0x87975e58,  /* e^16  */
        0xba4f534c,  /* e^8   */
        0xda648146,  /* e^4   */
        0xec732543,  /* e^2   */
        0xadf85442,  /* e^1   */
        0xD3094C41,  /* e^(1/2) */
        0xA45aF241,  /* e^(1/4) */
        0x910b0241,  /* e^(1/8) */
        0x88415b41,  /* e^(1/16) */
        0x84102b41,  /* e^(1/32) */
        0x82040541,  /* e^(1/64) */
        0x81010141,  /* e^(1/128) */
        0x80804041,  /* e^(1/256) */
        0x80401041,  /* e^(1/512) */
        0x80200441,  /* e^(1/1024) */
        0x80100141,  /* e^(1/2048) */
        0x80080041,  /* e^(1/4096) */
        0x80040041,  /* e^(1/8192) */
        0x80020041,  /* e^(1/16384) */
        0x80010041,  /* e^(1/32768) */
        0x80008041,  /* e^(1/65536) */
        0x80004041,  /* e^(1/131072) */
        0x80002041,  /* e^(1/262144) */
        0x80001041,  /* e^(1/524288) */
        0x80000841,  /* e^(1/1048576) */
        0x80000441,  /* e^(1/2097152) */
        0x80000241,  /* e^(1/4194304) */
        0x80000141   /* e^(1/8388608) */
    };
    
    ULONG Res, i;
    LONG Mantisse;
    char Exponent;

    Exponent = (fnum1 & FFPExponent_Mask) - 0x41;

    /* e^0 = 1, e^(2^(<=-24)) = 1 */
    if ( 0 == fnum1 || Exponent <= -24 ) return one;
    
    /* e^(>= 44) = overflow = infinity) */
    if (Exponent > 5)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return FFP_Pinfty;
    }
    
    i = 5 - Exponent;
    
    Mantisse = (fnum1 & FFPMantisse_Mask);
    Res = ExpTable[i++];
    Mantisse <<= 1;
    
    while ( 0 != Mantisse && i <= 28 )
    {
        /* is the highest bit set? */
        if ( Mantisse < 0 )
        {
            Res = SPMul(Res, ExpTable[i]);
            if (0 == Res)
            {
                SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
                return Res;
            }
            if (FFP_Pinfty == Res)
            {
                SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
                return Res;
            }
        }
        i++;
        Mantisse <<= 1;
    }
    
    if ( (char) fnum1 < 0) return SPDiv(one, Res);
    
    return Res;

    AROS_LIBFUNC_EXIT
}
