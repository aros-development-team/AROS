/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 function siglongjmp()
    Lang: english
*/

#include "aros/ppc/asm.h"

	.text
	_ALIGNMENT
	.globl AROS_CDEFNAME(siglongjmp)
	_FUNCTION(siglongjmp)
AROS_CDEFNAME(siglongjmp):
	cmpi 0,1,%r4,0
	bne okret
	li %r4,1
okret:
	lwz %r1,   4(3)
	lwz %r2,   8(3)
	lwz %r0,   0(3)
	lwz %r14, 16(3)
	lfd %f14, 88(3)
	lwz %r15, 20(3)
	lfd %f15, 96(3)
	lwz %r16, 24(3)
	lfd %f16,104(3)
	lwz %r17, 28(3)
	lfd %f17,112(3)
	lwz %r18, 32(3)
	lfd %f18,120(3)
	lwz %r19, 36(3)
	lfd %f19,128(3)
	lwz %r20, 40(3)
	lfd %f20,136(3)
	mtlr %r0
	lwz %r21, 44(3)
	lfd %f21,144(3)
	lwz %r22, 48(3)
	lfd %f22,152(3)
	lwz %r0,  12(3)
	lwz %r23, 52(3)
	lfd %f23,160(3)
	lwz %r24, 56(3)
	lfd %f24,168(3)
	lwz %r25, 60(3)
	lfd %f25,176(3)
	mtcrf 0xFF,%r0
	lwz %r26, 64(3)
	lfd %f26,184(3)
	lwz %r27, 68(3)
	lfd %f27,192(3)
	lwz %r28, 72(3)
	lfd %f28,200(3)
	lwz %r29, 76(3)
	lfd %f29,208(3)
	lwz %r30, 80(3)
	lfd %f30,216(3)
	lwz %r31, 84(3)
	lfd %f31,224(3)
	mr %r3,%r4
	blr
