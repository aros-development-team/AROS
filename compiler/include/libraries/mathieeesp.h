#ifndef LIBRARIES_MATHIEEESP_H
#define LIBRARIES_MATHIEEESP_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for mathieeespbas.library and
                          mathieeesptrans.library
    Lang: english
*/

#ifndef PI
#define PI      ((float)3.141592653589793))
#endif

#define TWO_PI  (((float) 2) * PI)
#define PI2     (PI/((float)2))
#define PI4     (PI/((float)4))

#ifndef E
#define E       ((float)2.718281828459045)
#endif

#define LOG10   ((float)2.302585092994046)
#define FPTEN   ((float)10.0)
#define FPONE   ((float)1.0)
#define FPHALF  ((float)0.5)
#define FPZERO  ((float)0.0)
#define trunc(x) ((int)(x))
#define round(x) ((int)((x) + 0.5))
#define itof(i)  ((float)(i))

/* Now let's define the ANSI C functions and map them to the
   IEEE signle precision functions
 */

#define fabs    IEEESPAbs
#define floor   IEEESPFloor
#define ceil    IEEESPCeil

#define cos     IEEESPCos
#define acos    IEEESPAcos
#define cosh    IEEESPCosh

#define sin     IEEESPSin
#define asin    IEEESPAsin
#define sinh    IEEESPSinh

#define tan     IEEESPTan
#define atan    IEEESPAtan
#define tanh    IEEESPTanh

#define exp     IEEESPExp
#define pow(a,b) IEEESPPow((b),(a))
#define log     IEEESPLog
#define log10   IEEESPLog10
#define sqrt    IEEESPSqrt


/* I also include the function prototypes here! */
#if 0
#ifndef PROTO_MATHIEEESINGBAS_H
#include <proto/mathieeesingbas.h>
#endif

#ifndef PROTO_MATHIEEESINGTRANS_H
#include <proto/mathieeesingtrans.h>
#endif
#endif
#endif  /* LIBRARIES_MATHIEEESP_H */
