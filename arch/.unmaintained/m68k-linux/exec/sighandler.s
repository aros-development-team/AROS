/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Signal handler for Linux/m68k
    Lang: english
*/

	#include "machine.i"

#ifdef TEST
#  undef Dispatch
#endif

#define FirstArg	4+(2*4)
#define signum		FirstArg+0
#define code		FirstArg+4
#define sc		FirstArg+8

	.text
	.align	2
	.globl	AROS_CDEFNAME(linux_sighandler)
	.type	AROS_CDEFNAME(linux_sighandler),@function
AROS_CDEFNAME(linux_sighandler):
	/* save regs */
	movem.l	%a5-%a6,-(%sp)
#ifdef __PIC__
	lea	(%pc,_GLOBAL_OFFSET_TABLE_@GOTPC),%a5
	movea.l	supervisor@GOT(%a5),%a0
#else
	lea	supervisor,%a0
#endif
#ifndef TEST
	/* Already in signal handler... Not good */
	tst.l	(%a0)
	jbne	.exit
#endif
	/* Increase supervisor count */
	addq.l	#1,(%a0)
	/* Call the Amiga interrupt server for this signal */
	move.l	signum(%sp),-(%sp)
#ifdef __PIC__
	bsr.l	AROS_CSYMNAME(call_inthandlers)@PLTPC
#else
	jbsr	AROS_CSYMNAME(call_inthandlers)
#endif
	addq.w	#4,%sp
#ifdef __PIC__
	movea.l	SysBase@GOT(%a5),%a1
        movea.l	(%a1),%a6
#else
	movea.l	SysBase,%a6
#endif
#ifdef TEST
	tst.l	TDNestCnt(%a6)
	jbge	.exitsup
#else
	/* Dispatch pending? */
	btst	#7,AttnResched(%a6)
	jbeq	.exitsup
	/* Clear flag */
	bclr	#7,AttnResched(%a6)
#endif
	/* If there's no current task, we don't need to save
	   the registers */
	movea.l	ThisTask(%a6),%a1
	jbeq	.nosave
	/* Don's save the registers if the task was removed,
	   it will never get to run again. */
	cmp.b	#TS_REMOVED,tc_State(%a1)
	jbeq	.nosave
	movea.l	sc(%sp),%a0
	move.l  sc_usp(%a0),tc_SPReg(%a1)
	movea.l	tc_UnionETask(%a1),%a1
	movea.l	iet_Context(%a1),%a1
	/* Save all registers in
	   SysBase->ThisTask->tc_UnionETask->iet_Context */
#if !UseRegisterArgs
	pea	sc_size.w
#else
	move.l	#sc_size,%d0
#endif
	move.l	%a1,-(%sp)
	move.l	%a0,-(%sp)
	jsr	CopyMem(%a6)
	movea.l	(%sp)+,%a0
	movea.l	(%sp)+,%a1
#if !UseRegisterArgs
	addq.w	#4,%sp
#endif
	movem.l	(%sp)+,%a5-%a6
	movem.l	%d2-%d7/%a2-%a6,regs(%a1)
	movem.l	%a5-%a6,-(%sp)
	fmovem.x	%fp2-%fp7,fpregs(%a1)
#ifdef __PIC__
	lea	(%pc,_GLOBAL_OFFSET_TABLE_@GOTPC),%a5
	movea.l	SysBase@GOT(%a5),%a6
	movea.l	(%a6),%a6
#else
	movea.l	SysBase,%a6
#endif
.nosave:
	/* Call Dispatch() to do its work */
#ifdef TEST
#  ifdef __PIC__
	bsr.l	AROS_CSYMNAME(Dispatch)@PLTPC
#  else
	jbsr	AROS_CSYMNAME(Dispatch)
#  endif
#else
#  if !UseRegisterArgs
	move.l	%a6,-(%sp)
#  endif
	jsr	Dispatch(%a6)
#  if !UseRegisterArgs
	move.l	(%sp)+,%a6
#  endif
#endif
	/* We have switched tasks, restore the registers */
	movea.l	ThisTask(%a6),%a1
	movea.l	tc_UnionETask(%a1),%a1
	movea.l	iet_Context(%a1),%a0
	movea.l	sc(%sp),%a1
	/* Is this the first time? */ 
	tst.l	sc_usp(%a0)
	jbeq	.nocopy
#if !UseRegisterArgs
	pea	sc_size.w
#else
	move.l	#sc_size,%d0
#endif
	move.l	%a1,-(%sp)
	move.l	%a0,-(%sp)
	jsr	CopyMem(%a6)
	move.l	(%sp)+,%a0
	move.l	(%sp)+,%a1
#if !UseRegisterArgs
	addq.w	#4,%sp
#endif
	fmovem.x	fpregs(%a0),%fp2-%fp7
	jbra	.cont
.nocopy:
	move.l	sc_d0(%a0),sc_d0(%a1)
	move.l	sc_d1(%a0),sc_d1(%a1)
	move.l	sc_a0(%a0),sc_a0(%a1)
	move.l	sc_a1(%a0),sc_a1(%a1)
	move.l	sc_pc(%a0),sc_pc(%a1)
.cont:
	movem.l	regs(%a0),%d2-%d7/%a2-%a6
	addq.w	#8,%sp
	movem.l	%a5-%a6,-(%sp)
#ifdef __PIC__
	lea	(%pc,_GLOBAL_OFFSET_TABLE_@GOTPC),%a5
	movea.l	SysBase@GOT(%a5),%a0
	movea.l	(%a0),%a6
#else
	movea.l	SysBase,%a6
#endif
	move.l	ThisTask(%a6),%a0
	move.l	tc_SPReg(%a0),sc_usp(%a1)
	/* Are interrupts enabled or disable for this task? */
	tst.b	IDNestCnt(%a6)
	jblt	.unmask
	moveq.l	#-1,%d0
	move.l	%d0,sc_mask(%a1)
	jbra	.except
.unmask:
	clr.l	sc_mask(%a1)
.except:
	/* Is there an exception to be processed? */
#ifndef TEST
	btst	#TB_EXCEPT,tc_Flags(%a0)
	jbeq	.exitsup
#  if !UseRegisterArgs
	move.l  %a6,-(%sp)
#  endif
	jsr	Disable(%a6)
	jsr	Exception(%a6)
	jsr	Enable(%a6)
#  if !UseRegisterArgs
	move.l	(%sp)+,%a6
#  endif
#endif
.exitsup:
#ifndef TEST
#  ifdef __PIC__
	movea.l	supervisor@GOT(%a5),%a0
#  else
	lea	supervisor,%a0
#  endif
	/* Decrease supervisor nest count */
	subq.l	#1,(%a0)
#else
	bset	#15,SysFlags(%a6)
#endif
.exit:
	movem.l	(%sp)+,%a5-%a6
	rts
