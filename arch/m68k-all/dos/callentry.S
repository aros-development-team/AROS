/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: setsr.S 36262 2010-12-27 12:17:48Z sonic $

    Desc: Call the entry point of a BCPL style routine
    Lang: english
*/

/*****************************************************************************

    NAME

	AROS_UFH7(ULONG, AOS_CallEntry,

    SYNOPSIS
	AROS_UFHA(STRPTR, argptr, A0),
	AROS_UFHA(ULONG,  argsize, D0),
	AROS_UFHA(LONG_FUNC, entry, A3),
	AROS_UFHA(BPTR, startpkt, D1),
	AROS_UFHA(APTR, globvec, A2),
	AROS_UFHA(APTR, splower, A1),
	AROS_UFHA(APTR, returnaddr, A4)

    LOCATION
        dos.library

    FUNCTION
	Call a DOS process

    INPUTS
	argptr  - Argument pointer (advisory)
	          Non-NULL for CLI, NULL for Workbench
	argsize - Length of arguments (for CLI only)
	entry   - Routine to call
	startpkt - BPTR to startup packet (for CLI only)
	globvec - DOS global vector (for CLI only)
	splower - Bottom of the stack

    RESULT
	Return value from calling entry()

    NOTES
	Internal use only

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AOS_CallEntry
AOS_CallEntry:

	/* D0 - Argsize
	 * D1 - startup packet pointer (NULL for CLI)
	 * D2..D7 - 0
	 * A0 - Argptr
	 * A1 - SPLower
	 * A2 - 0/GlobVec
	 * A3 - 0/Entry
	 * A4 - 0/ReturnAddr
	 * A5 - 0/BCPL_jsr
	 * A6 - 0/BCPL_rts
	 * A7 - C Stack
	 */

	movem.l	%d2-%d7/%a2-%a6,%sp@-
	movq.l	#0,%d1
	movq.l	#0,%d2
	movq.l	#0,%d3
	movq.l	#0,%d4
	movq.l	#0,%d5
	movq.l	#0,%d6

	/* Make sure me->pr_ReturnAddr is correct */
	move.l	%a1,%d7
	neg.l	%d7			/* D7 = - tc_SPLower */
	lea.l	%sp@(%d7),%a5		/* A5 = %sp - tc_SPLower */
	move.l	%a5,%sp@-		/* sp+ 4 = stksize */
	move.l	%sp,%a4@		/* Save address of return address */

	/* If A0 is NULL, it's a WB app. */
	move.l	%a0,%d7
	tst.l	%d7
	beq	wb_setup

/* CLI setup
 */
cli_setup:
	lea.l	BCPL_jsr,%a5
	lea.l	BCPL_rts,%a6
	jsr	(%a3)
	bra	cleanup

/* WB setup */
wb_setup:
	pea.l   0f
	move.l	%a3,%sp@-	/* Put the address to call on the stack */
	clr.l	%d7
	move.l	%d7,%a1		/* Clear registers */
	move.l	%d7,%a2
	move.l	%d7,%a3
	move.l	%d7,%a4
	move.l	%d7,%a5
	move.l	SysBase,%a6	/* A number of AROS WB entry points expect this */
	rts			/* Pop and 'return' to the entry point */
0:

cleanup:
	addq.l	#4,%sp		/* Drop the stacksize off the stack */
	movem.l	%sp@+,%d2-%d7/%a2-%a6
	rts
	
