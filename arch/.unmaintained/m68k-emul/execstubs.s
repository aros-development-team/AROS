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
#else
	move.l	%a6,-(%sp)
	move.l	%a0,-(%sp)
#endif
	jbsr	AROS_CSYMNAME(_ObtainSemaphore)
	addq.w	#8,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(ReleaseSemaphore,Exec)
	.type	AROS_SLIB_ENTRY(ReleaseSemaphore,Exec),@function
AROS_SLIB_ENTRY(ReleaseSemaphore,Exec):
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
#if !UseRegisterArgs
	move.l	24(%sp),-(%sp)
	move.l	24(%sp),-(%sp)
#else
	move.l	%a6,-(%sp)
	move.l	%a0,-(%sp)
#endif
	jbsr	AROS_CSYMNAME(_ReleaseSemaphore)
	addq.w	#8,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec)
	.type	AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec),@function
AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec):
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
#if !UseRegisterArgs
	move.l	24(%sp),-(%sp)
	move.l	24(%sp),-(%sp)
#else
	move.l	%a6,-(%sp)
	move.l	%a0,-(%sp)
#endif
	jbsr	AROS_CSYMNAME(_ObtainSemaphoreShared)
	addq.w	#8,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

        .globl	AROS_SLIB_ENTRY(Disable,Exec)
        .type	AROS_SLIB_ENTRY(Disable,Exec),@function
AROS_SLIB_ENTRY(Disable,Exec):
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
#if !UseRegisterArgs
	move.l	24(%sp),-(%sp)
#else
	move.l	%a6,-(%sp)
#endif
	jbsr	AROS_CSYMNAME(_Disable)
	addq.w	#4,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(Enable,Exec)
	.type	AROS_SLIB_ENTRY(Enable,Exec),@function
AROS_SLIB_ENTRY(Enable,Exec):
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
#if !UseRegisterArgs
	move.l	24(%sp),-(%sp)
#else
	move.l	%a6,-(%sp)
#endif
	jbsr	AROS_CSYMNAME(_Enable)
	addq.w	#4,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(Forbid,Exec)
	.type	AROS_SLIB_ENTRY(Forbid,Exec),@function
AROS_SLIB_ENTRY(Forbid,Exec):
	movem.l %d0-%d1/%a0-%a1,-(%sp)
#if !UseRegisterArgs
	move.l	24(%sp),-(%sp)
#else
	move.l	%a6,-(%sp)
#endif
	jbsr	AROS_CSYMNAME(_Forbid)
	addq.w	#4,%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(Permit,Exec)
	.type	AROS_SLIB_ENTRY(Permit,Exec),@function
AROS_SLIB_ENTRY(Permit,Exec):
	movem.l %d0-%d1/%a0-%a1,-(%sp)
#if !UseRegisterArgs
	move.l	24(%sp),-(%sp)
#else
	move.l	%a6,-(%sp)
#endif
	jbsr	AROS_CSYMNAME(_Permit)
	addq.w	#4,%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
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
