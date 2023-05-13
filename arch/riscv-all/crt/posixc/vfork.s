/*
 * Copyright © 2023, The AROS Development Team. All rights reserved.
 * $Id$
 */

#include "aros/riscv/asm.h"

	.text
	.align	2
	.global AROS_CDEFNAME(vfork)
	.type	AROS_CDEFNAME(vfork),%function

AROS_CDEFNAME(vfork):
	add  sp, sp, -4
	sw	ra, 0(sp)			/* Store return-address */
	add	sp, sp, -jmpbuf_SIZEOF		/* Create space for env structure */
	mv	a0, sp
	call setjmp				/* Prepare setjmp */

	lw	a0, jmpbuf_SIZEOF(sp)		/* restore return address */
	sw	a0, retaddr(sp)		/* save ra as first argument of env structure */

	add	a0, sp, jmpbuf_SIZEOF + 4	/* save previous stack pointer into env structure */
	sw	a0, 9*4(sp)

	mv	a0, sp				/* Argument to vfork() */
	jal	__vfork				/* never return... */
