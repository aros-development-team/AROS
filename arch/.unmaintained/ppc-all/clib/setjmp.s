/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function setjmp()
    Lang: english
*/

/******************************************************************************

    NAME
#include <setjmp.h>

	int setjmp (jmp_buf env);

    FUNCTION
	Save the current context so that you can return to it later.

    INPUTS
	env - The context/environment is saved here for later restoring

    RESULT
	0 if it returns from setjmp() and 1 when it returns from longjmp().

    NOTES

    EXAMPLE
	jmp_buf env;

	... some code ...

	if (!setjmp (env))
	{
	    ... this code is executed after setjmp() returns ...

	    // This is no good example on how to use this function
	    // You should not do that
	    if (error)
		longjmp (env, 5);

	    ... some code ...
	}
	else
	{
	    ... this code is executed if you call longjmp(env) ...
	}

    BUGS

    SEE ALSO
	longjmp()

    INTERNALS

    HISTORY
	12.10.1999 Code rewritten according to glibc
******************************************************************************/

#include "include/machine.i"

	.text
	_ALIGNMENT
	.global AROS_CDEFNAME(setjmp)
	_FUNCTION(AROS_CDEFNAME(setjmp))
	
AROS_CDEFNAME(setjmp):
	stw  %r1,(0*4)(3)
	mflr %r0
	stw  %r2,(1*4)(3)
	stw  %r14,((3+0)*4)(3)
	stfd %f14,((22+0*2)*4)(3)
	stw  %r0,(2*4)(3)
	stw  %r15,((3+1)*4)(3)
	stfd %f15,((22+1*2)*4)(3)
	mfcr %r0
	stw  %r16,((3+2)*4)(3)
	stfd %f16,((22+2*2)*4)(3)
	stw  %r0,(21*4)(3)
	stw  %r17,((3+3)*4)(3)
	stfd %f17,((22+3*2)*4)(3)
	stw  %r18,((3+4)*4)(3)
	stfd %f18,((22+4*2)*4)(3)
	stw  %r19,((3+5)*4)(3)
	stfd %f19,((22+5*2)*4)(3)
	stw  %r20,((3+6)*4)(3)
	stfd %f20,((22+6*2)*4)(3)
	stw  %r21,((3+7)*4)(3)
	stfd %f21,((22+7*2)*4)(3)
	stw  %r22,((3+8)*4)(3)
	stfd %f22,((22+8*2)*4)(3)
	stw  %r23,((3+9)*4)(3)
	stfd %f23,((22+9*2)*4)(3)
	stw  %r24,((3+10)*4)(3)
	stfd %f24,((22+10*2)*4)(3)
	stw  %r25,((3+11)*4)(3)
	stfd %f25,((22+11*2)*4)(3)
	stw  %r26,((3+12)*4)(3)
	stfd %f26,((22+12*2)*4)(3)
	stw  %r27,((3+13)*4)(3)
	stfd %f27,((22+13*2)*4)(3)
	stw  %r28,((3+14)*4)(3)
	stfd %f28,((22+14*2)*4)(3)
	stw  %r29,((3+15)*4)(3)
	stfd %f29,((22+15*2)*4)(3)
	stw  %r30,((3+16)*4)(3)
	stfd %f30,((22+16*2)*4)(3)
	stw  %r31,((3+17)*4)(3)
	stfd %f31,((22+17*2)*4)(3)
	li %r3,0
	blr
