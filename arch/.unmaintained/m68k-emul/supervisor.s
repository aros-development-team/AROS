#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/12/05 15:31:01  aros
#    Patches by Geert Uytterhoeven integrated
#
#    Revision 1.2  1996/08/01 17:41:32  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	.globl	_Exec_Supervisor
	.type	_Exec_Supervisor,@function
_Exec_Supervisor:
	# The emulation has no real supervisor mode.
	jmp %a5@
