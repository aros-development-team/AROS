#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.4  1996/10/24 15:51:31  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.3  1996/10/21 21:08:58  aros
#    Changed AROS_LA to AROS_LHA
#
#    Revision 1.2  1996/08/01 17:41:36  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	.globl	_trapvec
_trapvec:
	cmpl	#8,sp@
	jeq	pv
	movel	_storedtrap,sp@-
	rts
pv:	addqw	#4,sp
	jra	_TrapLevel8

	# The only trap I catch is privilege violation
	.globl	_TrapHandler
_Exec_TrapHandler:
	movel	#8,sp@-
	movel	_storedtrap,sp@-
	rts
