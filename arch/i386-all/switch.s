#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:26  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	Dispatch    =	-35
	Enqueue     =	-225
	ThisTask    =	284
	TaskReady   =	428
	tc_Flags    =	16
	tc_State    =	17
	TS_RUN	    =	2
	TS_READY    =	3
	TF_EXCEPT   =	32

	.text
	.align	16
	.globl	Exec_Switch
	.type	Exec_Switch,@function
Exec_Switch:
	/* Make room for Dispatch() address.
	subl	$4,%esp

	/* Preserve registers */
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	/* Get SysBase */
	movl	24(%esp),%ebx

	/* If current state is TS_RUN and TF_EXCEPT is 0... */
	movl	ThisTask(%ebx),%ecx
	movw	tc_Flags(%ecx),%eax
	andb	$TF_EXCEPT,%al
	cmpw	$TS_RUN*256,%ax
	jne	disp

	/* ...move task to the ready list */
	movb	$TS_READY,tc_State(%ecx)
	leal	Enqueue(%ebx),%edx
	pushl	%ebx
	pushl	%ecx
	leal	TaskReady(%ebx),%eax
	pushl	%eax
	call	*%edx
	addl	$12,%esp

	/* Prepare dispatch */
disp:	leal	Dispatch(%ebx),%eax
	movl	%eax,16(%esp)

	/* restore registers and dispatch */
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	ret
