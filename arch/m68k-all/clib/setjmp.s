/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: setjmp
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

******************************************************************************/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	setjmp

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	env, FirstArg
	.set	retaddr, 0

setjmp:
	/* New version adapted from libnix instead of ixemul.
         * Note the slightly different register save order.
         */
	move.l	4(%sp),%a0		/* get address of jmp_buf */
	move.l	(%sp),%a0@+		/* store return address */
	movem.l	%d2-%d7/%a2-%a6/%sp,%a0@	/* store all registers except scratch */
	lea.l	%a0@(12*4),%a0
	move.l	%a0,%sp@-		/* Save A0 for later */

	/* D0 = *(ULONG *)(FindTask(NULL)->tc_SPLower) */
	move.l	SysBase,%a6
	sub.l	%a1, %a1
	jsr	%a6@(FindTask)
	move.l	%d0, %a0
	move.l	%a0@(tc_SPLower),%a0
	move.l	%a0@,%d0

	move.l	%sp@+,%a0		/* Restore A0 from stack */
	move.l	%d0,%a0@+		/* Put RelBase index into jmp_buf */
	moveq.l	#0,%d0			/* return 0 */
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
