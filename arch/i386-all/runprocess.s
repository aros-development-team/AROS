#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#
#    Desc: DOS utility function RunProcess
#    Lang: english
#
# LONG RunProcess ( struct Process         * proc,
#		    struct StackSwapStruct * sss,
#		    STRPTR		     argptr,
#		    ULONG		     argsize,
#		    LONG_FUNC		     entry,
#		    struct DosLibrary	   * DOSBase

	.include "machine.i"

	FirstArg = 4+(4*4)   /* Return-Adress + 4 Registers */
	proc	 = FirstArg
	sss	 = proc+4    /* 24 */
	argptr	 = sss+4
	argsize  = argptr+4
	entry	 = argsize+4 /* 36 */
	DOSBase  = entry+4

	.text
	.balign 16
	.globl	_Dos_RunProcess
	.type	_Dos_RunProcess,@function

_Dos_RunProcess:
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
	/* Make room for one pointer */
	addl $-4,%eax
	/* Push sss onto the new stack */
	movl %ebx,(%eax)

	/* Make room for another pointer */
	addl $-4,%eax
	/* Get SysBase */
	movl DOSBase(%esp),%edx
	movl dl_SysBase(%edx),%edx
	/* Push SysBase on the new stack */
	movl %edx,(%eax)

	/* Store switch point in sss */
	movl %eax,stk_Pointer(%ebx)

	/* Push SysBase and sss on our stack */
	pushl %edx /* SysBase */
	pushl %ebx /* sss */
	/* Switch stacks */
	leal StackSwap(%edx),%edx
	call *%edx
	/* Clean (new) stack */
	addl $8,%esp

	/* Call the specified routine */
	call *%edi

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
	/* Clean our stack */
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
