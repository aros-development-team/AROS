/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: AndIMask() - Ands interrupt enable mask.
    Lang: english
*/

/*****************************************************************************

    NAME

	AndIMask

    SYNOPSIS
	UBYTE, mask

    FUNCTION
	Ands the interrupt enable mask with giver parameter. This function is
	used ONLY by exec library. Private function.
	
    INPUTS
	mask - the intena bits will be and'ed with given value.

    RESULT
	some ints may be reenabled after this call. You should always enable
	irq 2 because it's a cascade int.	

    NOTES
	This is IBMPC specific function.

    EXAMPLE

    BUGS

    SEE ALSO
	OrIMask()

    INTERNALS

    HISTORY

******************************************************************************/

			.text
			.globl	AndIMask
			.type	AndIMask,@function
AndIMask:		pushl	%ebx
			movl	8(%esp),%ebx
			andl	$0xfffffffb,%ebx /* Make sure irq2 is enabled */
			inb	$0x21,%al
			andb	%bl,%al
			outb	%al,$0x21
			inb	$0xa1,%al
			andb	%bh,%al
			outb	%al,$0xa1
			popl	%ebx
			ret
