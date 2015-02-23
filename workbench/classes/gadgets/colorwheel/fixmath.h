/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef FIXMATH_H
#define FIXMATH_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

/****************************************************************************/

#define FIXED_ONE       	INT_TO_FIXED(1)
#define FIXED_PI        	205887L
#define FIXED_2PI       	411775L
#define FIXED_E         	178144L
#define FIXED_ROOT2     	 74804L
#define FIXED_ROOT3     	113512L
#define FIXED_GOLDEN    	106039L

#define INT_TO_FIXED(x)         ((x) << 16)
#define DOUBLE_TO_FIXED(x)      ((Fixed32)(x * 65536.0 + 0.5))
#define FIXED_TO_INT(x)         ((x) >> 16)
#define FIXED_TO_DOUBLE(x)      (((double)x) / 65536.0)
#define ROUND_FIXED_TO_INT(x)   (((x) + 0x8000) >> 16)

#define ANG90			(FIXED_PI/2)
#define ANG180			(FIXED_PI)
#define ANG270			(ANG180+ANG90)
#define ANG360			(FIXED_2PI)

/****************************************************************************/

typedef LONG	Fixed32;

Fixed32 FixSqrt( Fixed32 );
Fixed32 FixSqrti(ULONG x);
//Fixed32 FixSinCos( Fixed32 theta, Fixed32 *sinus );
Fixed32 FixAtan2( Fixed32, Fixed32 );

/****************************************************************************/

__inline static Fixed32 FixMul(Fixed32 eins,Fixed32 zwei)
{
#ifdef __AROS__
    QUAD result = (QUAD)eins * (QUAD)zwei;
    eins = result >> 16;
#else

#ifndef version060

    __asm __volatile
    ("muls.l %1,%1:%0 \n\t"
     "move %1,%0 \n\t"
     "swap %0 "

      : "=d" (eins), "=d" (zwei)
      : "0" (eins), "1" (zwei)
    );

#else
#if 0
    __asm __volatile
    ("fmove.l	%0,fp0 \n\t"
     "fmul.l	%2,fp0 \n\t"
     "fmul.s	#$37800000,fp0 \n\t"

/*    "fintrz.x	fp0,fp0 \n\t"*/
     "fmove.l	fp0,%0"

      : "=d" (eins)
      : "0" (eins), "d" (zwei)
      : "fp0"
    );
#else
    eins = (Fixed32) ( (double) eins * (double) zwei * 0.0000152587890625 );
#endif

#endif /* version060 */

#endif /* __AROS__ */

    return eins;
}

/****************************************************************************/

__inline static Fixed32 FixDiv(Fixed32 eins,Fixed32 zwei)
{
#ifdef __AROS__
    QUAD result = ((QUAD)eins << 16) / (QUAD)zwei;
    eins = (Fixed32) result;
#else
	
#ifndef version060
    __asm __volatile
    ("swap      %0\n\t"
     "move.w    %0,d2\n\t"
     "ext.l		d2\n\t"
     "clr.w		%0\n\t"
     "divs.l	%1,d2:%0\n\t"

     : "=d" (eins), "=d" (zwei)
     : "0" (eins), "1" (zwei)
     : "d2"
    );
#else
#if 0
    __asm __volatile
    ("fmove.l	%0,fp0 \n\t"
     "fdiv.l	%2,fp0 \n\t"
     "fmul.s	#$47800000,fp0 \n\t" 

/*   "fintrz.x  fp0 \n\t"*/
     "fmove.l	fp0,%0"

     : "=d" (eins)
     : "0" (eins), "d" (zwei)
     : "fp0"
    );
#else
    eins = (double) eins / (double) zwei * 65536.0;
#endif
#endif /* version060 */

#endif /* __AROS__ */

    return eins;
}

/****************************************************************************/

__inline static LONG FixSqr( Fixed32 a )
{
//  a = INT_TO_FIXED(a);
//    return a * a;
  return FixMul( a, a );
}	

/****************************************************************************/

__inline static Fixed32 FixSinCos( Fixed32 theta, Fixed32 *sinus )
{
    #define MAX_TRIG	1024
    extern Fixed32 SinCosTab[];
  
    theta = FIXED_TO_INT( FixMul( theta << 9, 20861 ) );
    theta &= (MAX_TRIG - 1);
    
    *sinus = SinCosTab[theta];
    
    if( ( theta += 256 ) >= MAX_TRIG )
	theta = theta - MAX_TRIG;
		
    return SinCosTab[theta];
}	

/****************************************************************************/

#endif
