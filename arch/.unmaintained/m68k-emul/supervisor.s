#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:32  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	.globl	_Exec_Supervisor
_Exec_Supervisor:
	# The emulation has no real supervisor mode.
	jmp a5@
