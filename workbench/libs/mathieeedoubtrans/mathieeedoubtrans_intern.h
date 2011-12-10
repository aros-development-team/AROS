/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef __MATHIEEEDOUBTRANS_INTERN_H__
#define __MATHIEEEDOUBTRANS_INTERN_H__

/* the following line is necessary so that the function headers are
   created correctly and the functions can be compiled properly */

#define double QUAD

/* This is a short file that contains a few things every mathieeedoubtrans
   function needs */

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef PROTO_MATHIEEEDPTRANS_H
#   include <proto/mathieeedoubtrans.h>
#endif

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

#include <proto/mathieeedoubbas.h>
#include <proto/exec.h>

#ifndef __MATHIEEE64BIT_DEFINES_H__
#   include <aros/mathieee64bitdefines.h>
#endif


/*
    This is the MathIeeeDoubTransBase structure. It is documented here because
    it is completely private. Applications should treat it as a struct 
    Library, and use the mathieeedoubtrans.library functions to get information.
*/

extern struct Library * MathIeeeDoubBasBase;

struct MathIeeeDoubTransBase
{
    struct Library     library;
    struct ExecBase  * sysbase;
    BPTR               seglist;
};

/* Internal prototypes */
QUAD intern_IEEEDPLd(struct MathIeeeDoubTransBase * MathIeeeDoubTransBase, QUAD fnum);
LONG intern_IEEEDPisodd(QUAD fnum);


#define Zero_Bit     0x00000004  /* Flags of the 680xx CPU */
#define Negative_Bit 0x00000008  
#define Overflow_Bit 0x00000002  


#define IEEEDPMantisse_Mask_Hi 0x000FFFFF /* 62 bit for the mantisse  */
#define IEEEDPMantisse_Mask_Lo 0xFFFFFFFF 
#define IEEEDPExponent_Mask_Hi 0x7FF00000 /* 10 bit for the exponent  */
#define IEEEDPExponent_Mask_Lo 0x00000000 
#define IEEEDPSign_Mask_Hi     0x80000000 /*  1 bit for the sign      */
#define IEEEDPSign_Mask_Lo     0x00000000 

#define IEEEDPNAN_Hi           0x7FFFFFFF
#define IEEEDPNAN_Lo           0xFFFFFFFF
#define IEEEDPNAN_64           0x7FFFFFFFFFFFFFFFULL

#define IEEEDPPInfty_Hi        0x7FEFFFFF
#define IEEEDPPInfty_Lo        0xFFFFFFFF
#define IEEEDPPInfty_64        0x7FEFFFFFFFFFFFFFULL

#define IEEEDPMantisse_Mask_64 0x0007FFFFFFFFFFFFULL /* 51 bit for the mantisse */
#define IEEEDPExponent_Mask_64 0x7FF8000000000000ULL /* 12 bit for the exponent */
#define IEEEDPSign_Mask_64     0x8000000000000000ULL /*  1 bit for the sign     */

#define IEEESPMantisse_Mask    0x007FFFFF /* 23 bit for the mantisse */
#define IEEESPExponent_Mask    0x7F800000 /*  8 bit for the exponent */
#define IEEESPSign_Mask        0x80000000 /*  1 bit for the sign     */

#define pi_Hi 0x400921FB
#define pi_Lo 0x54442D18
#define pi_64 0x400921FB54442D18ULL
#define pi QuadData(pi_Hi, pi_Lo, pi_64)

#define pio2_Hi 0x3FF921FB
#define pio2_Lo 0x54442D18
#define pio2_64 0x3FF921FB54442D18ULL
#define pio2 QuadData(pio2_Hi, pio2_Lo, pio2_64)

#define pio2_hi_Hi 0x3FF921FB
#define pio2_hi_Lo 0x54442D18
#define pio2_hi_64 0x3FF921FB54442D18ULL
#define pio2_hi QuadData(pio2_hi_Hi, pio2_hi_Lo, pio2_hi_64)

#define neg_pio2_hi_Hi 0xBFF921FB
#define neg_pio2_hi_Lo 0x54442D18
#define neg_pio2_hi_64 0x3FF921FB54442D18ULL
#define neg_pio2_hi QuadData(neg_pio2_hi_Hi, neg_pio2_hi_Lo, neg_pio2_hi_64)

#define pio2_lo_Hi 0x3C91A626
#define pio2_lo_Lo 0x33145C07
#define pio2_lo_64 0x3C91A62633145C07ULL
#define pio2_lo QuadData(pio2_lo_Hi, pio2_lo_Lo, pio2_lo_64)

#define pio4_hi_Hi 0x3FE921FB
#define pio4_hi_Lo 0x54442D18
#define pio4_hi_64 0x3FE921FB54442D18ULL
#define pio4_hi QuadData(pio4_hi_Hi, pio4_hi_Lo, pio4_hi_64)

#define pS0_Hi 0x3FC55555
#define pS0_Lo 0x55555555
#define pS0_64 0x3FC5555555555555ULL
#define pS0    QuadData(pS0_Hi, pS0_Lo, pS0_64)

#define pS1_Hi 0xBFD4D612
#define pS1_Lo 0x03EB6F7D
#define pS1_64 0xBFD4D61203EB6F7DULL
#define pS1    QuadData(pS1_Hi, pS1_Lo, pS1_64)

#define pS2_Hi 0x3FC9C155
#define pS2_Lo 0x0E884455
#define pS2_64 0x3FC9C1550E884455ULL
#define pS2    QuadData(pS2_Hi, pS2_Lo, pS2_64)

#define pS3_Hi 0xBFA48228
#define pS3_Lo 0xB5688F3B
#define pS3_64 0xBFA48228B5688F3BULL
#define pS3    QuadData(pS3_Hi, pS3_Lo, pS3_64)

#define pS4_Hi 0x3F49EFE0
#define pS4_Lo 0x7501B288
#define pS4_64 0x3F49EFE07501B288ULL
#define pS4    QuadData(pS4_Hi, pS4_Lo, pS4_64)

#define pS5_Hi 0x3F023DE1
#define pS5_Lo 0x0DFDF709
#define pS5_64 0x3F023DE10DFDF709ULL
#define pS5    QuadData(pS5_Hi, pS5_Lo, pS5_64)

#define qS1_Hi 0xC0033A27
#define qS1_Lo 0x1C8A2D4B
#define qS1_64 0xC0033A271C8A2D4BULL
#define qS1    QuadData(qS1_Hi, qS1_Lo, qS1_64)

#define qS2_Hi 0x40002AE5
#define qS2_Lo 0x9C598AC8
#define qS2_64 0x40002AE59C598AC8ULL
#define qS2    QuadData(qS2_Hi, qS2_Lo, qS2_64)

#define qS3_Hi 0xBFE6066C
#define qS3_Lo 0x1B8D0159
#define qS3_64 0xBFE6066C1B8D0159ULL
#define qS3    QuadData(qS3_Hi, qS3_Lo, qS3_64)

#define qS4_Hi 0x3FB3B8C5
#define qS4_Lo 0xB12E9282
#define qS4_64 0x3FB3B8C5B12E9282ULL
#define qS4    QuadData(qS4_Hi, qS4_Lo, qS4_64)


#define zero_64 0x000000000000000ULL
#define zero QuadData(0x0, 0x0, 0ULL)

#define one_Hi 0x3ff00000
#define one_Lo 0x00000000
#define one_64 0x3ff0000000000000ULL
#define oneC QuadData(one_Hi, one_Lo, one_64)

#define two_Hi 0x40000000
#define two_Lo 0x00000000
#define two_64 0x4000000000000000ULL
#define two QuadData(two_Hi, two_Lo, two_64)

#define onehalf_Hi  0x3fe00000
#define onehalf_Lo  0x00000000
#define onehalf_64  0x3fe0000000000000ULL
#define onehalf QuadData(onehalf_Hi, onehalf_Lo, onehalf_64)


#define onethird_Hi 0x3fd55555
#define onethird_Lo 0x55555555
#define onethird_64 0x3fd5555555555555ULL

#define onefifth_Hi 0x3fc99999
#define onefifth_Lo 0x99999999
#define onefifth_64 0x3fc9999999999999ULL

/* Defines to calculate the cosine */
#define cosf1_Hi 0x3ff00000  /* 1 */
#define cosf1_Lo 0x00000000
#define cosf1_64 0x3ff0000000000000ULL
#define cosf1 QuadData(cosf1_Hi, cosf1_Lo, cosf1_64)

#define cosf2_Hi 0xbfe00000  /* -1/2! */
#define cosf2_Lo 0x00000000
#define cosf2_64 0xbfe0000000000000ULL
#define cosf2 QuadData(cosf2_Hi, cosf2_Lo, cosf2_64)

#define cosf3_Hi 0x3fa55555  /* 1/4! */
#define cosf3_Lo 0x55555555
#define cosf3_64 0x3fa5555555555555ULL
#define cosf3 QuadData(cosf3_Hi, cosf3_Lo, cosf3_64)

#define cosf4_Hi 0xbf56c16c  /* -1/6! */
#define cosf4_Lo 0x16c16c16
#define cosf4_64 0xbf56c16c16c16c16ULL
#define cosf4 QuadData(cosf4_Hi, cosf4_Lo, cosf4_64)

#define cosf5_Hi 0x3efa01a0  /* 1/8! */
#define cosf5_Lo 0x1a01a01a
#define cosf5_64 0x3efa01a01a01a010ULL
#define cosf5 QuadData(cosf5_Hi, cosf5_Lo, cosf5_64)

#define cosf6_Hi 0xbe927e4f  /* -1/10! */
#define cosf6_Lo 0xb7789f5c
#define cosf6_64 0xbe927e4fb7789f5cULL
#define cosf6 QuadData(cosf6_Hi, cosf6_Lo, cosf6_64)

#define cosf7_Hi 0x3e21eed8  /* 1/12! */
#define cosf7_Lo 0xeff8d897
#define cosf7_64 0x3e21eed8eff8d897ULL
#define cosf7 QuadData(cosf7_Hi, cosf7_Lo, cosf7_64)

#define cosf8_Hi 0xbda93974  /* -1/14! */
#define cosf8_Lo 0xa8c07c9d
#define cosf8_64 0xbda93974a8c07c9dULL
#define cosf8 QuadData(cosf8_Hi, cosf8_Lo, cosf8_64)

/* Defines to calculate the sine */
#define sinf1_Hi 0x3ff00000  /* 1 */
#define sinf1_Lo 0x00000000
#define sinf1_64 0x3ff0000000000000ULL
#define sinf1 QuadData(sinf1_Hi, sinf1_Lo, sinf1_64)

#define sinf2_Hi 0xbfc55555  /* -1/3! */
#define sinf2_Lo 0x55555555
#define sinf2_64 0xbfc5555555555555ULL
#define sinf2 QuadData(sinf2_Hi, sinf2_Lo, sinf2_64)

#define sinf3_Hi 0x3f811111  /* 1/5! */
#define sinf3_Lo 0x11111111
#define sinf3_64 0x3f81111111111111ULL
#define sinf3 QuadData(sinf3_Hi, sinf3_Lo, sinf3_64)

#define sinf4_Hi 0xbf2a01a0  /* -1/7! */
#define sinf4_Lo 0x1a01a01a
#define sinf4_64 0xbf2a01a01a01a01aULL
#define sinf4 QuadData(sinf4_Hi, sinf4_Lo, sinf4_64)

#define sinf5_Hi 0x3ec71de3  /* 1/9! */
#define sinf5_Lo 0xa556c733
#define sinf5_64 0x3ec71de3a556c733ULL
#define sinf5 QuadData(sinf5_Hi, sinf5_Lo, sinf5_64)

#define sinf6_Hi 0xbe5ae645  /* -1/11! */
#define sinf6_Lo 0x67f544e3
#define sinf6_64 0xbe5ae64567f544e3ULL
#define sinf6 QuadData(sinf6_Hi, sinf6_Lo, sinf6_64)

#define sinf7_Hi 0x3de61246  /* 1/13! */
#define sinf7_Lo 0x13a86d09
#define sinf7_64 0x3de6124613a86d09ULL
#define sinf7 QuadData(sinf7_Hi, sinf7_Lo, sinf7_64)

#define sinf8_Hi 0xbd6ae7f3  /* -1/15! */
#define sinf8_Lo 0xe733b81f
#define sinf8_64 0xbd6ae7f3e733b81fULL
#define sinf8 QuadData(sinf8_Hi, sinf8_Lo, sinf8_64)

#ifdef __mc68000
#include <aros/m68k/libcall_cc.h>
#endif

#endif /* __MATHIEEEDOUBTRANS_INTERN_H__  */
