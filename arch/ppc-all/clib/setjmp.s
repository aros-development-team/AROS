/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
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

#include "aros/ppc/asm.h"

	.text
	_ALIGNMENT
	.global AROS_CDEFNAME(setjmp)
	_FUNCTION(AROS_CDEFNAME(setjmp))
	
AROS_CDEFNAME(setjmp):
	stw  %r1,   4(3)
	mflr %r0
	stw  %r2,   8(3)
	stw  %r14, 16(3)
	stfd %f14, 88(3)
	stw  %r0,   0(3)
	stw  %r15, 20(3)
	stfd %f15, 96(3)
	mfcr %r0
	stw  %r16, 24(3)
	stfd %f16,104(3)
	stw  %r0,  12(3)
	stw  %r17, 28(3)
	stfd %f17,112(3)
	stw  %r18, 32(3)
	stfd %f18,120(3)
	stw  %r19, 36(3)
	stfd %f19,128(3)
	stw  %r20, 40(3)
	stfd %f20,136(3)
	stw  %r21, 44(3)
	stfd %f21,144(3)
	stw  %r22, 48(3)
	stfd %f22,152(3)
	stw  %r23, 52(3)
	stfd %f23,160(3)
	stw  %r24, 56(3)
	stfd %f24,168(3)
	stw  %r25, 60(3)
	stfd %f25,176(3)
	stw  %r26, 64(3)
	stfd %f26,184(3)
	stw  %r27, 68(3)
	stfd %f27,192(3)
	stw  %r28, 72(3)
	stfd %f28,200(3)
	stw  %r29, 76(3)
	stfd %f29,208(3)
	stw  %r30, 80(3)
	stfd %f30,216(3)
	stw  %r31, 84(3)
	stfd %f31,224(3)
	li %r3,0
	blr
