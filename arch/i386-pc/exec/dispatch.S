/*
     (C) 1998 AROS - The Amiga Research OS
     $Id$

     Desc: Exec function Dispatch()
     Lang: english
*/

/******************************************************************************

    NAME
	AROS_LH0(void, Dispatch,

    LOCATION
	struct ExecBase *, SysBase, 10, Exec)

    FUNCTION
	You DON'T WANT to know it ;-)))

    INPUTS

    RESULT

    NOTES
	This function DOES NOT save any registers.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include "machine.i"

	.text
	.globl	AROS_SLIB_ENTRY(Dispatch,Exec)
	.type	AROS_SLIB_ENTRY(Dispatch,Exec),@function

AROS_SLIB_ENTRY(Dispatch,Exec):
/* Get the ExacBase. I know it is in %eax so I got it from there. Other place
where ExecBase is is 0x00000004 */
	movl	%eax,%ecx	/* Get ExecBase !*/
	movl	ThisTask(%ecx),%edx

	/* Store real %esp */
	movl	usp,%eax
	movl	%eax,tc_SPReg(%edx)

	/* Call tc_Switch function if needed */
	testb	$TF_SWITCH,tc_Flags(%edx)
	je	1f
	movl	tc_Switch(%edx),%eax
	pushl	%ecx
	pushl	%edx
	call	*%eax
	popl	%edx
	popl	%ecx

1:
	/* Store IDNestCnt */
	movb	IDNestCnt(%ecx),%al
	movb	%al,tc_IDNestCnt(%edx)
	movb	$-1,IDNestCnt(%ecx)

	/* Save task (and fpu) context */
	
	movl	tc_ETask(%edx),%eax	
	movl	iet_Context(%eax),%ebx	/* task->tc_UnionETask.tc_ETask->iet_Context */
	pushl	%ebx
	call	StoreCPU		/* Store all registers */
	addl	$4,%esp

	movl	iet_FPU(%eax),%ebx
	pushl	%ebx
	call	StoreFPU
	addl	$4,%esp

	/* Remove task from ready list */
	movl	TaskReady(%ecx),%edx
	movl	(%edx),%eax
	movl	%eax,TaskReady(%ecx)
	movl	(%edx),%eax
	leal	TaskReady(%ecx),%ebx
	movl	%ebx,4(%eax)
	movl	%edx,ThisTask(%ecx)

	/* Use as current task */
	movb	$TS_RUN,tc_State(%edx)
	movb	tc_IDNestCnt(%edx),%al
	movb	%al,IDNestCnt(%ecx)

	/* Get task (and fpu) context */
	
	movl	tc_ETask(%edx),%eax	
	movl	iet_Context(%eax),%ebx	/* task->tc_UnionETask.tc_ETask->iet_Context */
	pushl	%ebx
	call	RestoreCPU		/* restore all registers */
	addl	$4,%esp

	movl	iet_FPU(%eax),%ebx
	pushl	%ebx
	call	RestoreFPU
	addl	$4,%esp

	/* Launch bit set? */
	testb	$TF_LAUNCH,tc_Flags(%edx)
	je	2f
	movl	tc_Launch(%edx),%eax
	pushl	%ecx
	pushl	%edx
	call	*%eax
	popl	%edx
	popl	%ecx

2:
	/* Get new sp */
	movl	tc_SPReg(%edx),%eax

	/* Compare agains SPLower */
	cmpl	%eax,tc_SPLower(%edx)
	ja	3f

	/* Compare against SPUpper */
	cmpl	%eax,tc_SPUpper(%edx)
	ja	4f

3:
	/* Call Alert() */
	pushl	%ecx
	pushl	$(AT_DeadEnd|AN_StackProbe)
	call	*Alert(%ecx)
	/* Function does not return */
	nop

4:
	/* Put the SP into the correct register after checking */
	movl	%eax,usp

	/* Except bit set? */
	testb	$TF_EXCEPT,tc_Flags(%edx)
	je	6f

	/* Raise task exception in Disable()d state */
	pushl	%ecx
	call	*Exception(%ecx)
	popl	%ecx

6:
	ret		/* Just return. Macros will do all switches now */
