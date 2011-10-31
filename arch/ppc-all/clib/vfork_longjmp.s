/*
    Copyright � 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

/* This function works the same as longjmp() except it lacks the argument 
   check. It's used only by vfork() implementation. */

#include "aros/ppc/asm.h"

	.text
	_ALIGNMENT
	.globl AROS_CDEFNAME(vfork_longjmp)
	_FUNCTION(vfork_longjmp)

AROS_CDEFNAME(vfork_longjmp):
	lwz %r1,(0*4)(%r3)
	lwz %r2,(1*4)(%r3)
	lwz %r0,(2*4)(%r3)
	lwz %r14,((3+0)*4)(%r3)
	lfd %f14,((22+0*2)*4)(%r3)
	lwz %r15,((3+1)*4)(%r3)
	lfd %f15,((22+1*2)*4)(%r3)
	lwz %r16,((3+2)*4)(%r3)
	lfd %f16,((22+2*2)*4)(%r3)
	lwz %r17,((3+3)*4)(%r3)
	lfd %f17,((22+3*2)*4)(%r3)
	lwz %r18,((3+4)*4)(%r3)
	lfd %f18,((22+4*2)*4)(%r3)
	lwz %r19,((3+5)*4)(%r3)
	lfd %f19,((22+5*2)*4)(%r3)
	lwz %r20,((3+6)*4)(%r3)
	lfd %f20,((22+6*2)*4)(%r3)
	mtlr %r0
	lwz %r21,((3+7)*4)(%r3)
	lfd %f21,((22+7*2)*4)(%r3)
	lwz %r22,((3+8)*4)(%r3)
	lfd %f22,((22+8*2)*4)(%r3)
	lwz %r0,(21*4)(%r3)
	lwz %r23,((3+9)*4)(%r3)
	lfd %f23,((22+9*2)*4)(%r3)
	lwz %r24,((3+10)*4)(%r3)
	lfd %f24,((22+10*2)*4)(%r3)
	lwz %r25,((3+11)*4)(%r3)
	lfd %f25,((22+11*2)*4)(%r3)
	mtcrf 0xFF,%r0
	lwz %r26,((3+12)*4)(%r3)
	lfd %f26,((22+12*2)*4)(%r3)
	lwz %r27,((3+13)*4)(%r3)
	lfd %f27,((22+13*2)*4)(%r3)
	lwz %r28,((3+14)*4)(%r3)
	lfd %f28,((22+14*2)*4)(%r3)
	lwz %r29,((3+15)*4)(%r3)
	lfd %f29,((22+15*2)*4)(%r3)
	lwz %r30,((3+16)*4)(%r3)
	lfd %f30,((22+16*2)*4)(%r3)
	lwz %r31,((3+17)*4)(%r3)
	lfd %f31,((22+17*2)*4)(%r3)
	mr %r3,%r4
	blr
