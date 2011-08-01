
#include <aros/m68k/asm.h>
#include <aros/config.h>

	.text
	.chip 68000
	.align 4 

	.globl rom_init
rom_init:

	/* Wait a bit for reset to finish.
	 * Simulate ROM checksum check.
	 */
	lea		0x00f80000,%a0
	move.l	#524288/4,%d0
	moveq	#0,%d1
0:	add.l	(%a0)+,%d1
	bcc.s	1f
	addq.l	#1,%d1
1:	subq.l	#1,%d0
	bne.s	0b

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

	/* Disable the overlay, power led off. */
	moveq	#0,%d0
	move.b	%d0,0xbfe001
	moveq	#0x3,%d0
	move.b	%d0,0xbfe201

	/* Initial Custom chipset setup. Does same writes as AOS.
	 * It is possible some bus snooping hardware assumes this.
	 */
	lea.l	0xdff000,%a0
	move.w	#0x7fff,%d0
	move.w	%d0,0x9a(%a0)	/* Interrupt disable */
	move.w	%d0,0x9c(%a0)	/* Clear interrupts */
	move.w	%d0,0x96(%a0)	/* Stop DMA */
	move.w	#0x0174,0x032(%a0)	/* SERPER */
	move.w	#0x0200,0x100(%a0)	/* BPLCON0 */
	move.w	#0x0000,0x110(%a0)	/* BPL1DAT */
	move.w	#0x0111,0x180(%a0)	/* COLOR00 */

	/* temp system stack */
	lea.l	_ss_end,%sp		/* System stack */
	lea.l	_ss,%a5	/* Put the memory bank array at the bottom of the stack */

	move.l	%a5,%a4	/* Space for CPU/FPU data array */
	addq.l	#8,%a5
	move.l	%a4,%a0
	bsr		_cpu_detect
	move.l	%a4@,%d6

	/* Memory region array is put into the
	 * system stack from the bottom up, so
	 * we check the non-autoconfig memory areas
	 * in order of fastest to slowest, going
	 * up the system stack in A5.
	 */

	btst	#1,%d6		/* AttnFlags AFB_68020 */
	beq.s	.skip32bit

	/* cpu slot ram (A3000/A4000) */
	move.l	#0x08000000,%a0
	move.l	#0x7f000000,%a1
	move.l	#0x00100000,%d0
	bsr		.L_memtest
	bpl.s	.mbramtest

	/* We have 68EC020 if N-flag set and AttnFlags = AFB_68020
	 * but we ignore set N-flag if CPU is not 68020 because some
	 * accelerator boards mirror unmapped 32-bit address space
	 * to 24-bit space */
    move.w	%d6,%d0
    and.w	#0x8f,%d0
	/* CPU=68020? */
    cmp.w	#2,%d0
    bne.s	.skip32bit
	/* set AttnFlags AFB_ADDR32 */
	or.w	#0x4000,%a4@(2)
	bra.s	.skip32bit

.mbramtest:
	/* set AttnFlags AFB_ADDR32 */
	or.w	#0x4000,%a4@(2)

	/* mainboard ram (A3000) */
	/* end is static 0x08000000, start addr is end - (size of ram) */
	move.l	%a5,%a6
	move.l	#-0x00100000,%d7
.a3000test:
	move.l	#0x08000000,%a1
	move.l	#0x00100000,%d0
	add.l	%d0,%d7
	move.l	%a1,%a0
	sub.l	%d0,%a0
	sub.l	%d7,%a0
	sub.l	%d7,%a1
	move.l	%a6,%a5
	lsr.l	#1,%d0
	bsr		.L_memtest
	bmi.s	.a3000noram
	bne.s	.a3000test
	move.l	%a6,%a5
	tst.l	%d7
	beq.s	.a3000noram
	move.l	#0x08000000,%a1
	sub.l	%d7,%a1
	move.l	%a1,%a5@+
	move.l	%d7,%a5@+
.a3000noram:

.skip32bit:
	/* Do we have a Gayle? */
	sub.l	%a6,%a6
	jsr	AROS_SLIB_ENTRY(ReadGayle,Exec,136)
	/* slow ram */
	move.l	#0x00c00000,%a0
	move.l	#0x00d80000,%a1
	tst.b	%d0
	bne.s	0f
	move.l	#0x00dc0000,%a1
0:	move.l	#0x00040000,%d0
	bsr		.L_memtest
	
	/* chip ram */
	sub.l	%a0,%a0
	move.l	#0x00200000,%a1
	move.l	#0x00040000,%d0
	bsr		.L_memtest
	beq	chipfault

	/* Adjust to skip the first 4K bytes of
	 * Chip RAM. It's reserved for the Trap area.
	 */
	move.l	#0x1000,%a5@(-8)
	sub.l	#0x1000,%a5@(-4)

	/* end of memory region list */
	clr.l	%a5@+
	clr.l	%a5@

	lea.l	_ss,%a5
	move.l	%a5,%sp@-	/* CPU/FPU data array */
	addq.l	#8,%a5
	move.l	%a5,%sp@-	/* memory bank array */
	jsr		exec_boot

chipfault:
	move.w	#0x0f0,0xdff180
looping:
	nop
	bra.s	looping

	.globl MemoryTest
MemoryTest:
	movem.l	%d2-%d5/%a2-%a3/%a5,%sp@-
	move.w	0xdff01c,%sp@-	/* save INTENA */
	subq.l	#8,%sp
	move.l	%sp,%a5
	bsr.s	.L_memtest
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
.L_memtest:
	move.l	%d0,%d5
	move.l	%a0,%a5@			/* write start address */
	move.l	%a1,%d0
	sub.l	%a0,%d0				/* max size */

	move.l	%a0,%d1
	and.l	#0xff000000,%d1
	beq.s	.L_memtest_24bitaddr
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
	bne.s	.L_memtest_32bitok	/* different? no mirror */
	move.l	#0xcafed00d,%d1
	move.l	%d1,0.w
	nop
	not.w	%d1
	move.l	%d1,0x100.w
	not.w	%d1
	nop
	cmp.l	%a1@,%d1
	bne.s	.L_memtest_32bitok	/* check again, maybe 0 already had our test value */
	move.l	%d2,0.w				/* restore saved value */
	move.l	%d3,0x100.w
	moveq	#-1,%d1
	bra		.L_memtest_none		/* 24-bit CPU, do not test this range */
.L_memtest_32bitok:
	move.l	%d2,0.w				/* restore saved value */
	move.l	%d3,0x100.w
.L_memtest_24bitaddr:

	/* a0 = tested address, d0 = max size, d1 = current size */

	clr.l	%d1
.L_memtest_loop:
	cmp.l	%d0,%d1
	bge		.L_memtest_done

	move.l	%a0,%d2
	and.l	#0xff000000,%d2
	bne.s	.L_memtest_chipcheck_done	/* no chiptest if 32bit address */
	move.w	#0x7fff,0xdff09a
	nop
	tst.w	%a0@(0x1c,%d1)				/* If non-zero, this is not INTENAR */
	bne.s	.L_memtest_chipcheck_done
	/* It was zero ... */
	move.w	#0xc000,0xdff09a			/* Try the master enable */
	nop
	tst.w	%a0@(0x1c,%d1)				/* If still zero, not INTENAR */
	bne.s	.L_memtest_done				/* It was a custom chip. */
.L_memtest_chipcheck_done:

	move.l	%a0@(%d1),%d3		/* read old value */
	move.l	%a0@,%a2			/* save mirror test contents */
	move.l	#0xfecaf00d,%a0@	/* write mirror test value */
	nop
	move.l	#0xcafed00d,%d2
	move.l	%d2,%a0@(%d1)		/* write test pattern */
	nop
	tst.l	%d1					/* first test addrress? */
	beq.s	.L_memtest_nomirror
	cmp.l	%a0@,%d2			/* no, check mirrorirng */
	bne.s	.L_memtest_nomirror
	move.l	%a2,%a0@			/* restore mirror test contents */
	bra.s	.L_memtest_done
.L_memtest_nomirror:

	not.l	%d2
	move.l	%a0@(4,%d1),%a3		/* read temp address */
	move.l	%d2,%a0@(4,%d1)		/* fill bus with something else */
	not.l	%d2
	nop
	move.l	%a0@(%d1),%d4		/* read test pattern */
	move.l	%a3,%a0@(4,%d1)		/* restore */

	cmp.l	%d4,%d2				/* pattern match? */
	bne.s	.L_memtest_done
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
	bne.s	.L_memtest_done
	not.l	%d2
	move.l	%d3,%a0@(%d1)		/* write old value back */

	move.l	%a2,%a0@			/* restore mirror test contents */

	add.l	%d5,%d1				/* next block */
	bra		.L_memtest_loop

.L_memtest_done:
	tst.l	%d1					/* nothing found? */
	beq.s	.L_memtest_none
	addq.l	#4,%a5
	move.l	%d1,%a5@+
.L_memtest_none:
	move.l	%d1,%d0
	rts

#define STACK_OFFSET (6*4)
	.globl Early_Exception
Early_Exception:
	or.w	#0x0700,%sr
#ifdef AROS_SERIAL_DEBUG
	movem.l	%d0-%d2/%a0-%a2,-(%sp)

	lea		DebugPutHex,%a2

	move.l	STACK_OFFSET+2(%sp),-(%sp)
	pea		.earlystr1(%pc)
	jsr		(%a2)
	addq.l	#8,%sp
	
	moveq	#0,%d0
	move.w	STACK_OFFSET+6(%sp),%d0
	move.l	%d0,-(%sp)
	pea		.earlystr2(%pc)
	jsr		(%a2)
	addq.l	#8,%sp

	move.l	STACK_OFFSET+8(%sp),-(%sp)
	pea		.earlystr3(%pc)
	jsr		(%a2)
	addq.l	#8,%sp

	movem.l	(%sp)+,%d0-%d2/%a0-%a2
#endif
	jmp		Early_TrapHandler

        .section .rodata
.earlystr1:
	.string "Early Exception!\nPC"
.earlystr2:
	.string "Vector"
.earlystr3:
	.string "Address"
