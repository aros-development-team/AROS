#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.5  1996/11/01 02:05:23  aros
#    Motorola syntax (no more MIT)
#
#    Revision 1.4  1996/10/24 15:51:30  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.3  1996/10/24 01:38:31  aros
#    Include machine.i
#
#    Revision 1.2  1996/08/01 17:41:34  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#
#	AROS_LH0(void, Disable,
#
#   LOCATION
#	struct ExecBase *, SysBase, 20, Exec)
#
#   FUNCTION
#	This function disables the delivery of all interrupts until a matching
#	call to Enable() is done. This implies a Forbid(). Since the system
#	needs the regular delivery of all interrupts it's forbidden to disable
#	them for longer than ~250 microseconds.
#
#	THIS CALL IS VERY DANGEROUS!!!
#
#	Do not use it without thinking very well about it or better don't use
#	it at all. Most of the time you can live without it by using semaphores
#	or similar.
#
#	Calls to Disable() nest, i.e. for each call to Disable() you need one
#	call to Enable().
#
#   INPUTS
#
#   RESULT
#
#   NOTES
#	This function preserves all registers.
#
#	This function may be used from interrupts to disable all higher
#	priority interrupts. Lower priority interrupts are disabled anyway.
#
#	To prevent deadlocks calling Wait() in disabled state breaks the
#	disable - thus interrupts and taskswitches may happen again.
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#	Forbid(), Permit(), Enable(), Wait()
#
#   INTERNALS
#	For old exec Disable() there exists an assembler macro replacement
#	that is (of course) importable. Using a Disable() implementation
#	equal to this macro would not only have some impact on Disable()
#	itself but also on other functions (e.g. Signal()).
#	Therefore I decided to drop support for this macro to a certain
#	extend. The difference is only miniscule:
#	If a task uses the assembler macro and activates some higher priority
#	task he cannot expect this task to be awakened immediately at Enable()
#	but only at the next context switch. But I don't think that this
#	poses any problems.
#
#   HISTORY
#
#******************************************************************************

	INTENA	    =	0xdff09a
	INTEN	    =	0x4000
	SET	    =	0x8000

	.include "machine.i"

	.text
	.balign 16
	.globl	_Exec_Disable
	.type	_Exec_Disable,@function

_Exec_Disable:
	# disable interrupts
	move.w	#INTEN,INTENA

	# increment nesting count and return
	addq.b	#1,IDNestCnt(a6)
	rts

