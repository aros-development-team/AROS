/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: DOS utility function RunProcess
    Lang: english

 LONG RunProcess ( struct Process         * proc,
		   struct StackSwapStruct * sss,
		   STRPTR		    argptr,
		   ULONG		    argsize,
		   LONG_FUNC		    entry,
		   struct DosLibrary	  * DOSBase )
*/

	#include "machine.i"

	.set FirstArg, 4+(4*4)   /* Return-Adress + 4 Registers */
	.set proc,     FirstArg
	.set sss,      proc+4	 /* 24 */
	.set argptr,   sss+4
	.set argsize,  argptr+4
	.set entry,    argsize+4 /* 36 */

#define DOSBase        entry+4

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(RunProcess,Dos)
	.type	AROS_SLIB_ENTRY(RunProcess,Dos),@function

AROS_SLIB_ENTRY(RunProcess,Dos):
	/* Save some registers */
	pushl %edi
	pushl %esi
	pushl %ebx
	pushl %ebp

	/* Fetch the arguments off the stack */
	movl sss(%esp),%ebx
	movl entry(%esp),%edi

	/* Move upper bounds of the new stack into eax */
	movl stk_Upper(%ebx),%eax

	/* Push arguments for entry onto stack */
	movl argptr(%esp),%edx
	movl %edx,-16(%eax)
	movl argsize(%esp),%edx
	movl %edx,-12(%eax)

	/* Push sss onto the new stack */
	movl %ebx,-4(%eax)

	/* Get SysBase */
	movl DOSBase(%esp),%edx
	movl dl_SysBase(%edx),%edx
	/* Push SysBase on the new stack */
	movl %edx,-8(%eax)

	/* Store switch point in sss */
	addl $-16,%eax
	movl %eax,stk_Pointer(%ebx)

	/* Push SysBase and sss on our stack */
	pushl %edx /* SysBase */
	pushl %ebx /* sss */
	/* Switch stacks */
	leal StackSwap(%edx),%edx
	call *%edx
	/* Clean new stack from call to StackSwap */
	addl $8,%esp

	/* Call the specified routine */
	call *%edi
	/* Clean (new) stack, leaving SysBase behind */
	addl $8,%esp

	/* Store the result of the routine in esi */
	movl %eax,%esi

	/* Swap the upper two values on the stack */
	popl %edx /* SysBase */
	popl %ebx /* sss */
	pushl %edx /* SysBase */
	pushl %ebx /* sss */
	/* Switch stacks back */
	leal StackSwap(%edx),%edx
	call *%edx
	/* Clean old stack */
	addl $8,%esp

	/* Put the result in eax where our caller expects it */
	movl %esi,%eax

	/* Restore registers */
	popl %ebp
	popl %ebx
	popl %esi
	popl %edi

	/* Done */
	ret
