/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
      Calculate e^x

    RESULT
      IEEE single precision number

      flags:
        zero     : result is zero
        negative : 0
        overflow : the result was out of range for the IEEE single precision
                   format

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      <code>
      e^(>= 89): return 0x7f800000;
      e^(2^(<=-24)): return one;
      </code>

    HISTORY
*/

AROS_LH1(float, IEEESPExp,
    AROS_LHA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 13, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT
    
    const LONG ExpTable[] =
    { 
        0x6da12cc2,  /* e^64  */
        0x568fa1fe,  /* e^32  */
        0x4b07975e,  /* e^16  */
        0x453a4f53,  /* e^8   */
        0x425a6481,  /* e^4   */
        0x40ec7325,  /* e^2   */
        0x402df854,  /* e^1   */
        0x3fd3094c,  /* e^(1/2) */
        0x3fa45af2,  /* e^(1/4) */
        0x3f910b02,  /* e^(1/8) */
        0x3f88415b,  /* e^(1/16) */
        0x3f84102b,  /* e^(1/32) */
        0x3f820405,  /* e^(1/64) */
        0x3f810101,  /* e^(1/128) */
        0x3f808040,  /* e^(1/256) */
        0x3f804010,  /* e^(1/512) */
        0x3f802004,  /* e^(1/1024) */
        0x3f801001,  /* e^(1/2048) */
        0x3f800800,  /* e^(1/4096) */
        0x3f800400,  /* e^(1/8192) */
        0x3f800200,  /* e^(1/16384) */
        0x3f800100,  /* e^(1/32768) */
        0x3f800080,  /* e^(1/65536) */
        0x3f800040,  /* e^(1/131072) */
        0x3f800020,  /* e^(1/262144) */
        0x3f800010,  /* e^(1/524288) */
        0x3f800008,  /* e^(1/1048576) */
        0x3f800004,  /* e^(1/2097152) */
        0x3f800002,  /* e^(1/4194304) */
        0x3f800001,  /* e^(1/8388608) */
    };
    ULONG Res, i;
    LONG Mantisse;
    char Exponent;   
    
    Exponent = ((y & IEEESPExponent_Mask) >> 23) -0x7f;
    
    /* e^0 = 1, e^(2^(<=-24)) = 1 */
    if ( 0 == y || Exponent <= -24 ) return one;
    
    /* e^(>= 89) = overflow) */
    if (Exponent > 6)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return IEEESP_Pinfty;
    }
    
    i = 6 - Exponent;
    
    Mantisse = (y & IEEESPMantisse_Mask) << 9;
    Res = ExpTable[i++];
    
    while ( 0 != Mantisse && i <= 29 )
    {
        /* is the highest bit set? */
        if ( Mantisse < 0 )
        {
            Res = IEEESPMul(Res, ExpTable[i]);
            if (0 == Res)
            {
                SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
                return Res;
            }
            if (IEEESP_Pinfty == Res)
            {
                SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
                return Res;
            }
        }
        i++;
        Mantisse <<= 1;
    }
    
    if ( y < 0) return IEEESPDiv(one, Res);
    
    return Res;

    AROS_LIBFUNC_EXIT
}
