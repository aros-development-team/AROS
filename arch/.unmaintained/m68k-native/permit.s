#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.6  1996/11/16 01:31:04  aros
#    Fixed hex from $ to 0x
#
#    Revision 1.5  1996/11/01 02:05:24  aros
#    Motorola syntax (no more MIT)
#
#    Revision 1.4  1996/10/24 15:51:31  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.3  1996/10/24 01:38:31  aros
#    Include machine.i
#
#    Revision 1.2  1996/08/01 17:41:36  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME */
#
#	AROS_LH0(void, Permit,
#
#   LOCATION
#	struct ExecBase *, SysBase, 23, Exec)
#
#   FUNCTION
#	This function activates the dispatcher again after a call to Permit().
#
#   INPUTS
#
#   RESULT
#
#   NOTES
#	This function preserves all registers.
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#	Forbid(), Disable(), Enable()
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	.include "machine.i"

	.text
	.balign 16
	.globl	_Exec_Permit
	.type	_Exec_Permit,@function

_Exec_Permit:
	# decrement nesting count and return if there are Forbid()s left
	subq.b	#1,TDNestCnt(a6)
	bpl	end

	# return if there are no delayed switches pending.
	tst.b	AttnResched+1(a6)
	bpl	end

	# if IDNestCnt is not -1 taskswitches are still forbidden
	tst.b	IDNestCnt(a6)
	bpl	end

	# Unset delayed switch bit and do the delayed switch
	bclr	#7,0x12b(a6)
	jsr    Switch(a6)

	# all done.
end:	rts

