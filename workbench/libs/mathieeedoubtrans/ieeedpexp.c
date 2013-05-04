/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPExp,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 13, MathIeeeDoubTrans)

/*  FUNCTION
        Calculate e^x

    INPUTS

    RESULT
        IEEE double precision number

        flags:
        zero     : result is zero
        negative : 0
        overflow : the result was out of range for the IEEE single precision
                   format

    BUGS

    INTERNALS
        e^(>= ): return 0x7f800000;
        e^(2^(<=-24)): return one;

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    const QUAD ExpTable[] =
    {
        QuadData(0x6e194765, 0x04ba852e,0x6e19476504ba852eULL) , /* e^(2^9)  */
        QuadData(0x57041c7a, 0x8814beba,0x57041c7a8814bebaULL) , /* e^(2^8)  */
        QuadData(0x4b795e54, 0xc5dd4217,0x4b795e54c5dd4217ULL) , /* e^(2^7)  */
        QuadData(0x45b42598, 0x2cf597cd,0x45b425982cf597cdULL) , /* e^(2^6)  */
        QuadData(0x42d1f43f, 0xcc4b662c,0x42d1f43fcc4b662cULL) , /* e^(2^5)  */
        QuadData(0x4160f2eb, 0xd0a80020,0x4160f2ebd0a80020ULL) , /* e^(2^4)  */
        QuadData(0x40a749ea, 0x7d470c6d,0x40a749ea7d470c6dULL) , /* e^(2^3)  */
        QuadData(0x404b4c90, 0x2e273a58,0x404b4c902e273a58ULL) , /* e^(2^2)  */
        QuadData(0x401d8e64, 0xb8d4ddad,0x401d8e64b8d4ddadULL) , /* e^(2^1)  */
        QuadData(0x4005bf0a, 0x8b145769,0x4005bf0a8b145769ULL) , /* e^(2^0)  */
        QuadData(0x3ffa6129, 0x8e1e069b,0x3ffa61298e1e069bULL) , /* e^(2^-1) */
        QuadData(0x3ff48b5e, 0x3c3e8186,0x3ff48b5e3c3e8186ULL) , /* e^(2^-2) */
        QuadData(0x3ff22160, 0x45b6f5cc,0x3ff2216045b6f5ccULL) , /* e^(2^-3) */
        QuadData(0x3ff1082b, 0x577d34ed,0x3ff1082b577d34edULL) , /* e^(2^-4) */
        QuadData(0x3ff08205, 0x601127ec,0x3ff08205601127ecULL) , /* e^(2^-5) */
        QuadData(0x3ff04080, 0xab55de39,0x3ff04080ab55de39ULL) , /* e^(2^-6) */
        QuadData(0x3ff02020, 0x15600445,0x3ff0202015600445ULL) , /* e^(2^-7) */
        QuadData(0x3ff01008, 0x02ab5577,0x3ff0100802ab5577ULL) , /* e^(2^-8) */
        QuadData(0x3ff00802, 0x00556001,0x3ff0080200556001ULL) , /* e^(2^-9) */
        QuadData(0x3ff00400, 0x800aabff,0x3ff00400800aabffULL) , /* e^(2^-10) */
        QuadData(0x3ff00200, 0x2001ff60,0x3ff002002001ff60ULL) , /* e^(2^-11) */
        QuadData(0x3ff00100, 0x08002aab,0x3ff0010008002aabULL) , /* e^(2^-12) */
        QuadData(0x3ff00080, 0x02000555,0x3ff0008002000555ULL) , /* e^(2^-13) */
        QuadData(0x3ff00040, 0x008000aa,0x3ff00040008000aaULL) , /* e^(2^-14) */
        QuadData(0x3ff00020, 0x00200015,0x3ff0002000200015ULL) , /* e^(2^-15) */
        QuadData(0x3ff00010, 0x00080002,0x3ff0001000080002ULL) , /* e^(2^-16) */
        QuadData(0x3ff00008, 0x00020000,0x3ff0000800020000ULL) , /* e^(2^-17) */
        QuadData(0x3ff00004, 0x00008000,0x3ff0000400008000ULL) , /* e^(2^-18) */
        QuadData(0x3ff00002, 0x00002000,0x3ff0000200002000ULL) , /* e^(2^-19) */
        QuadData(0x3ff00001, 0x00000800,0x3ff0000100000800ULL) , /* e^(2^-20) */
        QuadData(0x3ff00000, 0x80000200,0x3ff0000080000200ULL) , /* e^(2^-21) */
        QuadData(0x3ff00000, 0x40000080,0x3ff0000040000080ULL) , /* e^(2^-22) */
        QuadData(0x3ff00000, 0x20000020,0x3ff0000020000020ULL) , /* e^(2^-23) */
        QuadData(0x3ff00000, 0x10000008,0x3ff0000010000008ULL) , /* e^(2^-24) */
        QuadData(0x3ff00000, 0x08000002,0x3ff0000008000002ULL) , /* e^(2^-25) */
        QuadData(0x3ff00000, 0x04000000,0x3ff0000004000000ULL) , /* e^(2^-26) */
        QuadData(0x3ff00000, 0x02000000,0x3ff0000002000000ULL) , /* e^(2^-27) */
        QuadData(0x3ff00000, 0x01000000,0x3ff0000001000000ULL) , /* e^(2^-28) */
        QuadData(0x3ff00000, 0x00800000,0x3ff0000000800000ULL) , /* e^(2^-29) */
        QuadData(0x3ff00000, 0x00400000,0x3ff0000000400000ULL) , /* e^(2^-30) */
        QuadData(0x3ff00000, 0x00200000,0x3ff0000000200000ULL) , /* e^(2^-31) */
        QuadData(0x3ff00000, 0x00100000,0x3ff0000000100000ULL) , /* e^(2^-32) */
        QuadData(0x3ff00000, 0x00080000,0x3ff0000000080000ULL) , /* e^(2^-33) */
        QuadData(0x3ff00000, 0x00040000,0x3ff0000000040000ULL) , /* e^(2^-34) */
        QuadData(0x3ff00000, 0x00020000,0x3ff0000000020000ULL) , /* e^(2^-35) */
        QuadData(0x3ff00000, 0x00010000,0x3ff0000000010000ULL) , /* e^(2^-36) */
        QuadData(0x3ff00000, 0x00008000,0x3ff0000000008000ULL) , /* e^(2^-37) */
        QuadData(0x3ff00000, 0x00004000,0x3ff0000000004000ULL) , /* e^(2^-38) */
        QuadData(0x3ff00000, 0x00002000,0x3ff0000000002000ULL) , /* e^(2^-39) */
        QuadData(0x3ff00000, 0x00001000,0x3ff0000000001000ULL) , /* e^(2^-40) */
        QuadData(0x3ff00000, 0x00000800,0x3ff0000000000800ULL) , /* e^(2^-41) */
        QuadData(0x3ff00000, 0x00000400,0x3ff0000000000400ULL) , /* e^(2^-42) */
        QuadData(0x3ff00000, 0x00000200,0x3ff0000000000200ULL) , /* e^(2^-43) */
        QuadData(0x3ff00000, 0x00000100,0x3ff0000000000100ULL) , /* e^(2^-44) */
        QuadData(0x3ff00000, 0x00000080,0x3ff0000000000080ULL) , /* e^(2^-45) */
        QuadData(0x3ff00000, 0x00000040,0x3ff0000000000040ULL) , /* e^(2^-46) */
        QuadData(0x3ff00000, 0x00000020,0x3ff0000000000020ULL) , /* e^(2^-47) */
        QuadData(0x3ff00000, 0x00000010,0x3ff0000000000010ULL) , /* e^(2^-48) */
        QuadData(0x3ff00000, 0x00000008,0x3ff0000000000008ULL) , /* e^(2^-49) */
        QuadData(0x3ff00000, 0x00000004,0x3ff0000000000004ULL) , /* e^(2^-50) */
        QuadData(0x3ff00000, 0x00000002,0x3ff0000000000002ULL) , /* e^(2^-51) */
        QuadData(0x3ff00000, 0x00000001,0x3ff0000000000001ULL) , /* e^(2^-52) */
        QuadData(0x11c44109, 0xedb20931,0x11c44109edb20931ULL) , /* e^(-2^9)  */
        QuadData(0x28d97559, 0x56ad4e9b,0x28d9755956ad4e9bULL) , /* e^(-2^8)  */
        QuadData(0x34642eb9, 0xf39afb0b,0x34642eb9f39afb0bULL) , /* e^(-2^7)  */
        QuadData(0x3a2969d4, 0x7321e4cb,0x3a2969d47321e4cbULL) , /* e^(-2^6)  */
        QuadData(0x3d0c8464, 0xf7616468,0x3d0c8464f7616468ULL) , /* e^(-2^5)  */
        QuadData(0x3e7e355b, 0xbaee85ca,0x3e7e355bbaee85caULL) , /* e^(-2^4)  */
        QuadData(0x3f35fc21, 0x041027ac,0x3f35fc21041027acULL) , /* e^(-2^3)  */
        QuadData(0x3f92c155, 0xb8213cf4,0x3f92c155b8213cf4ULL) , /* e^(-2^2)  */
        QuadData(0x3fc152aa, 0xa3bf81cb,0x3fc152aaa3bf81cbULL) , /* e^(-2^1)  */
        QuadData(0x3fd78b56, 0x362cef37,0x3fd78b56362cef37ULL) , /* e^(-2^0)  */
        QuadData(0x3fe368b2, 0xfc6f9609,0x3fe368b2fc6f9609ULL) , /* e^(-2^-1)  */
        QuadData(0x3fe8ebef, 0x9eac820a,0x3fe8ebef9eac820aULL) , /* e^(-2^-2)  */
        QuadData(0x3fec3d6a, 0x24ed8221,0x3fec3d6a24ed8221ULL) , /* e^(-2^-3)  */
        QuadData(0x3fee0fab, 0xfbc702a3,0x3fee0fabfbc702a3ULL) , /* e^(-2^-4)  */
        QuadData(0x3fef03f5, 0x6a88b5d7,0x3fef03f56a88b5d7ULL) , /* e^(-2^-5)  */
        QuadData(0x3fef80fe, 0xabfeefa4,0x3fef80feabfeefa4ULL) , /* e^(-2^-6)  */
        QuadData(0x3fefc03f, 0xd56aa224,0x3fefc03fd56aa224ULL) , /* e^(-2^-7)  */
        QuadData(0x3fefe00f, 0xfaabffbb,0x3fefe00ffaabffbbULL) , /* e^(-2^-8)  */
        QuadData(0x3feff003, 0xff556aa8,0x3feff003ff556aa8ULL) , /* e^(-2^-9)  */
        QuadData(0x3feff800, 0xffeaabff,0x3feff800ffeaabffULL) , /* e^(-2^-10)  */
        QuadData(0x3feffc00, 0x3ffd556a,0x3feffc003ffd556aULL) , /* e^(-2^-11)  */
        QuadData(0x3feffe00, 0x0fffaaab,0x3feffe000fffaaabULL) , /* e^(-2^-12)  */
        QuadData(0x3fefff00, 0x03fff555,0x3fefff0003fff555ULL) , /* e^(-2^-13)  */
        QuadData(0x3fefff80, 0x00fffeaa,0x3fefff8000fffeaaULL) , /* e^(-2^-14)  */
        QuadData(0x3fefffc0, 0x003fffd5,0x3fefffc0003fffd5ULL) , /* e^(-2^-15)  */
        QuadData(0x3fefffe0, 0x000ffffa,0x3fefffe0000ffffaULL) , /* e^(-2^-16)  */
        QuadData(0x3feffff0, 0x0003ffff,0x3feffff00003ffffULL) , /* e^(-2^-17)  */
        QuadData(0x3feffff8, 0x000ffffl,0x3feffff8000fffffULL) , /* e^(-2^-18)  */
        QuadData(0x3feffffc, 0x00003fff,0x3feffffc00003fffULL) , /* e^(-2^-19)  */
        QuadData(0x3feffffe, 0x00000fff,0x3feffffe00000fffULL) , /* e^(-2^-20)  */
        QuadData(0x3fefffff, 0x000003ff,0x3fefffff000003ffULL) , /* e^(-2^-21)  */
        QuadData(0x3fefffff, 0x800000ff,0x3fefffff800000ffULL) , /* e^(-2^-22)  */
        QuadData(0x3fefffff, 0xc000003f,0x3fefffffc000003fULL) , /* e^(-2^-23)  */
        QuadData(0x3fefffff, 0xe000000f,0x3fefffffe000000fULL) , /* e^(-2^-24)  */
        QuadData(0x3fefffff, 0xf0000003,0x3feffffff0000003ULL) , /* e^(-2^-25)  */
        QuadData(0x3fefffff, 0xf8000000,0x3feffffff8000000ULL) , /* e^(-2^-26)  */
        QuadData(0x3fefffff, 0xfc000000,0x3feffffffc000000ULL) , /* e^(-2^-27)  */
        QuadData(0x3fefffff, 0xfe000000,0x3feffffffe000000ULL) , /* e^(-2^-28)  */
        QuadData(0x3fefffff, 0xff000000,0x3fefffffff000000ULL) , /* e^(-2^-29)  */
        QuadData(0x3fefffff, 0xff800000,0x3fefffffff800000ULL) , /* e^(-2^-30)  */
        QuadData(0x3fefffff, 0xffc00000,0x3fefffffffc00000ULL) , /* e^(-2^-31)  */
        QuadData(0x3fefffff, 0xffe00000,0x3fefffffffe00000ULL) , /* e^(-2^-32)  */
        QuadData(0x3fefffff, 0xfff00000,0x3feffffffff00000ULL) , /* e^(-2^-33)  */
        QuadData(0x3fefffff, 0xfff80000,0x3feffffffff80000ULL) , /* e^(-2^-34)  */
        QuadData(0x3fefffff, 0xfffc0000,0x3feffffffffc0000ULL) , /* e^(-2^-35)  */
        QuadData(0x3fefffff, 0xfffe0000,0x3feffffffffe0000ULL) , /* e^(-2^-36)  */
        QuadData(0x3fefffff, 0xffff0000,0x3fefffffffff0000ULL) , /* e^(-2^-37)  */
        QuadData(0x3fefffff, 0xffff8000,0x3fefffffffff8000ULL) , /* e^(-2^-38)  */
        QuadData(0x3fefffff, 0xffffc000,0x3fefffffffffc000ULL) , /* e^(-2^-39)  */
        QuadData(0x3fefffff, 0xffffe000,0x3fefffffffffe000ULL) , /* e^(-2^-40)  */
        QuadData(0x3fefffff, 0xfffff000,0x3feffffffffff000ULL) , /* e^(-2^-41)  */
        QuadData(0x3fefffff, 0xfffff800,0x3feffffffffff800ULL) , /* e^(-2^-42)  */
        QuadData(0x3fefffff, 0xfffffc00,0x3feffffffffffc00ULL) , /* e^(-2^-43)  */
        QuadData(0x3fefffff, 0xfffffe00,0x3feffffffffffe00ULL) , /* e^(-2^-44)  */
        QuadData(0x3fefffff, 0xffffff00,0x3fefffffffffff00ULL) , /* e^(-2^-45)  */
        QuadData(0x3fefffff, 0xffffff80,0x3fefffffffffff80ULL) , /* e^(-2^-46)  */
        QuadData(0x3fefffff, 0xffffffc0,0x3fefffffffffffc0ULL) , /* e^(-2^-47)  */
        QuadData(0x3fefffff, 0xffffffe0,0x3fefffffffffffe0ULL) , /* e^(-2^-48)  */
        QuadData(0x3fefffff, 0xfffffff0,0x3feffffffffffff0ULL) , /* e^(-2^-49)  */
        QuadData(0x3fefffff, 0xfffffff8,0x3feffffffffffff8ULL) , /* e^(-2^-50)  */
        QuadData(0x3fefffff, 0xfffffffc,0x3feffffffffffffcULL) , /* e^(-2^-51)  */
        QuadData(0x3fefffff, 0xfffffffe,0x3feffffffffffffeULL) , /* e^(-2^-52)  */
        QuadData(0x3fefffff, 0xffffffff,0x3fefffffffffffffULL)   /* e^(-2^-53)  */
    };
    QUAD Res;
    QUAD Mantisse;
    LONG Exponent;
    int i, Offset;
    
    Exponent = ((Get_High32of64(y) & IEEEDPExponent_Mask_Hi) >> 20) - 0x3ff;
    
    /* e^(+-0) = 1, e^(2^(<=-24)) = 1 */
    if (is_eqC(y, 0x0, 0x0) || is_eqC(y, 0x80000000, 0x0) || Exponent < -52 )
    {
        Set_Value64C(Res, 0x3ff00000, 0x0);
        return Res;
    }
    
    if ( is_lessSC(y, 0x0, 0x0)) /* y < 0 */
    {
        if (Exponent >= 10)
        {
            SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            Set_Value64C(Res , 0x0, 0x0);
            return Res;
        }
        Offset = 62;
    }
    else
    {
        if (Exponent >= 10)
        {
            SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            Set_Value64C(Res , IEEEDPPInfty_Hi, IEEEDPPInfty_Lo);
            return Res;
        }
        Offset = 0;
    }
    
    /* e^(>= 1024) = overflow) */
    
    i = 9 - Exponent + Offset;
    
    /* Mantisse = (y & IEEEDPMantisse_Mask) << 9; */
    Set_Value64(Mantisse, y);
    AND64QC(y, IEEEDPMantisse_Mask_Lo, IEEEDPMantisse_Mask_Hi);
    
    SHL64(Mantisse, Mantisse, 12);
    Res = ExpTable[i++];
    
    while ( is_neqC(Mantisse, 0x0, 0x0) && i <= (61+Offset) )
    {
        /* is the highest bit set? */
        if ( is_lessSC(Mantisse, 0x0, 0x0) /* Mantisse < 0 */ )
        {
            Res = IEEEDPMul(Res, ExpTable[i]);
            
            
            if (is_eqC(Res, 0x0, 0x0) /* 0 == Res */ )
            {
                SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
                return Res;
            }
            
            if (is_eqC(Res, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo))
            {
                SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
                return Res;
            }
        }
        i++;
        SHL64(Mantisse, Mantisse, 1);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
