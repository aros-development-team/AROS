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
	This is literal rewrite of MC68000 version plus code to clear
	EE bit in MSR
 
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
	subr
	push	scr

	/* disable amiga chipset interrupts */
	li	scr,INTEN
	stw	scr,INTENA
	/* disable external interrupts in PPC, must be executed in supervisor */
	lwz	arg0,_disab(0)
	jsrlvo	Supervisor,base
	/* we should come back here from _disab */
	/* increment nesting count and return */
	lbz	scr,IDNestCnt(base)
	addi	scr,scr,1
	stb	scr,IDNestCnt(base)

	pop	scr
	rts
	
_disab:
	mfmsr	scr
	andi.	scr,scr,0xFFFF7FFF
	isync
	mtmsr	scr
	sync
	rfi
