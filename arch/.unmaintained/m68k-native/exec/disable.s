/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(void, Disable,
 
    LOCATION
 	struct ExecBase *, SysBase, 20, Exec)
 
    FUNCTION
 	This function disables the delivery of all interrupts until a matching
 	call to Enable() is done. This implies a Forbid(). Since the system
 	needs the regular delivery of all interrupts it is forbidden to disable
 	them for longer than ~250 microseconds.
 
 	THIS CALL IS VERY DANGEROUS!!!
 
 	Do not use it without thinking very well about it or better do not use
 	it at all. Most of the time you can live without it by using semaphores
 	or similar.
 
 	Calls to Disable() nest, i.e. for each call to Disable() you need one
 	call to Enable().
 
    INPUTS
 
    RESULT
 
    NOTES
 	This function preserves all registers.
 
 	This function may be used from interrupts to disable all higher
 	priority interrupts. Lower priority interrupts are disabled anyway.
 
 	To prevent deadlocks calling Wait() in disabled state breaks the
 	disable - thus interrupts and taskswitches may happen again.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	Forbid(), Permit(), Enable(), Wait()
 
    INTERNALS
 	For old exec Disable() there exists an assembler macro replacement
 	that is (of course) unportable. Using a Disable() implementation
 	equal to this macro would not only have some impact on Disable()
 	itself but also on other functions (e.g. Signal()).
 	Therefore I decided to drop support for this macro to a certain
 	extent. The difference is only minuscule:
 	If a task uses the assembler macro and activates some higher priority
 	task he cannot expect this task to be awakened immediately at Enable()
 	but only at the next context switch. But I do not think that this
 	poses any problems.
 
    HISTORY
 
******************************************************************************/

	INTENA	    =	0xdff09a
	INTEN	    =	0x4000
	SET	    =	0x8000

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Disable,Exec)
	.type	AROS_SLIB_ENTRY(Disable,Exec),@function
AROS_SLIB_ENTRY(Disable,Exec):
	/* disable interrupts */
	move.w	#INTEN,INTENA

	/* increment nesting count and return */
	addq.b	#1,IDNestCnt(a6)
	rts

