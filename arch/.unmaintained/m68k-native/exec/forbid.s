/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
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
 
 	Do not use it without thinking very well about it or better do not use
 	it at all. Most of the time you can live without it by using semaphores
 	or similar.
 
 	Calls to Forbid() nest, i.e. for each call to Forbid() you need one
 	call to Enable().
 
    INPUTS
 
    RESULT
 
    NOTES
 	This function preserves all registers.
 
 	To prevent deadlocks calling Wait() in forbidden state breaks the
 	firbid - thus taskswitches may happen again.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	Permit(), Disable(), Enable(), Wait()
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Forbid,Exec)
	.type	AROS_SLIB_ENTRY(Forbid,Exec),@function
AROS_SLIB_ENTRY(Forbid,Exec):
	/* increment nesting count and return
	   this seems to be a very unspectacular call and a good job for a C
	   routine - but nothing in C can guarantee to preserve all registers
	   and to increment atomically - so better use this simple assembly
	   routine
	*/
	addq.b	#1,TDNestCnt(a6)
	rts


