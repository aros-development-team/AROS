/*
    (C) 1995-97 AROS - The Amiga Replacement OS
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
	tst.l	ThisTask(%a6)
	jbeq	.nosave
	movea.l	sc(%sp),%a0
	movea.l	ThisTask(%a6),%a1
	move.l  sc_usp(%a0),tc_SPReg(%a1)
	movea.l	tc_UnionETask(%a1),%a1
	movea.l	iet_Context(%a1),%a1
	/* Save all registers in
	   SysBase->ThisTask->tc_UnionETask->iet_Context */
	move.l	sc_d0(%a0),(%a1)+
	move.l	sc_d1(%a0),(%a1)+
	move.l	sc_a0(%a0),(%a1)+
	move.l	sc_a1(%a0),(%a1)+
	move.w	sc_sr(%a0),(%a1)+
	addq.w	#2,%a1
	move.w	sc_formatvec(%a0),(%a1)+
	addq.w	#2,%a1
	move.l	%d2,(%a1)+
	move.l	%d3,(%a1)+
	move.l	%d4,(%a1)+
	move.l	%d5,(%a1)+
	move.l	%d6,(%a1)+
	move.l	%d7,(%a1)+
	move.l	%a2,(%a1)+
	move.l	%a3,(%a1)+
	move.l	%a4,(%a1)+
	movem.l	(%sp)+,%a5-%a6
	move.l	%a5,(%a1)+
	move.l	%a6,(%a1)+
	move.l	sc_pc(%a0),(%a1)+
	movem.l	%a5-%a6,-(%sp)
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
	movea.l	iet_Context(%a1),%a1
	movea.l	sc(%sp),%a0
	move.l	(%a1)+,sc_d0(%a0)
	move.l	(%a1)+,sc_d1(%a0)
	move.l	(%a1)+,sc_a0(%a0)
	move.l	(%a1)+,sc_a1(%a0)
	move.w	(%a1)+,sc_sr(%a0)
	addq.w	#2,%a1
	move.w	(%a1)+,sc_formatvec(%a0)
	addq.w	#2,%a1
	move.l	(%a1)+,%d2
	move.l	(%a1)+,%d3
	move.l	(%a1)+,%d4
	move.l	(%a1)+,%d5
	move.l	(%a1)+,%d6
	move.l	(%a1)+,%d7
	movea.l	(%a1)+,%a2
	movea.l	(%a1)+,%a3
	movea.l	(%a1)+,%a4
	move.l	(%a1)+,%a5
	move.l	(%a1)+,%a6
	addq.w	#8,%sp
	movem.l	%a5-%a6,-(%sp)
	move.l	(%a1)+,sc_pc(%a0)
#ifdef __PIC__
	lea	(%pc,_GLOBAL_OFFSET_TABLE_@GOTPC),%a5
	movea.l	SysBase@GOT(%a5),%a1
	movea.l	(%a1),%a6
#else
	movea.l	SysBase,%a6
#endif
	move.l	ThisTask(%a6),%a1
	move.l	tc_SPReg(%a1),sc_usp(%a0)
	/* Are interrupts enabled or disable for this task? */
	tst.b	IDNestCnt(%a6)
	jblt	.unmask
	moveq.l	#-1,%d0
	move.l	%d0,sc_mask(%a0)
	jbra	.except
.unmask:
	clr.l	sc_mask(%a0)
.except:
	/* Is there an exception to be processed? */
#ifndef TEST
	btst	#TB_EXCEPT,tc_Flags(%a1)
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
