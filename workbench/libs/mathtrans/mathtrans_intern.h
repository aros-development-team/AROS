/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#ifndef MATHTRANS_INTERN_H
#define MATHTRANS_INTERN_H

/* the following line is necessary so that the function headers are
   created correctly and the functions can be compiled properly */

#define float LONG


/* This is a short file that contains a few things every mathffp function
    needs */

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
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

#include <proto/mathtrans.h>
#include <libraries/mathffp.h>

extern struct ExecBase * SysBase;
extern struct MathBase * MathBase;
/*
    This is the MathTransBase structure. It is documented here because it is
    completely private. Applications should treat it as a struct Library, and
    use the mathtrans.library functions to get information.
*/

/* internal prototypes */

LONG intern_SPLd(ULONG fnum);
LONG intern_SPisodd(ULONG fnum);

#define FFPMantisse_Mask 0xFFFFFF00 /* 24 bit for the mantisse */
#define FFPExponent_Mask 0x0000007F /*  7 bit for the exponent */
#define FFPSign_Mask     0x00000080

#define IEEESPMantisse_Mask 0x007FFFFF /* 23 bit for the mantisse */
#define IEEESPExponent_Mask 0x7F800000 /*  8 bit for the exponent */
#define IEEESPSign_Mask     0x80000000

#define Zero_Bit 0x00000004
#define Negative_Bit 0x00000008
#define Overflow_Bit 0x00000002

/* some Motorla fast floting point format constants  */
#define FFP_zero   0x00000000
#define FFP_Pinfty 0xffffff7f
#define FFP_Ninfty 0xffffffff
#define FFP_NAN    0xffffffff

/* constants neccessary for log and log10 */
#define InvLde   0xb1721840    /*  1 / (log e / log 2)  = log 2 / log e  */
#define InvLd10  0x9a209b3f    /*  1 / (log 10 / log 2) = log 2 / log 10 */
#define sqrtonehalf 0xb504f340 /* sqrt(1/2)                              */


/* constant necessary for exp */

#define exp1 0xadf85442

/* basic constants */
#define one      0x80000041 /*  1.00000000000000000000e+00 */
#define minusone 0x800000c1 /* -1.00000000000000000000e+00 */
#define two      0x80000042 /*  2.0                        */
#define onehalf  0x80000040 /*  0.5                        */
#define three    0xc0000042 /*  3.0                        */

/* some more constants: pi, pi/2, pi/4 */

#define pi       0xc90fdb42 /*  3.14159265358979311600e+00 */
#define pio2     0xc90fdb41 /*  1.57079632679489655800e+00 */
#define pio4     0xc90fdb40 /*  0.7853981634e+00           */

/*  for calculation of the sine */
#define sinf1    one        /*  1/1!   */
#define sinf2    0xaaaaabbe /* -1/3!   */
#define sinf3    0x8888893a /*  1/5!   */
#define sinf4    0xd00d01b4 /* -1/7!   */
#define sinf5    0xb8ef1d2e /*  1/9!   */
#define sinf6    0xd7322ba7 /* -1/11!  */

/*  for calculation of the cosine */
#define cosf1    one        /*  1/1!   */
#define cosf2    0x800000c0 /* -1/2!   */
#define cosf3    0xaaaaab3c /*  1/4!   */
#define cosf4    0xb60b61b7 /* -1/6!   */
#define cosf5    0xd00d0131 /*  1/8!   */
#define cosf6    0x93f27eab /* -1/10!  */

/* constants neccessary for acos and asin */
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

#endif /* MATHTRANS_INTERN_H  */



