/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#ifndef __MATHIEEESP_INTERN_H__
#define __MATHIEEESP_INTERN_H__

/* the following line is necessary so that the function headers are
   created correctly and the functions can be compiled properly */

#define float LONG

/* This is a short file that contains a few things every mathieeespbas
   function needs
*/
/*
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef PROTO_MATHFFP_H
#   include <proto/mathieeespbas.h>
#endif
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/exec.h>

#include <libcore/base.h>

/*
    This is the MathIEEESPBasBase structure. It is documented here because 
    it is completely private. Applications should treat it as a struct 
    Library, and use the mathieeespbas.library functions to get information.
*/

#define SysBase MathIeeeSingBasBase->lh_SysBase

#define IEEESPMantisse_Mask 0x007FFFFF /* 23 bit for the mantisse  */
#define IEEESPExponent_Mask 0x7F800000 /*  8 bit for the exponent  */
#define IEEESPSign_Mask     0x80000000 /*  1 bit for the sign      */

#define Zero_Bit     0x00000004  /* Flags of the 680xx CPU         */
#define Negative_Bit 0x00000008  
#define Overflow_Bit 0x00000002  

#endif /* __MATHFFP_INTERN_H__  */

