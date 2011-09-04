/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: longjmp
    Lang: english
*/

/******************************************************************************

    NAME
#include <setjmp.h>

	void vfork_longjmp (jmp_buf env, int val);

    FUNCTION
	Save the current context so that you can return to it later.

    INPUTS
	env - The context/environment to restore
	val - This value is returned by setjmp() when you return to the
	      saved context. This version can return 0.

    RESULT
	This function doesn't return.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	setjmp()

    INTERNALS

    HISTORY

******************************************************************************/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	vfork_longjmp

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	env, FirstArg
	.set	val, env+4
	.set	retaddr, 0
vfork_longjmp:
	/* New version adapted from libnix instead of ixemul.
         * Note the slightly different register save order.
         */
	addq.l	#4,%sp			/* returns to other address */
	move.l	(%sp)+,%a0		/* get address of jmp_buf */
	move.l	(%sp)+,%d0		/* get return code */
	move.l	%a0@(48),%sp		/* restore sp */
	move.l	%a0@,(%sp)		/* set return address */
	movem.l	%a0@(4),%d2-%d7/%a2-%a6	/* restore all registers except scratch and sp */
	lea.l	%a0@(52), %a0		/* %a0 is now old RelBase */

	/* Save %d0 and %a0@ for later */
	move.l	%d0,%sp@-
	move.l	%a0@,%d0
	move.l	%d0,%sp@-

	/* a1 = FindTask(NULL)->tc_SPLower */
	move.l	SysBase,%a6
	sub.l	%a1, %a1
	jsr	%a6@(FindTask)
	move.l	%d0, %a1
	move.l	%a1@(tc_SPLower),%a1

	/* %a1@ = %a0@ */
	move.l	%sp@+,%d0
	move.l	%d0,%a1@

	/* Restore %d0 */
	move.l	%sp@+, %d0

	rts

/*
	The jmp_buf is filled as follows (d0/d1/a0/a1 are not saved):

	_jmp_buf	offset	contents
	[0]   		0	old pc
	[1]		4	d2
	[2]		8	d3
	[3]		12	d4
	[4]		16	d5
	[5]		20	d6
	[6]		24	d7
	[7]		28	a2
	[8]		32	a3
	[9]		36	a4
	[10]		40	a5
	[11]		44	a6
	[12]		48	old sp
	[13]		52	*(ULONG *)(FindTask(NULL)->tc_SPLower)
*/
