#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:35  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME */
#
#	__AROS_LH0(void, Forbid,
#
#   LOCATION
#	struct ExecBase *, SysBase, 22, Exec)
#
#   FUNCTION
#	Forbid any further taskswitches until a matching call to Permit().
#	Naturally disbaling taskswitches means:
#
#	THIS CALL IS DANGEROUS
#
#	Do not use it without thinking very well about it or better don't use
#	it at all. Most of the time you can live without it by using semaphores
#	or similar.
#
#	Calls to Forbid() nest, i.e. for each call to Dorbid() you need one
#	call to Enable().
#
#   INPUTS
#
#   RESULT
#
#   NOTES
#	This function preserves all registers.
#
#	To prevent deadlocks calling Wait() in forbidden state breaks the
#	firbid - thus taskswitches may happen again.
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#	Permit(), Disable(), Enable(), Wait()
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	TDNestCnt   =	0x127

	.globl	_Exec_Forbid
_Exec_Forbid:
	# increment nesting count and return
	# this seems to be a very unspectacular call and a good job for a C
	# routine - but nothing in C can guarantee to preserve all registers
	# and to increment atomically - so better use this simple assembly
	# routine
	addqb	#1,a6@(TDNestCnt)
	rts


