/* @(#)s_nextafter.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_nextafter.c,v 1.11 2005/03/07 21:27:37 das Exp $";
#endif

/* IEEE functions
 *	nextafter(x,y)
 *	return the next machine floating-point number of x in the
 *	direction toward y.
 *   Special cases:
 */

#include <aros/system.h>
#include <float.h>

#include "math.h"
#include "math_private.h"

double
nextafter(double x, double y)
{
	volatile double t;
	int32_t hx,hy,ix,iy;
	uint32_t lx,ly;

	EXTRACT_WORDS(hx,lx,x);
	EXTRACT_WORDS(hy,ly,y);
	ix = hx&0x7fffffff;		/* |x| */
	iy = hy&0x7fffffff;		/* |y| */

	if(((ix>=0x7ff00000)&&((ix-0x7ff00000)|lx)!=0) ||   /* x is nan */
	   ((iy>=0x7ff00000)&&((iy-0x7ff00000)|ly)!=0))     /* y is nan */
	   return x+y;
	if(x==y) return y;		/* x=y, return y */
	if((ix|lx)==0) {			/* x == 0 */
	    INSERT_WORDS(x,hy&0x80000000,1);	/* return +-minsubnormal */
	    t = x*x;
	    if(t==x) return t; else return x;	/* raise underflow flag */
	}
	if(hx>=0) {				/* x > 0 */
	    if(hx>hy||((hx==hy)&&(lx>ly))) {	/* x > y, x -= ulp */
		if(lx==0) hx -= 1;
		lx -= 1;
	    } else {				/* x < y, x += ulp */
		lx += 1;
		if(lx==0) hx += 1;
	    }
	} else {				/* x < 0 */
	    if(hy>=0||hx>hy||((hx==hy)&&(lx>ly))){/* x < y, x -= ulp */
		if(lx==0) hx -= 1;
		lx -= 1;
	    } else {				/* x > y, x += ulp */
		lx += 1;
		if(lx==0) hx += 1;
	    }
	}
	hy = hx&0x7ff00000;
	if(hy>=0x7ff00000) return x+x;	/* overflow  */
	if(hy<0x00100000) {		/* underflow */
	    t = x*x;
	    if(t!=x) {		/* raise underflow flag */
	        INSERT_WORDS(y,hx,lx);
		return y;
	    }
	}
	INSERT_WORDS(x,hx,lx);
	return x;
}

#if (LDBL_MANT_DIG == 53)
/* Alias nextafter -> nexttoward */
AROS_MAKE_ASM_SYM(typeof(nexttoward), nexttoward, AROS_CSYM_FROM_ASM_NAME(nexttoward), AROS_CSYM_FROM_ASM_NAME(nextafter));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(nexttoward));

/* Alias nextafter -> nexttowardl */
AROS_MAKE_ASM_SYM(typeof(nexttowardl), nexttowardl, AROS_CSYM_FROM_ASM_NAME(nexttowardl), AROS_CSYM_FROM_ASM_NAME(nextafter));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(nexttowardl));

/* Alias nextafter -> nextafterl */
AROS_MAKE_ASM_SYM(typeof(nextafterl), nextafterl, AROS_CSYM_FROM_ASM_NAME(nextafterl), AROS_CSYM_FROM_ASM_NAME(nextafter));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(nextafterl));
#endif
