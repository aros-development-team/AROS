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
#ifdef UseExecstubs

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(ObtainSemaphore,Exec)
	.type	AROS_SLIB_ENTRY(ObtainSemaphore,Exec),@function
AROS_SLIB_ENTRY(ObtainSemaphore,Exec):
	jbra	AmigaSWInitObtainSemaphore
#ifdef __PIC__
	bra.l	AROS_CSYMNAME(_Exec_ObtainSemaphore)@PLTPC
#else
	jbra	AROS_CSYMNAME(_Exec_ObtainSemaphore)	
#endif
AmigaSWInitObtainSemaphore:
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	move.l	%a6,-(%sp)
	move.l	%a0,-(%sp)
#ifdef __PIC__
	bsr.l	AROS_CSYMNAME(_Exec_ObtainSemaphore)@PLTPC
#else
	jbsr	AROS_CSYMNAME(_Exec_ObtainSemaphore)
#endif
	addq.w	#8,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(ReleaseSemaphore,Exec)
	.type	AROS_SLIB_ENTRY(ReleaseSemaphore,Exec),@function
AROS_SLIB_ENTRY(ReleaseSemaphore,Exec):
	jbra	AmigaSWInitReleaseSemaphore
#ifdef __PIC__
	bra.l	AROS_CSYMNAME(_Exec_ReleaseSemaphore)@PLTPC
#else
	jbra	AROS_CSYMNAME(_Exec_ReleaseSemaphore)	
#endif
AmigaSWInitReleaseSemaphore:
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	move.l	%a6,-(%sp)
	move.l	%a0,-(%sp)
#ifdef __PIC__
	bsr.l	AROS_CSYMNAME(_Exec_ReleaseSemaphore)@PLTPC
#else
	jbsr	AROS_CSYMNAME(_Exec_ReleaseSemaphore)
#endif
	addq.w	#8,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec)
	.type	AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec),@function
AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec):
	jbra	AmigaSWInitObtainSemaphoreShared
#ifdef __PIC__
	bra.l	AROS_CSYMNAME(_Exec_ObtainSemaphoreShared)@PLTPC
#else
	jbra	AROS_CSYMNAME(_Exec_ObtainSemaphoreShared)	
#endif
AmigaSWInitObtainSemaphoreShared:
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	move.l	%a6,-(%sp)
	move.l	%a0,-(%sp)
#ifdef __PIC__
	bsr.l	AROS_CSYMNAME(_Exec_ObtainSemaphoreShared)@PLTPC
#else
	jbsr	AROS_CSYMNAME(_Exec_ObtainSemaphoreShared)
#endif
	addq.w	#8,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

        .globl	AROS_SLIB_ENTRY(Disable,Exec)
        .type	AROS_SLIB_ENTRY(Disable,Exec),@function
AROS_SLIB_ENTRY(Disable,Exec):
	jbra	AmigaSWInitDisable
#ifdef __PIC__
	bra.l	AROS_CSYMNAME(_Exec_Disable)@PLTPC
#else
	jbra	AROS_CSYMNAME(_Exec_Disable)	
#endif
AmigaSWInitDisable:
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	move.l	%a6,-(%sp)
#ifdef __PIC__
	bsr.l	AROS_CSYMNAME(_Exec_Disable)@PLTPC
#else
	jbsr	AROS_CSYMNAME(_Exec_Disable)
#endif
	addq.w	#4,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(Enable,Exec)
	.type	AROS_SLIB_ENTRY(Enable,Exec),@function
AROS_SLIB_ENTRY(Enable,Exec):
	jbra	AmigaSWInitEnable
#ifdef __PIC__
	bra.l	AROS_CSYMNAME(_Exec_Enable)@PLTPC
#else
	jbra	AROS_CSYMNAME(_Exec_Enable)	
#endif
AmigaSWInitEnable:
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	move.l	%a6,-(%sp)
#ifdef __PIC__
	bsr.l	AROS_CSYMNAME(_Exec_Enable)@PLTPC
#else
	jbsr	AROS_CSYMNAME(_Exec_Enable)
#endif
	addq.w	#4,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(Forbid,Exec)
	.type	AROS_SLIB_ENTRY(Forbid,Exec),@function
AROS_SLIB_ENTRY(Forbid,Exec):
	jbra	AmigaSWInitForbid
#ifdef __PIC__
	bra.l	AROS_CSYMNAME(_Exec_Forbid)@PLTPC
#else
	jbra	AROS_CSYMNAME(_Exec_Forbid)	
#endif
AmigaSWInitForbid:
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	move.l	%a6,-(%sp)
#ifdef __PIC__
	bsr.l	AROS_CSYMNAME(_Exec_Forbid)@PLTPC
#else
	jbsr	AROS_CSYMNAME(_Exec_Forbid)
#endif
	addq.w	#4,%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	AROS_SLIB_ENTRY(Permit,Exec)
	.type	AROS_SLIB_ENTRY(Permit,Exec),@function
AROS_SLIB_ENTRY(Permit,Exec):
	jbra	AmigaSWInitPermit
#ifdef __PIC__
	bra.l	AROS_CSYMNAME(_Exec_Permit)@PLTPC
#else
	jbra	AROS_CSYMNAME(_Exec_Permit)	
#endif
AmigaSWInitPermit:
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	move.l	%a6,-(%sp)
#ifdef __PIC__
	bsr.l	AROS_CSYMNAME(_Exec_Permit)@PLTPC
#else
	jbsr	AROS_CSYMNAME(_Exec_Permit)
#endif
	addq.w	#4,%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
	rts


#endif
