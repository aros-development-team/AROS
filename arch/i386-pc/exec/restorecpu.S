/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Restore CPU state.
    Lang: english
*/

/*****************************************************************************

    NAME
	RestoreCPU(

    SYNOPSIS
	Context)

    FUNCTION
	Restores all registers.
	
    INPUTS
	Context - APTR do 8*4 bytes of memory.

    RESULT

    NOTES
	You SHOULD NOT use this function in user mode. It's rather dangerous!

    EXAMPLE

    BUGS

    SEE ALSO
	StoreCPU(), StoreFPU(), RestoreFPU()
	
    INTERNALS

    HISTORY

******************************************************************************/

#include "machine.i"

			.text
			.balign	16
			.globl	RestoreCPU
			.type	RestoreCPU,@function

RestoreCPU:		cmpb	$0,supervisor	/* In supervisor mode? */
			je	.user_restore
			pushl	%ebp
			movl	%esp,%ebp
			pushl	%esi
			pushl	%edi
			pushl	%ecx
			movl	8(%ebp),%esi	/* From Context */
			movl	(esp),%edi	/* To stack */
			movl	$8,%ecx		/* 8 registers */
			cld
			rep
			movsl			/* COPY! */
			popl	%ecx
			popl	%edi
			popl	%esi
			popl	%ebp
			ret
.user_restore:		lea	-32(%esp),%esp	/* Reserve space on stack */
			movl	36(%esp),%esi	/* From Context */
			movl	%esp,%edi	/* To stack */
			movl	$8,%ecx		/* 8 registers */
			cld
			rep
			movsl			/* COPY! */
			popal			/* Restore NEW registers */
			ret
