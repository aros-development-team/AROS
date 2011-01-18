	.text

	.align 4 

	reset
	.globl rom_init
	nop
rom_init:
	/* In ROM, so disable the overlay */
	moveq	#0x3, %d0
	move.b	%d0, 0xbfe201
	moveq	#0x2, %d0
	move.b	%d0, 0xbfe001

	/* Wait a bit for reset to finish */
	move.l	#0x100000,%d0
0:	subq.l	#1,%d0
	bge.s	0b

	/* Disable interrupts and DMA */
	lea.l	0xdff000, %a0
	move.w	#0x7fff, %d0
	move.w	%d0,0x9a(%a0)	/* Interrupt disable */
	move.w	%d0,0x9c(%a0)	/* Clear interrupts */
	move.w	%d0,0x96(%a0)	/* Stop DMA */

	/* Check for cartridge ROM */
	lea	0xf00000,%a0
	move.w	%a0@(0),%d0	/* Get the ROM ID (0x1111) */
	cmp.w	#0x1111,%d0
	bne.s	0f		/* Must be 0x1111 */
	lea %pc@(0),%a1
	move.l %a1,%d0
	clr.w %d0
	cmp.l %d0,%a0
	beq.s 0f /* skip if we booted at 0xF00000 */
	lea	0x40000,%sp	/* Safe stack at Chip Mem 128K */
	lea	0f,%a5
	jmp	%a0@(2)
0:

	/* Do we have a Gayle? */
	lea		1f(%pc),%a7
	jmp		Exec_ReadGayle
1:	move.l  %d0,%d6         /* Save for later - %d6 is Gayle ID */

	/* reserve space for memory region array */
	lea		_bss,%a4
	sub.w	#64,%a4
	move.l	%a4,%a5
	/* temp supervisor stack */
	move.l	%a4,%a0
	move.l	%a0,%sp
	sub.w	#0x400,%a0
	/* temp user stack */
	move.l	%a0,%usp

	/* check non-autoconfig memory areas
	 * in order of fastest to slowest */	

	/* cpu slot ram (A3000/A4000) */
	move.l	#0x08000000,%a0
	move.l	#0x7f000000,%a1
	move.l	#0x00100000,%d0
	bsr		.L_memtest

	/* mainboard ram (A3000) */
	/* end is static 0x08000000, start addr is end - (size of ram) */
	move.l	%a5,%a3
	move.l	#-0x00100000,%d7
.a3000test:
	move.l	#0x08000000,%a1
	move.l	#0x00100000,%d0
	add.l	%d0,%d7
	move.l	%a1,%a0
	sub.l	%d0,%a0
	sub.l	%d7,%a0
	sub.l	%d7,%a1
	move.l	%a3,%a5
	lsr.l	#1,%d0
	bsr		.L_memtest
	bne.s	.a3000test
	move.l	%a3,%a5
	tst.l	%d7
	beq.s	.a3000noram
	move.l	#0x08000000,%a1
	sub.l	%d7,%a1
	move.l	%a1,%a5@+
	move.l	%d7,%a5@+
.a3000noram:

	/* slow ram */
	move.l	#0x00c00000,%a0
	move.l	#0x00d80000,%a1
	tst.b	%d6
	bne.s	0f
	move.l	#0x00dc0000,%a1
0:	move.l	#0x00080000,%d0
	bsr		.L_memtest
	
	/* chip ram */
	sub.l	%a0,%a0
	move.l	#0x00200000,%a1
	move.l	#0x00040000,%d0
	bsr		.L_memtest
	beq.s	chipfault
	/* adjust bss */
	move.l	#_bss_end,%a5@(-8)
	sub.l	#_bss_end,%a5@(-4)

	/* end of memory region list */
	clr.l	%a5@+
	clr.l	%a5@

	move.w	#0,%sr		/* Go to user mode */

	move.l	%sp,%sp@-
	move.l	#_bss,%sp@-
	move.l	%a4,%sp@-	/* memory bank array */
	jsr		start

chipfault:
	move.w	#0x0f0,0xdff180
looping:
	nop
	bra.s	looping

MemoryTest:
	movem.l	%d2-%d7/%a2-%a6,%sp@-
	subq.l	#8,%sp
	move.l	%sp,%a5
	bsr.s	.L_memtest
	addq.l	#8,%sp
	movem.l	%sp@+,%d2-%d7/%a2-%a6
	rts

	/******** Memory tester *******/
	 * IN: A0 - Address, A1 - Max end address, A5 - outtable
	 * IN: D0 = block size
	 * OUT: D0 - Detected size
	 * TRASH: D1, D2, D3, D4, D5, A1, A2
	 */
.L_memtest:
	move.l	%d0,%d5
	move.l	%a0,%a5@
	move.l	%a1,%d0
	sub.l	%a0,%d0

	move.l	%a0,%d1
	and.l	#0xff000000,%d1
	beq		.L_memtest_24bitaddr
	/* test if 32bit address mirrors address zero */
	move.l	%d1,%a1
	move.l	0.w,%d2				/* save old */
	move.l	#0xfecaf00d,%d1
	move.l	%d1,0.w
	nop		/* force 68040/060 bus cycle to finish */
	cmp.l	%a1@,%d1
	bne		.L_memtest_32bitok	/* different? no mirror */
	move.l	#0xcafed00d,%d1
	move.l	%d1,0.w
	nop
	cmp.l	%a1@,%d1
	bne		.L_memtest_32bitok	/* check again, maybe 0 already had our test value */
	move.l	%d2,0.w				/* restore saved value */
	moveq	#0,%d0
	bra		.L_memtest_none		/* 24-bit CPU, do not test this range */
.L_memtest_32bitok:
	move.l	%d2,0.w				/* restore saved value */
.L_memtest_24bitaddr:

	move.l	%a0@,%a2

	move.l	#0xfecaf00d,%a0@			/* mirror test value */
	clr.l	%d1

	move.l	%a1,%d2
	and.l	#0xff000000,%d2
	beq.s	.L_memtest_chipcheck	/* no chiptest if 32bit address */
	move.l	%d0,%d1
	bra.s	.L_memtest_chipcheck_done
.L_memtest_chipcheck:					/* Can we turn off interrupts? */
	add.l	%d5,%d1
	cmp.l	%d0,%d1
	bgt.s	.L_memtest_chipcheck_done
	lea.l	%a0@(%d1),%a1
	move.w	#0x7fff,0xdff09a
	tst.w	%a1@(-(0x40000 - 0x3f01c))	/* If non-zero, this is not INTENAR */
	bne.s	.L_memtest_chipcheck
	/* It was zero ... */
	move.w	#0xc000,0xdff09a			/* Try the master enable */
	tst.w	%a1@(-(0x40000 - 0x3f01c))	/* If still zero, not INTENAR */
	beq.s	.L_memtest_chipcheck
	/* It was a custom chip. */
.L_memtest_chipcheck_done:

	move.l	%d1,%d0
	sub.l	%d5,%d0

	/* TODO: Better pattern checks */
	clr.l	%d1
	move.l	#0xcafed00d,%d2
.L_memtest_pattern:
	cmp.l	%d0,%d1
	bge.s	.L_memtest_done_end
	move.l	%a0@(%d1),%d3
	move.l	%d2,%a0@(%d1)
	nop
	tst.l	%d1
	beq.s	.L_memtest_nomirror
	cmp.l	%a0@,%d2				/* mirrored? */
	bne.s	.L_memtest_nomirror
	move.l	%d3,%a0@(%d1)
	bra.s	.L_memtest_done
.L_memtest_nomirror:
	move.l	%a0@(%d1),%d4
	move.l	%d3,%a0@(%d1)
	nop
	cmp.l	%d4,%d2
	bne.s	.L_memtest_done
	add.l	%d5,%d1
	bra.s	.L_memtest_pattern
.L_memtest_done_end:
	move.l	%d0,%d1
.L_memtest_done:
	move.l	%a2,%a0@
	tst.l	%d1
	beq.s	.L_memtest_none
	addq.l	#4,%a5
	move.l	%d1,%a5@+
.L_memtest_none:
	move.l	%d1,%d0
	rts

