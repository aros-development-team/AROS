
	#include <aros/config.h>
	#include "aros/m68k/asm.h"

#define STACKOFFSET (4 + 4*4)

	.text
	.balign 4
	.globl	buserror030
buserror030:
	/* 68030 bus error handling is so simple.. */
	or.w	#0x0700,%sr
	subq.l	#4,%sp
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	moveq	#0,%d0
	moveq	#0,%d1
	/* SSW */
	move.w	STACKOFFSET+0x0a(%sp),%d1
	/* fault address */
	move.l	STACKOFFSET+0x10(%sp),%a0
	cmp.w	#4,%a0
	bne		.doexit
	and.w	#0x01f0,%d1
	/* DATA (DF) + READ + LONG? */
	cmp.w	#0x0140,%d1
	bne		.doexit
	/* clear DF */
	and.w	#~0x0100,STACKOFFSET+0x0a(%sp)
	movec	%vbr,%a0
	/* Fetch execbase from VBR */
	move.l	4(%a0),STACKOFFSET+0x2c(%sp)
	/* 68030 continues the execution using above fetched value */
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	addq.l	#4,%sp
	rte

	.balign 4
	.globl	buserror040
buserror040:
	or.w	#0x0700,%sr
	subq.l	#4,%sp
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	moveq	#1,%d0
	/* SSW */
	moveq	#0,%d1
	move.w	STACKOFFSET+0x0c(%sp),%d1
	/* fault address */
	move.l	STACKOFFSET+0x14(%sp),%a0
	cmp.w	#4,%a0
	bne		.doexit
	and.w	#0xffc0,%d1
	/* LONG DATA READ + ATC fault? */
	cmp.w	#0x0500,%d1
	bne		.doexit
.dotrace:
	/* It was long read from address 4. Special handling required.
	 *
	 * Because 68040 (and 060) restart instruction after read fault,
	 * we need to mark zeropage as valid and re-run faulting
	 * instruction in trace mode. Trace exception is used to restore
	 * zero page descriptor back to original invalid state.
	 */
	movec	%vbr,%a0
	move.l	253*4(%a0),%a0
	/* Zero page -> resident */
	or.b	#3,%a0@
	cpushl	%dc,(%a0)
	pflushan
	movec	%vbr,%a0
	/* Store old trace vector in "safe" place */
	move.l	9*4(%a0),254*4(%a0)
	/* Store old SR */
	move.w	STACKOFFSET(%sp),255*4(%a0)
	/* Set temporary trace */
	move.l	#trace040,9*4(%a0)
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	addq.l	#4,%sp
	/* Modify SR in stack, trace on, interrupts disabled! */
	and.w	#0x3fff,(%sp)
	or.w	#0x8700,(%sp)
	/* Re-run the instruction */
	rte

	.balign 4
trace040:
	move.l	%a0,-(%sp)
	movec	%vbr,%a0
	move.l	253*4(%a0),%a0
	/* Zero page -> invalid */
	and.b	#~3,%a0@
	cpushl	%dc,(%a0)
	pflushan
	movec	%vbr,%a0
	/* Original trace vector */
	move.l	254*4(%a0),9*4(%a0)
	/* Original SR */
	move.w	255*4(%a0),4(%sp)
	move.l	(%sp)+,%a0
	rte

	.balign 4
	.globl	buserror060
buserror060:
	or.w	#0x0700,%sr
	subq.l	#4,%sp
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	moveq	#2,%d0
	/* FSLW */
	move.l	STACKOFFSET+0x0c(%sp),%d1
	/* fault address */
	move.l	STACKOFFSET+0x08(%sp),%a0
	cmp.w	#4,%a0
	bne		.doexit
	and.l	#0x07000200,%d1
	/* DATA READ PAGE FAULT? */
	cmp.l	#0x01000200,%d1
	beq		.dotrace
	
.doexit:

#define STACK_RESERVE 80

#if AROS_SERIAL_DEBUG
	sub.w	#STACK_RESERVE,%sp
	move.l	%sp@(STACK_RESERVE+0),%sp@(16+0) 		// stacked D0 -> regs[0]
	move.l	%sp@(STACK_RESERVE+4),%sp@(16+4)		// stacked D1 -> regs[1]
	move.l	%sp@(STACK_RESERVE+8),%sp@(16+8*4+0)	// stacked A0 -> regs[8]
	move.l	%sp@(STACK_RESERVE+12),%sp@(16+8*4+4)	// stacked A1 -> regs[9]
	move.l	%sp,%a0
	add.w	#STACK_RESERVE+STACKOFFSET,%a0
	move.l	%a0,%sp@								//  0 Bus error stack frame
	move.l	%sp,%a0
	addq.l	#4,%a0
	move.l	%d0,(%a0)+								//  4 CPU type
	movec	%usp,%d0
	move.l	%d0,(%a0)+								//  8 USP
	movec	%vbr,%d0
	move.l	%d0,(%a0)+								// 12 VBR
	move.l	%sp,%d0
	add.l	#STACK_RESERVE+STACKOFFSET,%d0
													// 16
	move.l	%d0,%a0@(8*4+7*4)						// SSP -> regs[15]
	movem.l	%d2-%d7,%a0@(8)							// regs[2]-regs[7]
	movem.l	%a2-%a6,%a0@(8*4+8)						// regs[10]-regs[14]
	move.l	%sp,-(%sp)
	jsr	bushandler
	add.w	#STACK_RESERVE+4,%sp
#endif

	/* jump to original exception vector
	 * we assume illegal instruction uses same address
	 *
	 * TODO: Handle 68040 writebacks. Not really important at this
	 * point because this is meant to be kernel debugger that simply
	 * crashes the system if access fault happens.
	 */

	movec	%vbr,%a0
	move.l	4*4(%a0),STACKOFFSET-4(%sp)
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

	.globl	addrerror030
addrerror030:
	or.w	#0x0700,%sr
	subq.l	#4,%sp
	move.w	#0x10,(%sp)
.addrerror:
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	move.w	STACKOFFSET-4(%sp),%d0
	bra		.doexit

	.globl	addrerror040
addrerror040:
	or.w	#0x0700,%sr
	subq.l	#4,%sp
	move.w	#0x11,(%sp)
	bra.s	.addrerror

	.globl	addrerror060
addrerror060:
	or.w	#0x0700,%sr
	subq.l	#4,%sp
	move.w	#0x12,(%sp)
	bra.s	.addrerror

	.chip 68030
	.globl	mmu_valid_check_030
mmu_valid_check_030:
	move.l	4(%sp),%a0
	ptestr	#5,(%a0),#7 // FC=5, all levels
	subq.l	#4,%sp
	pmove	%psr,(%sp)
	move.w	(%sp),%d0
	addq.l	#4,%sp
	and.w	#0x8400,%d0
	rts

	.chip 68040
	.globl	mmu_valid_check_040
mmu_valid_check_040:
	move.l	4(%sp),%a0
	movec	%dfc,%d1
	moveq	#5,%d0
	movec	%d0,%dfc
	ptestr	(%a0)
	movec	%mmusr,%d0
	movec	%d1,%dfc
	and.w	#0x0800,%d0
	rts

	.chip 68060
	.globl	mmu_valid_check_060
mmu_valid_check_060:
	move.l	%d2,-(%sp)
	movec	%dfc,%d2
	moveq	#5,%d0
	movec	%d0,%dfc
	movec	%vbr,%a0
	addq.l	#8,%a0
	lea		mmu_invalid_060(%pc),%a1
	move.l	(%a0),%d1
	move.l	%a1,(%a0)
	move.l	8(%sp),%a1
	moveq	#0,%d0
	plpar	(%a1)
	move.l	%d1,(%a0)
	movec	%d2,%dfc
	move.l	(%sp)+,%d2
	rts
mmu_invalid_060:
	moveq	#-1,%d0
	addq.l	#2,2(%sp)
	rte

