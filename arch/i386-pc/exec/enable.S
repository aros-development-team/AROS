/*
    Copyright (C) 1995-1997 AROS - The Amiga Research OS
    $Id$

    Desc: Enable() - Allow interrupts to occur after Disable().
    Lang: english
*/

/*****************************************************************************

    NAME

	AROS_LH0(void, Enable,

    LOCATION
	struct ExecBase *, SysBase, 21, Exec)

    FUNCTION
	This function will allow interrupts to occur after they have
	been disabled by Disable().

	Note that calls to Disable() nest, and for every call to
	Disable() you need a matching call to Enable().

	***** WARNING *****

	Using this function is considered very harmful, and it is
	not recommended to use this function for ** ANY ** reason.

	It is quite possible to either crash the system, or to prevent
	normal activities (disk/port i/o) from occuring.

	Note: As taskswitching is driven by the interrupts subsystem,
	      this function has the side effect of disabling
	      multitasking.

    INPUTS
	None.

    RESULT
	Interrupts will be enabled again when this call returns.

    NOTES
	This function preserves all registers.

	To prevent deadlocks calling Wait() in disabled state breaks
	the disable - thus interrupts may happen again.

    EXAMPLE
	No you DEFINITATELY don't want to use this function.

    BUGS
	The only architecture that you can rely on the registers being
	saved is on the Motorola mc68000 family and i386 ibmpc.

    SEE ALSO
	Forbid(), Permit(), Disable(), Wait()

    INTERNALS

    HISTORY

******************************************************************************/

#include "machine.i"

	    .text
	    .globl	AROS_SLIB_ENTRY(Enable,Exec)
	    .type	AROS_SLIB_ENTRY(Enable,Exec),@function
AROS_SLIB_ENTRY(Enable,Exec):
	    pushl	%eax
	    movl	8(%esp),%eax
	    decb	IDNestCnt(%eax)
	    jge		.not_switch
	    sti
	    testb	$0x80,AttnResched(%eax)
	    je		.not_switch
	    cmpb	$0,TDNestCnt(%eax)
	    jge		.not_switch
	    andb	$0x7f,AttnResched(%eax)
	    pushl	%eax
	    call	*Switch(%eax)
	    addl	$4,%esp
.not_switch:
	    popl	%eax
	    ret

