/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1997/09/27 22:00:38  bergers
    new version of library (uses libheader.c)

    Revision 1.3  1997/07/28 20:43:04  bergers
    Some bug-fixes and changes

    Revision 1.2  1997/07/27 21:47:24  bergers
    Initial revision - yet more math stuff!

    Revision 1.1  1997/07/24 17:25:49  bergers
    Initial revision


    Desc:
    Lang: english
*/
#ifndef __MATHIEEESINGTRANS_INTERN_H__
#define __MATHIEEESINGTRANS_INTERN_H__

/* This is a short file that contains a few things every mathieeesingtrans 
   function needs */

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef PROTO_MATHIEEESINGTRANS_H
#   include <proto/mathieeesingbas.h>
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

/*
    This is the MathIeeeSingTransBase structure. It is documented here because
    it is completely private. Applications should treat it as a struct 
    Library, and use the mathieeespbas.library functions to get information.
*/

extern struct ExecBase * SysBase;
extern struct MathIeeeSingBasBase * MathIeeeSingBasBase;

struct MathIeeeSingTransBase
{
    struct Library     library;
    BPTR               seglist;
};


/* Internal prototypes */

LONG intern_IEEESPLd(struct MathIeeeSingTransBase * MathIeeeSingTransBase, ULONG fnum);
LONG intern_IEEESPisodd(LONG fnum);

#define expunge() \
 AROS_LC0(BPTR, expunge, struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 3, Mathieeesingtrans)

#define IEEESPMantisse_Mask 0x007FFFFF // 23 bit for the mantisse
#define IEEESPExponent_Mask 0x7F800000 //  8 bit for the exponent
#define IEEESPSign_Mask     0x80000000 //  1 bit for the sign

#define Zero_Bit     0x00000004  // Flags of the 680xx CPU
#define Negative_Bit 0x00000008  //
#define Overflow_Bit 0x00000002  //

#define IEEESP_zero   0x00000000  //
#define IEEESP_Pinfty 0x7f800000  // +infinity
#define IEEESP_Ninfty 0xff800000  // -infinity
#define IEEESP_NAN    0xffffffff  // illegal state

/* some constants we need */

#define InvLde   0x3f317218 /*  1 / (log e / log 2)  = log 2 / log e  */
#define InvLd10  0x3e9a209a /*  1 / (log 10 / log 2) = log 2 / log 10 */

#define sqrtonehalf 0x3fb504f3 /* sqrt(1/2) */


#define one      0x3f800000 /*  1.00000000000000000000e+00  */
#define minusone 0xbf800000 /* -1.00000000000000000000e+00  */
#define two      0x40000000 /*  2.0                         */
#define onehalf  0x3f000000 /*  0.5                         */

#define pi       0x40490fdb /*  3.14159265358979311600e+00  */
#define pio2     0x3fc90fdb /*  1.57079632679489655800e+00  */
#define pio4     0x3f490fdb /*  0.7853981634e+00            */

// for calculation of the sine
#define sinf1    one        /*  1/1!   */
#define sinf2    0xbe2aaaab /* -1/3!   */
#define sinf3    0x3c088889 /*  1/5!   */
#define sinf4    0xb9500d01 /* -1/7!   */
#define sinf5    0x3638ef1d /*  1/9!   */
#define sinf6    0xb2d7322b /* -1/11!  */

// for calculation of the cosine
#define cosf1    one        /*  1/1!   */
#define cosf2    0xbf000000 /* -1/2!   */
#define cosf3    0x3d2aaaab /*  1/4!   */
#define cosf4    0xbab60b61 /* -1/6!   */
#define cosf5    0x37d00d01 /*  1/8!   */
#define cosf6    0xb493f27e /* -1/10!  */



#endif /* __MATHIEEESINGTRANS_INTERN_H__  */

