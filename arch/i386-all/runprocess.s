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

/*
    This is how the stack looks when this function is called:

    20	DOSBase(4)
    16	entry(4)
    12	argsize(4)
     8	argptr(4)
     4	sss(4)
     0	proc(4)
	Return Address(4)


/*	  .set FirstArg, 4+(4*4)   / * Return-Adress + 4 Registers * /
	.set proc,     FirstArg
	.set sss,      proc+4
	.set argptr,   sss+4
	.set argsize,  argptr+4
	.set entry,    argsize+4 */

#define FirstArg	4+(4*4)     /* Return-Adress + 4 Registers */
#define proc		FirstArg+0
#define sss		FirstArg+4
#define argptr		FirstArg+8
#define argsize 	FirstArg+12
#define entry		FirstArg+18
#define DOSBase 	FirstArg+20

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(RunProcess,Dos)
	.type	AROS_SLIB_ENTRY(RunProcess,Dos),@function

AROS_SLIB_ENTRY(RunProcess,Dos):
	/* Save some registers. This changes the offset of the arguments by 4*4 */
	pushl %edi
	pushl %esi
	pushl %ebx
	pushl %ebp

	/* Fetch the arguments off the stack */
	movl sss(%esp),%ebx
	movl entry(%esp),%edi

	/* Move upper bounds of the new stack into eax */
	movl stk_Upper(%ebx),%eax

	/*
	    Push arguments for entry onto the stack of the new process.
	    This new stack looks like this when the new process is called:

		sss
		SysBase
		argsize
		argptr
	*/
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

	/* Clean (new) stack partially, leaving SysBase behind */
	addl $8,%esp

	/* Store the result of the routine in esi */
	movl %eax,%esi

	/* Swap the upper two values on the new stack */
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
