/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1997/06/25 21:36:46  bergers
    *** empty log message ***


    Desc:
    Lang: english
*/
#ifndef __MATHFFP_INTERN_H__
#define __MATHFFP_INTERN_H__

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
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

#define MATHIEEESPBASNAME     "mathieeespbas.library"

/*
    This is the MathIEEESPBasBase structure. It is documented here because 
    it is completely private. Applications should treat it as a struct 
    Library, and use the mathieeespbas.library functions to get information.
*/

#define MathieeesingbasBase MathIeeeSingBasBase

struct MathIeeeSingBasBase
{
    struct Library LibNode;
};



#define IEEESPMantisse_Mask 0x007FFFFF /* 23 bit for the mantisse  */
#define IEEESPExponent_Mask 0x7F800000 /*  8 bit for the exponent  */
#define IEEESPSign_Mask     0x80000000 /*  1 bit for the sign      */

#define Zero_Bit     0x00000004  /* Flags of the 680xx CPU         */
#define Negative_Bit 0x00000008  
#define Overflow_Bit 0x00000002  


#define expunge() \
 AROS_LC0(BPTR, expunge, struct Library *, MathieeesingbasBase, 3, Mathieeespbas)

#endif /* __MATHFFP_INTERN_H__  */

/* Some notes...

  Results of the original libraries

  Division:
   0.0 / 0.0 = $7f88 0000  (division by zero)
   XXX / 0.0 = $f780 0000  (division by zero)

  Multiplication:
   Overflow: $7f80 0000
*/
