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

/* Internal prototypes */

/*
    This is the MathIeeeDoubTransBase structure. It is documented here because
    it is completely private. Applications should treat it as a struct 
    Library, and use the mathieeedoubtrans.library functions to get information.
*/

extern struct ExecBase * SysBase;

struct MathIeeeDoubTransBase
{
    struct Library     library;
    struct ExecBase  * sysbase;
    BPTR               seglist;
};

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
#define IEEEDPNAN_64           0x7FFFFFFFFFFFFFFFUUL

#define IEEEDPPInfty_Hi        0x7FEFFFFF
#define IEEEDPPInfty_Lo        0xFFFFFFFF
#define IEEEDPPInfty_64        0x7FEFFFFFFFFFFFFFUUL

#define IEEEDPMantisse_Mask_64 0x0007FFFFFFFFFFFFULL /* 63 bit for the mantisse */
#define IEEEDPExponent_Mask_64 0x7FF8000000000000ULL /* 10 bit for the exponent */
#define IEEEDPSign_Mask_64     0x8000000000000000ULL /*  1 bit for the sign     */

#define IEEESPMantisse_Mask    0x007FFFFF /* 23 bit for the mantisse */
#define IEEESPExponent_Mask    0x7F800000 /*  8 bit for the exponent */
#define IEEESPSign_Mask        0x80000000 /*  1 bit for the sign     */


#endif /* __MATHIEEEDOUBTRANS_INTERN_H__  */

