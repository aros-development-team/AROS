/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#ifndef __MATHIEEEDOUBTRANS_INTERN_H__
#define __MATHIEEEDOUBTRANS_INTERN_H__

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
#   include "mathieee64bitdefines.h"
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


#define pi      QuadData( 0x400921FB, 0x54442D18, 0x400921FB54442D18ULL )
#define pio2_hi QuadData( 0x3FF921FB, 0x54442D18, 0x3FF921FB54442D18ULL )
#define pio2_lo QuadData( 0x3C91A626, 0x33145C07, 0x3C91A62633145C07ULL )
#define pS0     QuadData( 0x3FC55555, 0x55555555, 0x3FC5555555555555ULL )
#define pS1     QuadData( 0xBFD4D612, 0x03EB6F7D, 0xBFD4D61203EB6F7DULL )
#define pS2     QuadData( 0x3FC9C155, 0x0E884455, 0x3FC9C1550E884455ULL )
#define pS3     QuadData( 0xBFA48228, 0xB5688F3B, 0xBFA48228B5688F3BULL )
#define pS4     QuadData( 0x3F49EFE0, 0x7501B288, 0x3F49EFE07501B288ULL )
#define pS5     QuadData( 0x3F023DE1, 0x0DFDF709, 0x3F023DE10DFDF709ULL )
#define qS1     QuadData( 0xC0033A27, 0x1C8A2D4B, 0xC0033A271C8A2D4BULL )
#define qS2     QuadData( 0x40002AE5, 0x9C598AC8, 0x40002AE59C598AC8ULL )
#define qS3     QuadData( 0xBFE6066C, 0x1B8D0159, 0xBFE6066C1B8D0159ULL )
#define qS4     QuadData( 0x3FB3B8C5, 0xB12E9282, 0x3FB3B8C5B12E9282ULL )


#endif /* __MATHIEEEDOUBTRANS_INTERN_H__  */

