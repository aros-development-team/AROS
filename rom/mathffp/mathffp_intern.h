/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1997/06/25 21:36:43  bergers
    *** empty log message ***

    Revision 1.1  1997/05/30 20:50:57  aros
    *** empty log message ***


    Desc:
    Lang: english
*/
#ifndef __MATHFFP_INTERN_H__
#define __MATHFFP_INTERN_H__

/* This is a short file that contains a few things every mathffp function
    needs */

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef PROTO_MATHFFP_H
#   include <proto/mathffp.h>
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

#define MATHFFPNAME     "mathffp.library"

/*
    This is the MAthffpBase structure. It is documented here because it is
    completely private. Applications should treat it as a struct Library, and
    use the mathffp.library functions to get information.
*/

struct MathBase
{
    struct Library LibNode;
};


#define FFPMantisse_Mask 0xFFFFFF00 /* 24 bit for the mantisse    */
#define FFPExponent_Mask 0x0000007F /*  7 bit for the exponent    */
#define FFPSign_Mask     0x00000080 /*  1 bit for the sign        */

#define IEEESPMantisse_Mask 0x007FFFFF /* 23 bit for the mantisse */
#define IEEESPExponent_Mask 0x7F800000 /*  8 bit for the exponent */
#define IEEESPSign_Mask     0x80000000 /*  1 bit for the sign     */

#define Zero_Bit     0x00000004  /* Flags of the 680xx CPU */
#define Negative_Bit 0x00000008  
#define Overflow_Bit 0x00000002  

#define expunge() \
 AROS_LC0(BPTR, expunge, struct MathBase *, MathBase, 3, Mathffp)

#endif /* __MATHFFP_INTERN_H__  */

