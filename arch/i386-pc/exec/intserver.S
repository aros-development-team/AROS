/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: IntServer() - Interrupt handler.
    Lang: english
*/

/*****************************************************************************

    NAME
	IntServer

    SYNOPSIS

    FUNCTION
	This is asm version of IntServer which manages the interrupts
	
    INPUTS
	Just nothing. It's an interrupt.

    RESULT

    NOTES
	The VERY INTERNAL. Please don't even touch it unless you know
	what you're doing.
	Interrupt servers should return %eax!=0 if they are sure the interrupt
	was especially for them.
	See /rom/exec/exec_init.c for more information about IntServer routine.

    EXAMPLE

    BUGS

    SEE ALSO
	/rom/exec/exec_init.c
    INTERNALS

    HISTORY

******************************************************************************/

#include "machine.i"
#include "intserv.i"

			.text
			.balign	16
			.globl	IntServer
			.type	IntServer,@function
IntServer:		pushl	%edi
			movl	8(%esp),%edi
			movl	lh_Head(%edi),%edi
.is_loop:		cmpl	$0,ln_Succ(%edi)
			je	.is_exit
			pushl	%edi		/* Store %edi register (int list) */
			movl	4,%eax		/* ExecBase */
			pushl	%eax
			movl	is_Data(%edi),%eax
			pushl	%eax
			movl	is_Code(%edi),%eax
			call	*%eax
			lea	8(%esp),%esp
			popl	%edi
			cmpl	$0,%eax		/* Continue? */
			jne	.is_exit
			movl	ln_Succ(%edi),%edi
			jmp	.is_loop
.is_exit:		popl	%edi
			ret
