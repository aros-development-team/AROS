/*
    Copyright (C) 1995-1997 AROS - The Amiga Research OS
    $Id$

    Desc: Permit() - Allow tasks switches to occur.
    Lang: english
*/

/*****************************************************************************

    NAME

	AROS_LH0(void, Permit,

    LOCATION
	struct ExecBase *, SysBase, 23, Exec)

    FUNCTION
	This function will reactivate the task dispatcher after a call
	to Forbid(). Note that calls to Forbid() nest, and for every
	call to Forbid() you need a matching call to Permit().

    INPUTS
	None.

    RESULT
	Multitasking will be re-enabled.

    NOTES
	This function preserves all registers.

	To prevent deadlocks calling Wait() in forbidden state breaks
	the forbid - thus taskswitches may happen again.

    EXAMPLE
	No you really don't want to use this function.

    BUGS
	The only architecture that you can rely on the registers being
	saved is on the Motorola mc68000 family and i386 ibmpc.

    SEE ALSO
	Forbid(), Disable(), Enable(), Wait()

    INTERNALS

    HISTORY

******************************************************************************/

#include "machine.i"

	    .text
	    .globl	AROS_SLIB_ENTRY(Permit,Exec)
	    .type	AROS_SLIB_ENTRY(Permit,Exec),@function
AROS_SLIB_ENTRY(Permit,Exec):
	    pushl	%eax
	    movl	8(%esp),%eax
	    decb	TDNestCnt(%eax)
	    jge		.no_switch
	    testb	$0x80,AttnResched(%eax)
	    je		.no_switch
	    cmpb	$0,IDNestCnt(%eax)
	    jge		.no_switch
	    andb	$0x7f,AttnResched(%eax)
	    pushl	%eax
	    call	*Switch(%eax)
	    addl	$4,%esp
.no_switch:
	    popl	%eax
	    ret

