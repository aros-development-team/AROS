/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: OrIMask() - Ors interrupt enable mask.
    Lang: english
*/

/*****************************************************************************

    NAME

	OrIMask

    SYNOPSIS
	UBYTE, mask

    FUNCTION
	Or's the interrupt enable mask with giver parameter. This function is
	used ONLY by exec library. Private function.
	
    INPUTS
	mask - the intena bits will be or'ed with given value.

    RESULT
	some ints may be disabled after this call. You MUST NOT disable irq 2
	because it's a cascade int.

    NOTES
	This is IBMPC specific function.

    EXAMPLE

    BUGS

    SEE ALSO
	AndIMask()

    INTERNALS

    HISTORY

******************************************************************************/

			.text
			.globl	OrIMask
			.type	OrIMask,@function
OrIMask:		pushl	%ebx
			movl	8(%esp),%ebx
			andl	$0xfffffffb,%ebx	/* Make sure nobody touches bit 2 */
			inb	$0x21,%al
			orb	%bl,%al
			outb	%al,$0x21
			inb	$0xa1,%al
			orb	%bh,%al
			outb	%al,$0xa1
			popl	%ebx
			ret
