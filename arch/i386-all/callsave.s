/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Call a function preserving all registers.
    Lang: english
*/

/******************************************************************************

    NAME
#include <setjmp.h>

	void callsave (void (* func)(...), ...);

    FUNCTION
	Save registers, call function and restore registers again.

    INPUTS
	func - This function is called. Up to four parameters can be
		passed to func.

    RESULT
	None.

    NOTES

    EXAMPLE
	void _Permit (void)
	{
	    if ((--SysBase->TDNestCnt) < 0
		&& (SysBase->AttnResched & 0x80)
		&& SysBase->IDNestCnt < 0
	    )
	    {
		SysBase->AttnResched &= ~0x80;

		Switch ();
	    }
	}

	// This function is guranteed to preserve all registers.
	void Permit (void)
	{
	    // ... so preserve them, to the work and then restore them
	    // again.
	    callsave (_Permit);
	}

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

	#include "machine.i"

	.text
	.balign 16
	.globl	AROS_CDEFNAME(callsave)
	.type	AROS_CDEFNAME(callsave),@function

	.set	FirstArg, 7*4 /* Skip Return-Adress + 7 registers */
	.set	lastarg, FirstArg+4*4 /* Last of the four args */
	.set	func, 12*4 /* 4 args + 7 regs + ret addr */

AROS_CDEFNAME(callsave):
	/* Save all registers */
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp

	/* Copy four additional arguments */
	movl  lastarg(%esp),%eax
	pushl  %eax		   /* This adds 4 to %esp !! */
	movl  lastarg(%esp),%eax
	pushl  %eax
	movl  lastarg(%esp),%eax
	pushl  %eax
	movl  lastarg(%esp),%eax
	pushl  %eax

	/* Call function */
	movl  func(%esp),%eax
	call  *%eax

	/* Clean stack */
	addl  $16,%esp

	/* Restore all registers */
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	ret
