#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/10/21 21:08:58  aros
#    Changed __AROS_LA to __AROS_LHA
#
#    Revision 1.2  1996/08/01 17:41:36  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	# The following functions are guaranteed to preserve
	# all registers. But I don't want to write them completely
	# in assembly - C is generally more readable.
	# So I use those stubs to preserve the registers.

	.text
	.balign 16
	.globl	_Exec__ObtainSemaphore
	.type	_Exec__Obtainsemaphore,@function
_Exec__ObtainSemaphore:
	moveml	d0/d1/a0/a1,sp@-
	jbsr	_Exec_ObtainSemaphore
	moveml	sp@+,d0/d1/a0/a1
	rts

	.globl	_Exec__ReleaseSemaphore
	.type	_Exec__ReleaseSemaphore,@function
_Exec__ReleaseSemaphore:
	moveml	d0/d1/a0/a1,sp@-
	jbsr	_Exec_ReleaseSemaphore
	moveml	sp@+,d0/d1/a0/a1
	rts

	.globl	_Exec__ObtainSemaphoreShared
	.type	_Exec__ObtainSemaphoreShared,@function
_Exec__ObtainSemaphoreShared:
	moveml	d0/d1/a0/a1,sp@-
	jbsr	_Exec_ObtainSemaphoreShared
	moveml	sp@+,d0/d1/a0/a1
	rts

