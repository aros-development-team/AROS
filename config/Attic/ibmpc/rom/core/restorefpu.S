/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Restore FPU state.
    Lang: english
*/

/*****************************************************************************

    NAME
	RestoreFPU(

    SYNOPSIS
	FPU)
	
    FUNCTION
	Restores whole FPU state.
	
    INPUTS
	FPU - 108 bytes long memory space with the FPU state.

    RESULT

    NOTES
	You shouldn't generally use it from user mode. BTW... Why you're using
	it at all?
	
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include "machine.i"

			.text
			.globl	RestoreFPU
			.type	RestoreFPU,@function

RestoreFPU:		pushl	%edi
			movl	8(%esp),%edi
			frstor	(%edi)
			popl	%edi
			ret
