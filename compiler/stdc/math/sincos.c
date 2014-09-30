/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include "math.h"
#include "math_private.h"

void sincos(double x, double *sin, double *cos)
{
	double y[2],z=0.0;
	int32_t n, ix;

    /* High word of x. */
	GET_HIGH_WORD(ix,x);

    /* |x| ~< pi/4 */
	ix &= 0x7fffffff;
	if(ix <= 0x3fe921fb) {
	    if(ix<0x3e400000)			/* |x| < 2**-27 */
	       {if((int)x==0) { *sin=x; *cos=1.0; return; }}	/* generate inexact */
	    *sin = __kernel_sin(x,z,0);
	    *cos = __kernel_cos(x,z);
	    return;
	}

	/* cos(Inf or NaN) is NaN */
	else if (ix>=0x7ff00000) {*sin = *cos = x-x; return; }

	/* argument reduction needed */
	else {
		n = __ieee754_rem_pio2(x,y);
		switch(n&3) {
		case 0: *sin =  __kernel_sin(y[0],y[1],1); *cos =  __kernel_cos(y[0],y[1]);   break;
		case 1: *sin =  __kernel_cos(y[0],y[1]);   *cos = -__kernel_sin(y[0],y[1],1); break;
		case 2: *sin = -__kernel_sin(y[0],y[1],1); *cos = -__kernel_cos(y[0],y[1]);   break;
		default:
			*sin = -__kernel_cos(y[0],y[1]); *cos = __kernel_sin(y[0],y[1],1);
		}
	}
}
