/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: longjmp
    Lang: english
*/

/******************************************************************************

    NAME
#include <setjmp.h>

	void longjmp (jmp_buf env, int val);

    FUNCTION
	Save the current context so that you can return to it later.

    INPUTS
	env - The context/environment to restore
	val - This value is returned by setjmp() when you return to the
		saved context. You cannot return 0. If val is 0, then
		setjmp() returns with 1.

    RESULT
	This function doesn't return.

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
	setjmp()

    INTERNALS

    HISTORY

******************************************************************************/

	#include "machine.i"

#define FirstArg	4 /* Skip Return-Adress */
#define env		FirstArg
#define val		FirstArg+4

	.text
	.balign 16
	.globl	AROS_CDEFNAME(longjmp)
	.type	AROS_CDEFNAME(longjmp),@function
AROS_CDEFNAME(longjmp):
	move.l	env(%sp),%a0		/* save area pointer */
	move.l	val(%sp),%d0
	jbne	.okret
	moveq.l	#1,%d0			/* make sure it isn't 0 */
.okret:
	move.l	4(%a0),%sp		/* restore SP */
	move.l	0(%a0),(%sp)		/* restore PC */
	movem.l	8(%a0),%d2-%d7/%a2-%a6	/* restore remaining non-scratch regs */
	rts

/*
	The jmp_buf is filled as follows (d0/d1/a0/a1 are not saved):

	_jmp_buf	offset	contents
	[0]   		0	old pc
	[1]		4	old sp
	[2]		8	d2
	[3]		12	d3
	[4]		16	d4
	[5]		20	d5
	[6]		24	d6
	[7]		28	d7
	[8]		32	a2
	[9]		36	a3
	[10]		40	a4
	[11]		44	a5
	[12]		48	a6
*/
