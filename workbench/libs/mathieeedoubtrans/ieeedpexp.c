/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/


#include <aros/libcall.h>
#include <proto/mathieeedoubbas.h>
#include <proto/mathieeedoubtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeedoubtrans_intern.h"


/*****************************************************************************

    NAME */

      AROS_LHQUAD1(QUAD, IEEEDPExp,

/*  SYNOPSIS */

      AROS_LHAQUAD(QUAD, y, D0, D1),

/*  LOCATION */

      struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 13, Mathieeedoubtrans)

/*  FUNCTION

      Calculate e^x

    INPUTS

      y - IEEE double precision number

    RESULT

      IEEE double precision number


      flags:
        zero     : result is zero
        negative : 0
        overflow : the result was out of range for the IEEE double precision
                   format

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      e^(>=2^10 ): return pos. infinity;
      e^(2^(<=-24)): return one;

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT

#define QuadData(A,B) A, B

const QUAD ExpTable[] =
    {
      QuadData(0x6e194765, 0x04ba852e) , /* e^(2^9)  */
      QuadData(0x57041c7a, 0x8814beba) , /* e^(2^8)  */
      QuadData(0x4b795e54, 0xc5dd4217) , /* e^(2^7)  */
      QuadData(0x45b42598, 0x2cf597cd) , /* e^(2^6)  */
      QuadData(0x42d1f43f, 0xcc4b662c) , /* e^(2^5)  */
      QuadData(0x4160f2eb, 0xd0a80020) , /* e^(2^4)  */
      QuadData(0x40a749ea, 0x7d470c6d) , /* e^(2^3)  */
      QuadData(0x404b4c90, 0x2e273a58) , /* e^(2^2)  */
      QuadData(0x401d8e64, 0xb8d4ddad) , /* e^(2^1)  */
      QuadData(0x4005bf0a, 0x8b145769) , /* e^(2^0)  */
      QuadData(0x3ffa6129, 0x8e1e069b) , /* e^(2^-1) */
      QuadData(0x3ff48b5e, 0x3c3e8186) , /* e^(2^-2) */
      QuadData(0x3ff22160, 0x45b6f5cc) , /* e^(2^-3) */
      QuadData(0x3ff1082b, 0x577d34ed) , /* e^(2^-4) */
      QuadData(0x3ff08205, 0x601127ec) , /* e^(2^-5) */
      QuadData(0x3ff04080, 0xab55de39) , /* e^(2^-6) */
      QuadData(0x3ff02020, 0x15600445) , /* e^(2^-7) */
      QuadData(0x3ff01008, 0x02ab5577) , /* e^(2^-8) */
      QuadData(0x3ff00802, 0x00556001) , /* e^(2^-9) */
      QuadData(0x3ff00400, 0x800aabff) , /* e^(2^-10) */
      QuadData(0x3ff00200, 0x2001ff60) , /* e^(2^-11) */
      QuadData(0x3ff00100, 0x08002aab) , /* e^(2^-12) */
      QuadData(0x3ff00080, 0x02000555) , /* e^(2^-13) */
      QuadData(0x3ff00040, 0x008000aa) , /* e^(2^-14) */
      QuadData(0x3ff00020, 0x00200015) , /* e^(2^-15) */
      QuadData(0x3ff00010, 0x00080002) , /* e^(2^-16) */
      QuadData(0x3ff00008, 0x00020000) , /* e^(2^-17) */
      QuadData(0x3ff00004, 0x00008000) , /* e^(2^-18) */
      QuadData(0x3ff00002, 0x00002000) , /* e^(2^-19) */
      QuadData(0x3ff00001, 0x00000800) , /* e^(2^-20) */
      QuadData(0x3ff00000, 0x80000200) , /* e^(2^-21) */
      QuadData(0x3ff00000, 0x40000080) , /* e^(2^-22) */
      QuadData(0x3ff00000, 0x20000020) , /* e^(2^-23) */
      QuadData(0x3ff00000, 0x10000008) , /* e^(2^-24) */
      QuadData(0x3ff00000, 0x08000002) , /* e^(2^-25) */
      QuadData(0x3ff00000, 0x04000000) , /* e^(2^-26) */
      QuadData(0x3ff00000, 0x02000000) , /* e^(2^-27) */
      QuadData(0x3ff00000, 0x01000000) , /* e^(2^-28) */
      QuadData(0x3ff00000, 0x00800000) , /* e^(2^-29) */
      QuadData(0x3ff00000, 0x00400000) , /* e^(2^-30) */
      QuadData(0x3ff00000, 0x00200000) , /* e^(2^-31) */
      QuadData(0x3ff00000, 0x00100000) , /* e^(2^-32) */
      QuadData(0x3ff00000, 0x00080000) , /* e^(2^-33) */
      QuadData(0x3ff00000, 0x00040000) , /* e^(2^-34) */
      QuadData(0x3ff00000, 0x00020000) , /* e^(2^-35) */
      QuadData(0x3ff00000, 0x00010000) , /* e^(2^-36) */
      QuadData(0x3ff00000, 0x00008000) , /* e^(2^-37) */
      QuadData(0x3ff00000, 0x00004000) , /* e^(2^-38) */
      QuadData(0x3ff00000, 0x00002000) , /* e^(2^-39) */
      QuadData(0x3ff00000, 0x00001000) , /* e^(2^-40) */
      QuadData(0x3ff00000, 0x00000800) , /* e^(2^-41) */
      QuadData(0x3ff00000, 0x00000400) , /* e^(2^-42) */
      QuadData(0x3ff00000, 0x00000200) , /* e^(2^-43) */
      QuadData(0x3ff00000, 0x00000100) , /* e^(2^-44) */
      QuadData(0x3ff00000, 0x00000080) , /* e^(2^-45) */
      QuadData(0x3ff00000, 0x00000040) , /* e^(2^-46) */
      QuadData(0x3ff00000, 0x00000020) , /* e^(2^-47) */
      QuadData(0x3ff00000, 0x00000010) , /* e^(2^-48) */
      QuadData(0x3ff00000, 0x00000008) , /* e^(2^-49) */
      QuadData(0x3ff00000, 0x00000004) , /* e^(2^-50) */
      QuadData(0x3ff00000, 0x00000002) , /* e^(2^-51) */
      QuadData(0x3ff00000, 0x00000001) , /* e^(2^-52) */

      QuadData(0x11c44109, 0xedb20931) , /* e^(-2^9)  */
      QuadData(0x28d97559, 0x56ad4e9b) , /* e^(-2^8)  */
      QuadData(0x34642eb9, 0xf39afb0b) , /* e^(-2^7)  */
      QuadData(0x3a2969d4, 0x7321e4cb) , /* e^(-2^6)  */
      QuadData(0x3d0c8464, 0xf7616468) , /* e^(-2^5)  */
      QuadData(0x3e7e355b, 0xbaee85ca) , /* e^(-2^4)  */
      QuadData(0x3f35fc21, 0x041027ac) , /* e^(-2^3)  */
      QuadData(0x3f92c155, 0xb8213cf4) , /* e^(-2^2)  */
      QuadData(0x3fc152aa, 0xa3bf81cb) , /* e^(-2^1)  */
      QuadData(0x3fd78b56, 0x362cef37) , /* e^(-2^0)  */
      QuadData(0x3fe368b2, 0xfc6f9609) , /* e^(-2^-1)  */
      QuadData(0x3fe8ebef, 0x9eac820a) , /* e^(-2^-2)  */
      QuadData(0x3fec3d6a, 0x24ed8221) , /* e^(-2^-3)  */
      QuadData(0x3fee0fab, 0xfbc702a3) , /* e^(-2^-4)  */
      QuadData(0x3fef03f5, 0x6a88b5d7) , /* e^(-2^-5)  */
      QuadData(0x3fef80fe, 0xabfeefa4) , /* e^(-2^-6)  */
      QuadData(0x3fefc03f, 0xd56aa224) , /* e^(-2^-7)  */
      QuadData(0x3fefe00f, 0xfaabffbb) , /* e^(-2^-8)  */
      QuadData(0x3feff003, 0xff556aa8) , /* e^(-2^-9)  */
      QuadData(0x3feff800, 0xffeaabff) , /* e^(-2^-10)  */
      QuadData(0x3feffc00, 0x3ffd556a) , /* e^(-2^-11)  */
      QuadData(0x3feffe00, 0x0fffaaab) , /* e^(-2^-12)  */
      QuadData(0x3fefff00, 0x03fff555) , /* e^(-2^-13)  */
      QuadData(0x3fefff80, 0x00fffeaa) , /* e^(-2^-14)  */
      QuadData(0x3fefffc0, 0x003fffd5) , /* e^(-2^-15)  */
      QuadData(0x3fefffe0, 0x000ffffa) , /* e^(-2^-16)  */
      QuadData(0x3feffff0, 0x0003ffff) , /* e^(-2^-17)  */
      QuadData(0x3feffff8, 0x000ffffl) , /* e^(-2^-18)  */
      QuadData(0x3feffffc, 0x00003fff) , /* e^(-2^-19)  */
      QuadData(0x3feffffe, 0x00000fff) , /* e^(-2^-20)  */
      QuadData(0x3fefffff, 0x000003ff) , /* e^(-2^-21)  */
      QuadData(0x3fefffff, 0x800000ff) , /* e^(-2^-22)  */
      QuadData(0x3fefffff, 0xc000003f) , /* e^(-2^-23)  */
      QuadData(0x3fefffff, 0xe000000f) , /* e^(-2^-24)  */
      QuadData(0x3fefffff, 0xf0000003) , /* e^(-2^-25)  */
      QuadData(0x3fefffff, 0xf8000000) , /* e^(-2^-26)  */
      QuadData(0x3fefffff, 0xfc000000) , /* e^(-2^-27)  */
      QuadData(0x3fefffff, 0xfe000000) , /* e^(-2^-28)  */
      QuadData(0x3fefffff, 0xff000000) , /* e^(-2^-29)  */
      QuadData(0x3fefffff, 0xff800000) , /* e^(-2^-30)  */
      QuadData(0x3fefffff, 0xffc00000) , /* e^(-2^-31)  */
      QuadData(0x3fefffff, 0xffe00000) , /* e^(-2^-32)  */
      QuadData(0x3fefffff, 0xfff00000) , /* e^(-2^-33)  */
      QuadData(0x3fefffff, 0xfff80000) , /* e^(-2^-34)  */
      QuadData(0x3fefffff, 0xfffc0000) , /* e^(-2^-35)  */
      QuadData(0x3fefffff, 0xfffe0000) , /* e^(-2^-36)  */
      QuadData(0x3fefffff, 0xffff0000) , /* e^(-2^-37)  */
      QuadData(0x3fefffff, 0xffff8000) , /* e^(-2^-38)  */
      QuadData(0x3fefffff, 0xffffc000) , /* e^(-2^-39)  */
      QuadData(0x3fefffff, 0xffffe000) , /* e^(-2^-40)  */
      QuadData(0x3fefffff, 0xfffff000) , /* e^(-2^-41)  */
      QuadData(0x3fefffff, 0xfffff800) , /* e^(-2^-42)  */
      QuadData(0x3fefffff, 0xfffffc00) , /* e^(-2^-43)  */
      QuadData(0x3fefffff, 0xfffffe00) , /* e^(-2^-44)  */
      QuadData(0x3fefffff, 0xffffff00) , /* e^(-2^-45)  */
      QuadData(0x3fefffff, 0xffffff80) , /* e^(-2^-46)  */
      QuadData(0x3fefffff, 0xffffffc0) , /* e^(-2^-47)  */
      QuadData(0x3fefffff, 0xffffffe0) , /* e^(-2^-48)  */
      QuadData(0x3fefffff, 0xfffffff0) , /* e^(-2^-49)  */
      QuadData(0x3fefffff, 0xfffffff8) , /* e^(-2^-50)  */
      QuadData(0x3fefffff, 0xfffffffc) , /* e^(-2^-51)  */
      QuadData(0x3fefffff, 0xfffffffe) , /* e^(-2^-52)  */
      QuadData(0x3fefffff, 0xffffffff)   /* e^(-2^-53)  */   };
QUAD Res;
QUAD Mantisse;
LONG Exponent;
int i, Offset;

  Exponent = ((Get_High32of64(y) & IEEEDPExponent_Mask_Hi) >> 20) - 0x3ff;


  /* e^(+-0) = 1, e^(2^(<=-24)) = 1 */
  if ( is_eqC(y, 0x0, 0x0, 0x0UUL) ||
       is_eqC(y, 0x80000000, 0x0, 0x8000000000000000) ||
       Exponent < -52 )
  {
    Set_Value64C(Res, 0x3ff00000, 0x0, 0x3ff0000000000000UUL);
    return Res;
        }

  if ( is_lessSC(y, 0x0, 0x0, 0x0UUL)) /* y < 0 */
  {
    if (Exponent >= 10)
    {
      SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      Set_Value64C(Res , 0x0, 0x0, 0x0UUL);
      return Res;
    }
    Offset = 62;
        }
        else
        {
    if (Exponent >= 10)
    {
      SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      Set_Value64C(Res , IEEEDPPInfty_Hi,
                         IEEEDPPInfty_Lo,
                         IEEEDPPInfty_64);
      return Res;
    }
  Offset = 0;
        }

  /* e^(>= 1024) = overflow) */

  i = 9 - Exponent + Offset;

  /* Mantisse = (y & IEEEDPMantisse_Mask) << 9; */
  Set_Value64(Mantisse, y);
  AND64QC(y, IEEEDPMantisse_Mask_Lo,
             IEEEDPMantisse_Mask_Hi,
             IEEEDPMantisse_Mask_64);

  SHL64(Mantisse, Mantisse, 12);
  Res = ExpTable[i++];

  while ( is_neqC(Mantisse, 0x0, 0x0, 0x0UUL) && i <= (61+Offset) )
  {
    /* is the highest bit set? */
    if ( is_lessSC(Mantisse, 0x0, 0x0, 0x0UUL) /* Mantisse < 0 */ )
    {
      Res = IEEEDPMul(Res, ExpTable[i]);


      if (is_eqC(Res, 0x0, 0x0, 0x0UUL) /* 0 == Res */ )
      {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return Res;
      }

      if (is_eqC(Res, IEEEDPPInfty_Hi,
                      IEEEDPPInfty_Lo,
                      IEEEDPPInfty_64))
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
} /* IEEEDPExp */
