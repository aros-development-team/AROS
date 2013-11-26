/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 * 
 * $Id$
 */

#include <aros/config.h>

	.text
	.chip 68000
	.align 4 

	.globl MemoryTest
MemoryTest:
	movem.l	%d2-%d5/%a2-%a3/%a5,%sp@-
	move.w	0xdff01c,%sp@-	/* save INTENA */
	subq.l	#8,%sp
	move.l	%sp,%a5
	bsr.s	__MemoryTest
	addq.l	#8,%sp
	move.w	%sp@+,%d1
	or.w	#0x8000,%d1
	move.w	%d1,0xdff09a	/* restore INTENA */
	movem.l	%sp@+,%d2-%d5/%a2-%a3/%a5
	rts

	/******** Memory tester *******/
	 * IN: A0 - Address, A1 - Max end address, A5 - outtable
	 * IN: D0 = block size
	 * OUT: D0 - Detected size
	 * TRASH: D1, D2, D3, D4, D5, A1, A2, A3
	 */
	.globl __MemoryTest
__MemoryTest:
	move.l	%d0,%d5
	move.l	%a0,%a5@			/* write start address */
	move.l	%a1,%d0
	sub.l	%a0,%d0				/* max size */

	move.l	%a0,%d1
	and.l	#0xff000000,%d1
	beq.s	__MemoryTest_24bitaddr
	/* test if 32bit address mirrors address zero */
	move.l	%d1,%a1
	move.l	0.w,%d2				/* save old */
	move.l	0x100.w,%d3
	move.l	#0xfecaf00d,%d1
	move.l	%d1,0.w
	nop
	not.w	%d1
	move.l	%d1,0x100.w			/* write something else, some bus types "remember" old value */
	not.w	%d1
	nop							/* force 68040/060 bus cycle to finish */
	cmp.l	%a1@,%d1
	bne.s	__MemoryTest_32bitok	/* different? no mirror */
	move.l	#0xcafed00d,%d1
	move.l	%d1,0.w
	nop
	not.w	%d1
	move.l	%d1,0x100.w
	not.w	%d1
	nop
	cmp.l	%a1@,%d1
	bne.s	__MemoryTest_32bitok	/* check again, maybe 0 already had our test value */
	move.l	%d2,0.w				/* restore saved value */
	move.l	%d3,0x100.w
	moveq	#-1,%d1
	bra		__MemoryTest_none		/* 24-bit CPU, do not test this range */
__MemoryTest_32bitok:
	move.l	%d2,0.w				/* restore saved value */
	move.l	%d3,0x100.w
__MemoryTest_24bitaddr:

	/* a0 = tested address, d0 = max size, d1 = current size */

	clr.l	%d1
__MemoryTest_loop:
	cmp.l	%d0,%d1
	bge		__MemoryTest_done

	move.l	%a0,%d2
	and.l	#0xff000000,%d2
	bne.s	__MemoryTest_chipcheck_done	/* no chiptest if 32bit address */
	move.w	#0x7fff,0xdff09a
	nop
	tst.w	%a0@(0x1c,%d1)				/* If non-zero, this is not INTENAR */
	bne.s	__MemoryTest_chipcheck_done
	/* It was zero ... */
	move.w	#0xc000,0xdff09a			/* Try the master enable */
	nop
	tst.w	%a0@(0x1c,%d1)				/* If still zero, not INTENAR */
	bne 	__MemoryTest_done				/* It was a custom chip. */
__MemoryTest_chipcheck_done:

	move.l	%a0,%a2
	add.l	%d1,%a2
	cmp.l	#__MemoryTest,%a2		/* Make sure we don't modify our own test code */
	bcs.s	__MemoryTest_nottestcode
	cmp.l	#__MemoryTest_end,%a2
	bcs.s	__MemoryTest_next
__MemoryTest_nottestcode:

	move.l	%a0@(%d1),%d3		/* read old value */
	move.l	%a0@,%a2			/* save mirror test contents */
	move.l	#0xfecaf00d,%a0@	/* write mirror test value */
	nop
	move.l	#0xcafed00d,%d2
	move.l	%d2,%a0@(%d1)		/* write test pattern */
	nop
	tst.l	%d1					/* first test addrress? */
	beq.s	__MemoryTest_nomirror
	cmp.l	%a0@,%d2			/* no, check mirrorirng */
	bne.s	__MemoryTest_nomirror
	move.l	%a2,%a0@			/* restore mirror test contents */
	bra.s	__MemoryTest_done
__MemoryTest_nomirror:

	not.l	%d2
	move.l	%a0@(4,%d1),%a3		/* read temp address */
	move.l	%d2,%a0@(4,%d1)		/* fill bus with something else */
	not.l	%d2
	nop
	move.l	%a0@(%d1),%d4		/* read test pattern */
	move.l	%a3,%a0@(4,%d1)		/* restore */

	cmp.l	%d4,%d2				/* pattern match? */
	bne.s	__MemoryTest_done
	neg.l	%d2					/* test pattern 2 */

	move.l	%d2,%a0@(%d1)		/* write test pattern */
	nop
	not.l	%d2
	move.l	%a0@(4,%d1),%a3		/* read temp address */
	move.l	%d2,%a0@(4,%d1)		/* fill bus with something else */
	not.l	%d2
	nop
	move.l	%a0@(%d1),%d4		/* read test pattern */
	move.l	%a3,%a0@(4,%d1)		/* restore */

	cmp.l	%d4,%d2
	bne.s	__MemoryTest_done
	not.l	%d2
	move.l	%d3,%a0@(%d1)		/* write old value back */

	move.l	%a2,%a0@			/* restore mirror test contents */
__MemoryTest_next:
	add.l	%d5,%d1				/* next block */
	bra		__MemoryTest_loop

__MemoryTest_done:
	tst.l	%d1					/* nothing found? */
	beq.s	__MemoryTest_none
	addq.l	#4,%a5
	move.l	%d1,%a5@+
__MemoryTest_none:
	move.l	%d1,%d0
	rts
__MemoryTest_end:


