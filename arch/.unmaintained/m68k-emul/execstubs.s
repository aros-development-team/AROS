/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: glue functions
    Lang: english
*/

#include "machine.i"

/*
    The following functions are guaranteed to preserve
    all registers. But I don't want to write them completely
    in assembly - C is generally more readable.
    So I use those stubs to preserve the registers.
*/

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(ObtainSemaphore,Exec)
	.type	AROS_SLIB_ENTRY(ObtainSemaphore,Exec),@function
AROS_SLIB_ENTRY(ObtainSemaphore,Exec):
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
#if !UseRegisterArgs
	move.l	24(%sp),-(%sp)
	move.l	24(%sp),-(%sp)
#endif
	jbsr	AROS_CSYMNAME(ObtainSemaphore,Exec)
#if !UseRegisterArgs
	addq.w	#8,%sp
#endif
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(ReleaseSemaphore,Exec)
	.type	AROS_SLIB_ENTRY(ReleaseSemaphore,Exec),@function
AROS_SLIB_ENTRY(ReleaseSemaphore,Exec):
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
#if !UseRegisterArgs
	move.l	24(%sp),-(%sp)
	move.l	24(%sp),-(%sp)
#endif
	jbsr	AROS_CSYMNAME(ReleaseSemaphore,Exec)
#if !UseRegisterArgs
	addq.w	#8,%sp
#endif
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec)
	.type	AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec),@function
AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec):
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
#if !UseRegisterArgs
	move.l	24(%sp),-(%sp)
	move.l	24(%sp),-(%sp)
#endif
	jbsr	AROS_CSYMNAME(ObtainSemaphoreShared,Exec)
#if !UseRegisterArgs
	addq.w	#8,%sp
#endif
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_CDEFNAME(os_disable)
	.type	AROS_CDEFNAME(os_disable),@function
AROS_CDEFNAME(os_disable):
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	jbsr	AROS_CSYMNAME(_os_disable)
	movem.l (%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl  AROS_CDEFNAME(os_enable)
	.type	AROS_CDEFNAME(os_enable),@function
AROS_CDEFNAME(os_enable):
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	jbsr	AROS_CSYMNAME(_os_enable)
	movem.l (%sp)+,%d0-%d1/%a0-%a1
	rts
