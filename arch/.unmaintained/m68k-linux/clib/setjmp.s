/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
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

	#include "machine.i"

#define FirstArg	4 /* Skip Return-Address */
#define	env		FirstArg
#define	retaddr		0

	.text
	.balign 16
	.globl	AROS_CDEFNAME(setjmp)
	.type	AROS_CDEFNAME(setjmp),@function
AROS_CDEFNAME(setjmp):
#ifdef OLDJMP
	move.l	env(%sp),%a0		/* save area pointer */
	move.l	retaddr(%sp),(%a0)+	/* save old PC */
	lea.l	env(%sp),%a1		/* adjust saved SP since we won't rts */
	move.l	%a1,(%a0)+		/* save old SP */
	movem.l	%d2-%d7/%a2-%a6,(%a0)	/* save remaining non-scratch regs */
	clr.l	%d0			/* return 0 */
	rts
#else
	move.l	env(%sp),%a0			/* get address of jmp_buf */
	move.l	(%sp),(%a0)+			/* store return address */
	movem.l	%d2-%d7/%a2-%a6/%sp,(%a0)	/* store all registers except scratch */
	moveq.l	#0,%d0				/* return 0 */
	rts
#endif

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
*/
