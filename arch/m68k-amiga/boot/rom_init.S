	.text

	.align 4 

#define SS_STACK_SIZE	0x04000
#define US_STACK_SIZE	0x0c000

	.globl rom_init
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
	move.l  #0f,%a7
	jmp     Exec_ReadGayle
0:
    move.l  %d0,%d6         /* Save for later - %d6 is Gayle ID */

	/* Check for 000000 - 200000 Chip Ram
	 */
.L_chip_mem:
	move.l	#0x000000, %a0
	move.l	#0x200000, %d0
	lea.l	0f,%a7
	jmp	.L_memtest
0:

	/* Stash away fast mem config in:
	 *    %a4 (start)
	 *    %d4 (size)
	 */
	move.l	%a0,%a4
	move.l	%d0,%d4

	/* Check for C00000 - D80000 Fast Ram 
	 * We really want to put our stacks there.
	 */
.L_fast_mem:
	move.l	#0xc00000, %a0
	move.l	#0x180000, %d0
	tst.l   %d6                 /* Gayle? */
	bne.s   0f                  /* Yes, limit to 0x180000 */
	add.l   #0x040000, %d0      /* No, look for ram in Gayle space */
0:
	lea.l	0f,%a7
	jmp	.L_memtest
0:
	/* %d0 is now the memory size */

	/* Default stack is the end of chip memory */
	move.l	%d4,%d1
	move.l	%a4,%a1

	/* Do we have fast memory? */
	tst.l	%d0
	beq	.L_stack_setup

	/* Yes! We have some fast memory! */
	move.l	%d0,%d1
	move.l	%a0,%a1

	/* Put the Exec Supervisor stack at the top 64K,
	 * and the Exec User stack as the next 64K under that
	 */
.L_stack_setup:
	lea.l	%a1@(%d1),%sp
	move.w	#0,%sr		/* Go to user mode */
	subi.l	#SS_STACK_SIZE,%d1
	lea.l	%a1@(%d1),%sp

	/* Was this fast memory? */
	tst.l	%d0
	beq	0f
	/* Yes - shrink fast memory by size of SS */
	move.l	%d1,%d0
	jmp	1f
0:	/* No - shrink chip memory by size of SS */
	move.l	%d1,%d4
1:

	/* Fix up chipmem offsets.
	 *  We need some for exceptions and for some BSS
	 *  ( _bss_end is the end marker )
	 */
	lea.l	_bss_end,%a4
	sub.l	%a4,%d4

	/* Args for start:
	 *   ChipMem Start
	 *   ChipMem Size
	 *   FastMem Start
	 *   FastMem Size
	 *   ss_stack_upper
	 *   ss_stack_lower
	 */
	pea.l	%a1@(%d1)
	pea.l	%a1@(%d1)
	addi.l	#(SS_STACK_SIZE),%sp@
	move.l	%d0,%sp@-
	move.l	%a0,%sp@-
	move.l	%d4,%sp@-
	move.l	%a4,%sp@-
	jsr	start

	/******** Memory tester *******/
	/* TODO: Don't clobber memory of SysBase is valid
	 * IN: A0 - Address, D0 - Possible Size
	 *     A7 - Return address
	 * OUT: D0 - Detected size
	 * TRASH: D1, D2, A1
	 */
.L_memtest:
	clr.l	%d1
.L_memtest_chipcheck:                   /* Can we turn off interrupts? */
	addi.l	#0x40000,%d1
	cmp.l	%d0,%d1
	bgt.s	.L_memtest_chipcheck_done

	lea.l	%a0@(%d1),%a1
	move.w	#0x7fff,%a1@(-(0x40000 - 0x3f09a))
	tst.w	%a1@(-(0x40000 - 0x3f01c))	/* If non-zero, this is not INTENAR */
	bne.s	1f
	/* It was zero ... */
	move.w	#0xc000,%a1@(-(0x40000 - 0x3f09a))    /* Try the master enable */
	tst.w	%a1@(-(0x40000 - 0x3f01c))           /* If still zero, not INTENAR */
	beq.s	1f
0:	/* It was a custom chip. */
	jmp	.L_memtest_chipcheck_done
1:	bra	.L_memtest_chipcheck
.L_memtest_chipcheck_done:
	move.l	%d1,%d0
	subi.l	#0x40000,%d0

	/* TODO: Better pattern checks */
	clr.l	%d1
	move.l	#0xcafed00d,%d2
.L_memtest_pattern:
	move.l	%d2,%a0@(%d1)
	addi.l	#0x40000,%d1	/* 256K is the minimum block size */
	addi.l	#0x40000,%d2	/* 256K is the minimum block size */
	cmp.l	%d0,%d1
	blt.s	.L_memtest_pattern

	clr.l	%d1
	move.l	#0xcafed00d,%d2
.L_memtest_check:
	cmp.l	%a0@(%d1),%d2
	bne	.L_memtest_done
	addi.l	#0x40000,%d1
	addi.l	#0x40000,%d2
	cmp.l	%d0,%d1
	blt.s	.L_memtest_check
.L_memtest_done:
	move.l	%d1,%d0
	jmp	(%a7)

