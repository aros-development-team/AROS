/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX function vfork()
    Lang: english
*/

#include "aros/riscv/asm.h"

	.text
	.align	2
	.global AROS_CDEFNAME(vfork)
	.type	AROS_CDEFNAME(vfork),%function

AROS_CDEFNAME(vfork):
	/*
	 * Allocate a jmp_buf (jmpbuf_SIZEOF is a 16-byte multiple, see
	 * aros/stdc/setjmp.h) plus a 16-byte slot for the original ra and
	 * sp. They must survive the call to setjmp, and the t registers
	 * are caller-saved (a linklib stub for setjmp may clobber them),
	 * so they are stashed on the stack, not in scratch registers.
	 */
	addi	sp, sp, -(jmpbuf_SIZEOF+16)
	sw	ra, jmpbuf_SIZEOF+0(sp)		/* stash caller's ra           */
	addi	a0, sp, jmpbuf_SIZEOF+16
	sw	a0, jmpbuf_SIZEOF+4(sp)		/* stash caller's sp           */

	mv	a0, sp				/* a0 = jmp_buf                */
	call	setjmp				/* fill with current state     */

	/* Patch the jmp_buf: return to our caller, on our caller's stack */
	lw	a1, jmpbuf_SIZEOF+0(sp)
	sw	a1, retaddr(sp)			/* retaddr = caller's ra       */
	lw	a1, jmpbuf_SIZEOF+4(sp)
	sw	a1, 13*4(sp)			/* sp slot = caller's sp       */

	mv	a0, sp				/* a0 = jmp_buf for __vfork    */
	call	__vfork				/* never returns               */
