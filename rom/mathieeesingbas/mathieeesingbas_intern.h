/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1997/09/28 11:31:46  bergers
    updated version - again

    Revision 1.1  1997/06/25 21:36:46  bergers
    *** empty log message ***


    Desc:
    Lang: english
*/
#ifndef __MATHIEEESP_INTERN_H__
#define __MATHIEEESP_INTERN_H__

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


/*
    This is the MathIEEESPBasBase structure. It is documented here because 
    it is completely private. Applications should treat it as a struct 
    Library, and use the mathieeespbas.library functions to get information.
*/

extern struct ExecBase * SysBase;


struct MathIeeeSingBasBase
{
    struct Library   library;
    BPTR	     seglist;
};



#define IEEESPMantisse_Mask 0x007FFFFF /* 23 bit for the mantisse  */
#define IEEESPExponent_Mask 0x7F800000 /*  8 bit for the exponent  */
#define IEEESPSign_Mask     0x80000000 /*  1 bit for the sign      */

#define Zero_Bit     0x00000004  /* Flags of the 680xx CPU         */
#define Negative_Bit 0x00000008  
#define Overflow_Bit 0x00000002  

#endif /* __MATHFFP_INTERN_H__  */

