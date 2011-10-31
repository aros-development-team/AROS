
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
	/* SSW */
	move.w	STACKOFFSET+0x0a(%sp),%d0
	/* fault address */
	move.l	STACKOFFSET+0x10(%sp),%a0
	cmp.w	#4,%a0
	bne		.doexit
	move.w	%d0,%d1
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
	/* SSW */
	moveq	#0,%d0
	move.w	STACKOFFSET+0x0c(%sp),%d0
	/* fault address */
	move.l	STACKOFFSET+0x14(%sp),%a0
	cmp.w	#4,%a0
	bne		.doexit
	move.w	%d0,%d1
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
	move.l	SysBase,%a0
	move.l	%a0@(eb_KernelBase),%a0
	move.l	%a0@(kb_PlatformData),%a0
	move.l	%a0@(zeropagedescriptor),%a0
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

	.balign 4
	.globl	buserror060
buserror060:
	or.w	#0x0700,%sr
	subq.l	#4,%sp
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	/* FSLW */
	move.l	STACKOFFSET+0x0c(%sp),%d0
	/* fault address */
	move.l	STACKOFFSET+0x08(%sp),%a0
	cmp.w	#4,%a0
	bne		.doexit
	move.l	%d0,%d1
	and.l	#0x07000200,%d1
	/* DATA READ PAGE FAULT? */
	cmp.l	#0x01000200,%d1
	beq		.dotrace
	
.doexit:
	/* jump to original exception vector
	 * we assume address error uses same address
	 *
	 * TODO: Handle 68040 writebacks. Not really important at this
	 * point because this is meant to be kernel debugger that simply
	 * crashes the system if access fault happens.
	 *
	 * A0 = fault address, D0 = MMU status flags
	 * Following code MUST NOT access SysBase! */
	 */
#ifdef AROS_SERIAL_DEBUG
	
	move.l	%a0,-(%sp)
	
	move.l	%d0,-(%sp)
	pea		.berrstr1(%pc)
	jsr		DebugPutHex
	addq.l	#8,%sp

	pea		.berrstr2(%pc)
	jsr		DebugPutHex
	addq.l	#8,%sp
	
	move.l	STACKOFFSET+2(%sp),-(%sp)
	pea		.berrstr3(%pc)
	jsr		DebugPutHex
	addq.l	#8,%sp

	moveq	#0,%d0
	move.w	STACKOFFSET+0(%sp),%d0
	move.l	%d0,-(%sp)
	pea		.berrstr4(%pc)
	jsr		DebugPutHex
	addq.l	#8,%sp
	
	movec	%usp,%d0
	move.l	%d0,-(%sp)
	pea		.berrstr5(%pc)
	jsr		DebugPutHex
	addq.l	#8,%sp

	move.l	%a7,%d0
	add.l	#STACKOFFSET,%d0
	move.l	%d0,-(%sp)
	pea		.berrstr6(%pc)
	jsr		DebugPutHex
	addq.l	#8,%sp

	move.l	%d7,-(%sp)
	movem.l	%d0-%d7/%a0-%a6,-(%sp)
	move.l	15*4+4(%sp),(%sp)		// Original D0
	move.l	15*4+4+4(%sp),8*4(%sp)	// Original A0

	moveq	#0,%d7
0:	move.l	0(%sp,%d7.w*4),-(%sp)
	jsr		DebugPutHexVal
	addq.l	#4,%sp
	addq.w	#1,%d7
	cmp.w	#8,%d7
	bne.s	0b

	pea		.berrlf(%pc)
	jsr		DebugPutStr
	addq.l	#4,%sp

1:	move.l	0(%sp,%d7.w*4),-(%sp)
	jsr		DebugPutHexVal
	addq.l	#4,%sp
	addq.w	#1,%d7
	cmp.w	#15,%d7
	bne.s	1b

	pea		.berrlf(%pc)
	jsr		DebugPutStr
	addq.l	#4,%sp
	
	lea		15*4(%sp),%sp
	move.l	(%sp)+,%d7

#endif

	movec	%vbr,%a0
	move.l	3*4(%a0),STACKOFFSET-4(%sp)
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts

#ifdef AROS_SERIAL_DEBUG
.berrstr1:
	.string	"Bus error!\nStatus"
.berrstr2:
	.string "Fault address"
.berrstr3:
	.string "PC"
.berrstr4:
	.string "SR"
.berrstr5:
	.string "USP"
.berrstr6:
	.string "SSP"
.berrlf:
	.string "\n"
#endif
