/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Change the stack of a task.
    Lang: english
*/

/******************************************************************************

    NAME
	AROS_LH1(void, StackSwap,

    SYNOPSIS
	ARHS_LHA(struct StackSwapStruct *, sss, A0)

    LOCATION
	struct ExecBase *, SysBase, 122, Exec)

    FUNCTION
	Change the stack of a task.

    INPUTS
	sss - The description of the new stack

    RESULT
	There will be a new stack.

    NOTES
	Calling this routine the first time will change sss and
	calling it a second time, the changes will be undone.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	This is a symmetrical routine. If you call it twice, then
	everything will be as it was before.

    HISTORY

******************************************************************************/

	#include "machine.i"

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec)
	.type	AROS_SLIB_ENTRY(StackSwap,Exec),@function

	/* The stack looks like this:

	    8 SysBase
	    4 sss
	    0 return address
	*/

#define sss	    4
#define SysBase     8

AROS_SLIB_ENTRY(StackSwap,Exec):
	/* Read parameter sss */
	movl sss(%esp),%edx

	/* copy new SP into ecx */
	movl stk_Pointer(%edx),%ecx

	/* Pop return address and sss from the current stack and copy them
	    onto the one specified in sss */
	popl %eax		    /* pop Return address */
	movl %eax,-12(%ecx)         /* Push return address on new stack */
	popl %eax		    /* pop sss */
	movl %eax,-8(%ecx)          /* Push sss on new stack */

	/* Copy SysBase from the current stack onto the one in sss */
	movl (%esp),%eax
	movl %eax,-4(%ecx)          /* Push SysBase on new stack */

	/* Calc new start of stack in sss */
	addl $-12,%ecx

	/* Call Disable() (SysBase is still on the stack) */
	leal Disable(%eax),%eax
	call *%eax
	popl %eax		    /* Remove SysBase from current stack */

	movl %esp,stk_Pointer(%edx) /* Save current SP in sss */
	movl %ecx,%esp		    /* Load the new stack */

	movl ThisTask(%eax),%ecx
	leal tc_SPLower(%ecx),%ecx  /* ecx = &SysBase->ThisTask->tc_SPLower */

	push %ebx		    /* Save register */

	/* Swap ThisTask->tc_SPLower and sss->stk_Lower */
	movl stk_Lower(%edx),%eax
	movl (%ecx),%ebx
	movl %eax,(%ecx)
	movl %ebx,stk_Lower(%edx)

	/* Swap tc_SPUpper and sss->stk_Upper, too */
	movl stk_Upper(%edx),%eax
	movl 4(%ecx),%ebx
	movl %eax,4(%ecx)
	movl %ebx,stk_Upper(%edx)

	popl %ebx		    /* Restore register */

	/* Call Enable() */
	movl SysBase(%esp),%eax
	pushl %eax		    /* push SysBase on new stack */
	leal Enable(%eax),%eax      /* call enable */
	call *%eax
	addl $4,%esp		    /* Clean stack */

	/* Note that at this time, the new stack from sss contains the
	   same values as the previous stack */
	ret
