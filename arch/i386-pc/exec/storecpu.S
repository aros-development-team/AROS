/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Store cpu state.
    Lang: english
*/

/*****************************************************************************

    NAME
	StoreCPU(

    SYNOPSIS
	Context)

    FUNCTION
	Store CPU registers in given area.

    INPUTS
	Context - APTR to 4*8 bytes of memory.

    RESULT

    NOTES
	Generally you shouldn't use this function from user mode.

    EXAMPLE

    BUGS

    SEE ALSO
	StoreFPU(), RestoreCPU(), RestoreFPU()

    INTERNALS

    HISTORY

******************************************************************************/

#include "machine.i"

			.text
			.balign	16
			.globl	StoreCPU
			.type	StoreCPU,@function

StoreCPU:		cmpb	$0,supervisor	/* In supervisor mode? */
			je	.user_store
			pushl	%ebp
			movl	%esp,%ebp
			pushl	%esi
			pushl	%edi
			pushl	%ecx
			movl	(esp),%esi	/* Copy from esp */
			movl	8(%ebp),%edi	/* To Context */
			movl	$8,%ecx		/* 8 registers */
			cld
			rep
			movsl			/* GO! */
			popl	%ecx
			popl	%edi
			popl	%esi
			popl	%ebp
			ret
.user_store:		pushal			/* Store all registers */
			movl	%esp,%esi	/* Copy from stack */
			movl	36(%esp),%edi	/* To Context */
			movl	$8,%ecx		/* 8 registers */
			cld
			rep
			movsl			/* GO! */
			popal
			ret
