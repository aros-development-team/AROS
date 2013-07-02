/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Obtain and install a Quick Interrupt vector
    Lang: english
*/


/*****************************************************************************

    NAME

	AROS_LH1(ULONG, ObtainQuickVector,

    SYNOPSIS
	AROS_LHA(APTR, interruptCode, A0),

    LOCATION
	struct ExecBase *, SysBase, 131, Exec)

    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/

	#include "aros/m68k/asm.h"


	.text
	.balign 4
	.globl AROS_SLIB_ENTRY(ObtainQuickVector,Exec,131)
	.type	AROS_SLIB_ENTRY(ObtainQuickVector,Exec,131),@function
AROS_SLIB_ENTRY(ObtainQuickVector,Exec,131):

	move.l	%a5,%a1
	lea		.super(%pc),%a5
	jmp		Supervisor(%a6)
.super:
	or.w	#0x0700,%sr
	move.l	%a1,%a5
	sub.l	%a1,%a1
	btst	#0,%a6@(AttnFlags+1)
	beq.s	3f
	movec	%vbr,%a1
	/* start from 0x200 downwards because we may
	 * have arosbootstrap fake exec above 0x200 */
3:	move.l	#0x200,%d0
2:	subq.l	#4,%d0
	cmp.w	#0x100,%d0
	beq.s	0f
	move.l	0(%a1,%d0.w),%d1
	bne.s	2b
	move.l	%a0,(%a1,%d0.w)
	lsr.l	#2,%d0
1:	rte
0:	moveq	#0,%d0
	bra.s	1b
	