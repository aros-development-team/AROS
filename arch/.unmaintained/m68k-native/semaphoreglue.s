#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.5  1996/11/01 02:05:24  aros
#    Motorola syntax (no more MIT)
#
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

	# The following functions are guaranteed to preserve
	# all registers. But I don't want to write them completely
	# in assembly - C is generally more readable.
	# So I use those stubs to preserve the registers.

	.text
	.balign 16
	.globl	_Exec__ObtainSemaphore
	.type	_Exec__Obtainsemaphore,@function
_Exec__ObtainSemaphore:
	movem.l	d0-d1/a0-a1,-(sp)
	bsr	_Exec_ObtainSemaphore
	movem.l	(sp)+,d0-d1/a0-a1
	rts

	.globl	_Exec__ReleaseSemaphore
	.type	_Exec__ReleaseSemaphore,@function
_Exec__ReleaseSemaphore:
	movem.l	d0-d1/a0-a1,-(sp)
	bsr	_Exec_ReleaseSemaphore
	movem.l	(sp)+,d0-d1/a0-a1
	rts

	.globl	_Exec__ObtainSemaphoreShared
	.type	_Exec__ObtainSemaphoreShared,@function
_Exec__ObtainSemaphoreShared:
	movem.l	d0-d1/a0-a1,-(sp)
	bsr	_Exec_ObtainSemaphoreShared
	movem.l	(sp)+,d0-d1/a0-a1
	rts

