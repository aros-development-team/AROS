/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef __MATHFFP_INTERN_H__
#define __MATHFFP_INTERN_H__

/* the following line is necessary so that the function headers are
   created correctly and the functions can be compiled properly */

#define float LONG


/* This is a short file that contains a few things every mathffp function
    needs */

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#include <proto/arossupport.h>
#ifndef PROTO_MATHFFP_H
#   include <proto/mathffp.h>
#endif

#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
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

#include <proto/exec.h>
#include <libcore/base.h>

/*
    This is the MAthffpBase structure. It is documented here because it is
    completely private. Applications should treat it as a struct Library, and
    use the mathffp.library functions to get information.
*/

#undef SysBase
#define SysBase (MathBase->lh_SysBase)

#define FFPMantisse_Mask 0xFFFFFF00 /* 24 bit for the mantisse    */
#define FFPExponent_Mask 0x0000007F /*  7 bit for the exponent    */
#define FFPSign_Mask     0x00000080 /*  1 bit for the sign        */

#define IEEESPMantisse_Mask 0x007FFFFF /* 23 bit for the mantisse */
#define IEEESPExponent_Mask 0x7F800000 /*  8 bit for the exponent */
#define IEEESPSign_Mask     0x80000000 /*  1 bit for the sign     */

#define Zero_Bit     0x00000004  /* Flags of the 680xx CPU */
#define Negative_Bit 0x00000008  
#define Overflow_Bit 0x00000002  

/* some Motorla fast floting point format constants  */
#define SP_zero   0x00000000
#define SP_pinfty 0xffffff7f

/* basic constants */
#define one      0x80000041 /*  1.00000000000000000000e+00 */
#define minusone 0x800000c1 /* -1.00000000000000000000e+00 */
#define two      0x80000042 /*  2.0                        */
#define onehalf  0x80000040 /*  0.5                        */

#endif /* __MATHFFP_INTERN_H__  */
