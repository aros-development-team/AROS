/*
     (C) 1995-96 AROS - The Amiga Research OS
     $Id$
 
     Desc:
     Lang:
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
	This is literal rewrite of MC68000 version
 
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
	PROLOG
	LI	R3,INTEN
	STW	R3,INTENA

	/* increment nesting count and return */

	LBZ	R4,IDNestCnt(r31)
	ADDI	R4,R4,1
	STB	R4,IDNestCnt(r31)
	EPILOG