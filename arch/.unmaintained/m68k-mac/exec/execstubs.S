/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: glue functions
    Lang: english
*/

#include "aros/m68k/asm.h"

/*
    The following functions are guaranteed to preserve
    all registers. But I don't want to write them completely
    in assembly - C is generally more readable.
    So I use those stubs to preserve the registers.
*/
#ifdef UseExecstubs

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(ObtainSemaphore,Exec,94)
AROS_SLIB_ENTRY(ObtainSemaphore,Exec,94):
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

	.globl	AROS_SLIB_ENTRY(ReleaseSemaphore,Exec,95)
AROS_SLIB_ENTRY(ReleaseSemaphore,Exec,95):
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

	.globl	AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec,113)
AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec,113):
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

        .globl	AROS_SLIB_ENTRY(Disable,Exec,20)
     
AROS_SLIB_ENTRY(Disable,Exec,20):
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

	.globl	AROS_SLIB_ENTRY(Enable,Exec,21)
AROS_SLIB_ENTRY(Enable,Exec,21):
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

	.globl	AROS_SLIB_ENTRY(Forbid,Exec,22)
AROS_SLIB_ENTRY(Forbid,Exec,22):
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

	.globl	AROS_SLIB_ENTRY(Permit,Exec,23)
AROS_SLIB_ENTRY(Permit,Exec,23):
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
