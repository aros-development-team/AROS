
	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	buserror030
buserror030:
	/* 68030 bus error handling is so simple.. */
	or.w	#0x0700,%sr
	subq.l	#4,%sp
	movem.l	%d0/%a0,-(%sp)
	cmp.l	#4,12+0x10(%sp)
	bne		.doexit
	/* SSW */
	move.w	12+0x0a(%sp),%d0
	and.w	#0x01f0,%d0
	/* DATA (DF) + READ + LONG? */
	cmp.w	#0x0140,%d0
	bne		.doexit
	/* clear DF */
	and.w	#~0x0100,12+0x0a(%sp)
	movec	%vbr,%a0
	/* Fetch execbase from VBR */
	move.l	4(%a0),12+0x2c(%sp)
	/* 68030 continues the execution using above fetched value */
	movem.l	(%sp)+,%d0/%a0
	addq.l	#4,%sp
	rte

	.text
	.balign 4
	.globl	buserror040
buserror040:
	or.w	#0x0700,%sr
	subq.l	#4,%sp
	movem.l	%d0/%a0,-(%sp)
	/* fault address */
	cmp.l	#4,12+0x14(%sp)
	bne.s	.doexit
	/* SSW */
	move.w	12+0x0c(%sp),%d0
	and.w	#0xffc0,%d0
	/* LONG DATA READ + ATC fault? */
	cmp.w	#0x0500,%d0
	bne.s	.doexit
.dotrace:
	/* It was long read from address 4. Special handling required.
	 *
	 * Because 68040 (and 060) restart instruction after read fault,
	 * we need to mark zeropage as valid and re-run faulting
	 * instruction in trace mode. Trace exception is used to restore
	 * zero page descriptor back to original invalid state.
	 */
	move.l	zeropagedescriptor,%a0
	/* Zero page -> resident */
	or.b	#3,%a0@
	cpushl	%dc,(%a0)
	pflushan
	movec	%vbr,%a0
	/* Store old trace vector in "safe" place */
	move.l	9*4(%a0),254*4(%a0)
	/* Store old SR */
	move.w	12(%sp),255*4(%a0)
	/* Set temporary trace */
	move.l	#trace040,9*4(%a0)
	movem.l	(%sp)+,%d0/%a0
	addq.l	#4,%sp
	/* Modify SR in stack, trace on, interrupts disabled! */
	and.w	#0x3fff,(%sp)
	or.w	#0x8700,(%sp)
	/* Re-run the instruction */
	rte

.doexit:
	/* jump to original exception vector
	 * we assume address error uses same address
	 *
	 * TODO: Handle 68040 writebacks. Not really important at this
	 * point because this is meant to be kernel debugger that simply
	 * crashes the system if access fault happens.
	 */
	movec	%vbr,%a0
	move.l	3*4(%a0),8(%sp)
	movem.l	(%sp)+,%d0/%a0
	rts

trace040:
	move.l	%a0,-(%sp)
	move.l	zeropagedescriptor,%a0
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

	.text
	.balign 4
	.globl	buserror060
buserror060:
	or.w	#0x0700,%sr
	subq.l	#4,%sp
	movem.l	%d0/%a0,-(%sp)
	/* fault address */
	cmp.l	#4,12+0x08(%sp)
	bne		.doexit
	/* FSLW */
	move.l	12+0x0c(%sp),%d0
	and.l	#0x03e08200,%d0
	/* LONG DATA READ PAGE FAULT? */
	cmp.l	#0x01408200,%d0
	bne		.doexit
	bra		.dotrace