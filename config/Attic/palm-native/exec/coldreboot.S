/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(void, ColdReboot,
 
    SYNOPSIS
 
    LOCATION
 	struct ExecBase *, SysBase, 121, Exec)
 
    FUNCTION
	Reboots the Amiga.
 
    INPUTS
 
    RESULT
 
    NOTES
	This function never returns.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "../machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(ColdReboot,Exec)
AROS_SLIB_ENTRY(ColdReboot,Exec):
	jsr	Disable(%a6)		/* disable */
	moveq.l	#0,%d0			/* cache bits */
	moveq.l	#-1,%d1			/* cache mask */
	jsr	CacheControl(%a6)	/* disable all cache modes */

	lea.l	resetus(%pc),%a5	/* actual reset routine */
	jsr	Supervisor(%a6)		/* supervisor mode, ofcourse */

	.balign 4
resetus:
#warning FIXME! This is code that might work on an Amiga but not on Palm.
	lea.l	0x1000000,%a0		/* end of rom */
	sub.l	-0x14(%a0),%a0		/* subtract size of rom */
	move.l	4(%a0),%a0		/* get vector from rom+4 */
	subq.w	#2,%a0			/* point to reset instruction */
	reset				/* reset stuff */
	jmp	(%a0)			/* this instruction was still in prefetch */
	/* never gets here */
