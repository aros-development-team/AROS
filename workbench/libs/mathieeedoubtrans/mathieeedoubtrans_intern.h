/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
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


#ifndef __MATHIEEE64BIT_DEFINES_H__
#   include <aros/mathieee64bitdefines.h>
#endif


/*
    This is the MathIeeeDoubTransBase structure. It is documented here because
    it is completely private. Applications should treat it as a struct 
    Library, and use the mathieeedoubtrans.library functions to get information.
*/

extern struct ExecBase * SysBase;
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


/*
#define MSTB(mstb) ((struct MathIeeeDoubTransBase*)mstb)
#undef  SysBase
#define SysBase (MSTB(MathIeeeDoubTransBase) -> sysbase)
*/


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


#endif /* __MATHIEEEDOUBTRANS_INTERN_H__  */

