/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

struct MathIeeeSingTransBase_intern; /* prereference */
/* Internal prototypes */

LONG intern_IEEESPLd(struct MathIeeeSingTransBase_intern * MathIeeeSingTransBase, ULONG fnum);


#define MATHIEEESINGTRANSNAME     "mathieeesingtrans.library"

/*
    This is the MathIeeeSingTransBase structure. It is documented here because
    it is completely private. Applications should treat it as a struct 
    Library, and use the mathieeespbas.library functions to get information.
*/


struct MathIeeeSingTransBase_intern
{
    struct Library     library;
    struct ExecBase  * sysbase;
    BPTR               seglist;

    struct Library   * MathIeeeSingBas;
};

#define MSTB(mstb) ((struct MathIeeeSingTransBase_intern *)mstb)
#undef SysBase
#define SysBase (MSTB(MathIeeeSingTransBase) -> sysbase)
#undef MathIeeeSingBasBase
#define MathIeeeSingBasBase (MSTB(MathIeeeSingTransBase) -> MathIeeeSingBas)



#define IEEESPMantisse_Mask 0x007FFFFF // 23 bit for the mantisse
#define IEEESPExponent_Mask 0x7F800000 //  8 bit for the exponent
#define IEEESPSign_Mask     0x80000000 //  1 bit for the sign

#define Zero_Bit     0x00000004  // Flags of the 680xx CPU
#define Negative_Bit 0x00000008  //
#define Overflow_Bit 0x00000002  //

#define IEEESP_zero   0x00000000  //
#define IEEESP_Pinfty 0x7f800000  // +infinity
#define IEEESP_Ninfty 0xff800000  // -infinity

/* some constants we need */

#define InvLde   0x3f317218 /*  1 / (log e / log 2)  = log 2 / log e  */
#define InvLd10  0x3e9a209a /*  1 / (log 10 / log 2) = log 2 / log 10 */

#define sqrtonehalf 0x3fb504f3 /* sqrt(1/2) */


#define one      0x3f800000 /*  1.00000000000000000000e+00  */
#define minusone 0xbf800000 /* -1.00000000000000000000e+00  */
#define two      0x40000000 /*  2.0                         */
#define onehalf  0x3f000000 /*  0.5                         */
// untere Wert müssen noch geändert werden!
#define pi       0xc90fdb42 /*  3.14159265358979311600e+00  */
#define pio2     0xc90fdb41 /*  1.57079632679489655800e+00  */
#define pio4     0xc90fdb40 /*  0.7853981634e+00            */

#define pS0      0xaaaaaa3e /*a  1.66666666666666657415e-01 */
#define pS1      0xa6b090bf /*0 -3.25565818622400915405e-01 */
#define pS2      0xce0aa83e /*8  2.01212532134862925881e-01 */
#define pS3      0xa41146bc /*5 -4.00555345006794114027e-02 */
#define pS4      0xcf7f0436 /*3  7.91534994289814532176e-04 */
#define pS5      0x91ef0932 /*8  3.47933107596021167570e-05 */

#define qS1      0x99d138c2 /*8 -2.40339491173441421878e+00 */
#define qS2      0x81572c42 /*c  2.02094576023350569471e+00 */
#define qS3      0xb03361c0 /*0 -6.88283971605453293030e-01 */
#define qS4      0x9dc62e3d /*d  7.70381505559019352791e-02 */


#define expunge() \
 AROS_LC0(BPTR, expunge, struct MathIeeeSingTransBase *, MathIeeeSingTransBase, 3, Mathieeesingtrans)

#endif /* __MATHIEEESINGTRANS_INTERN_H__  */

