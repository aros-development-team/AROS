/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Forbid() - Prevent tasks switches from taking place.
    Lang: english
*/

/*****************************************************************************

    NAME

	AROS_LH0(void, Forbid,

    LOCATION 
	struct ExecBase *, SysBase, 22, Exec)

    FUNCTION
	Forbid any further taskswitches until a matching call to Permit().
	Naturally disabling taskswitches means:

	THIS CALL IS DANGEROUS

	Do not use it without thinking very well about it or better
	do not use it at all. Most of the time you can live without
	it by using semaphores or similar.

	Calls to Forbid() nest, i.e. for each call to Forbid() you
	need one call to Permit().

    INPUTS
	None.

    RESULT
	The multitasking state will be disabled AFTER this function
	returns to the caller.

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
	Permit(), Disable(), Enable(), Wait()

    HISTORY

******************************************************************************/

#include "machine.i"

	    .text
	    .globl 	AROS_SLIB_ENTRY(Forbid,Exec)
	    .type  	AROS_SLIB_ENTRY(Forbid,Exec),@function
AROS_SLIB_ENTRY(Forbid,Exec):
	    pushl	%eax
	    movl	8(%esp),%eax
	    incb	TDNestCnt(%eax)
	    popl	%eax
	    ret
