/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: setjmp
    Lang: english
*/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	setjmp

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	env, FirstArg

setjmp:
	/* New version adapted from libnix instead of ixemul.
         * Note the slightly different register save order.
         */
	move.l	4(%sp),%a0		/* get address of jmp_buf */
	move.l	(%sp),(%a0)+		/* store return address */
	movem.l	%d2-%d7/%a2-%a6/%sp,(%a0)	/* store all registers except scratch */
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
*/
