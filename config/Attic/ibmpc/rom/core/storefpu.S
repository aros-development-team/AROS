/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Stores FPU state.
    Lang: english
*/

/*****************************************************************************

    NAME
	StoreFPU(

    SYNOPSIS
	FPU)
	
    FUNCTION
	Stores whole FPU state in memory.
	
    INPUTS
	FPU - 108 bytes long memory space for keeping the state.

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
			.globl	StoreFPU
			.type	StoreFPU,@function

StoreFPU:		pushl	%edi
			movl	8(%esp),%edi
			fsave	(%edi)
			fwait
			popl	%edi
			ret
