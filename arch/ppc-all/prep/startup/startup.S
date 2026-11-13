/*
    Copyright C 2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bootstrap code for AROS/PPC on PReP
    Lang: English
*/

/*
    This is the startup code for PReP (PowerPC Reference Platform) machines.
    It may be loaded by Firmware into ram (at almost any possible location).

    Real kernel location.
    The older PReP machines have the DMA area limited to first 16MB of system
    memory. Therefore it is reasonable, to place the kernel at the end of DMA
    RAM. Both areas may be then easily separated.

    Currently, the kernel should be located at 15th megabyte of RAM but it may
    be changed in future.
*/

#include <asm/ppc.h>

	.text

	.globl	startup
	.type	startup,@function
startup:
	/* Update the Link Pointer by jumping to the next instruction */
	/* this way we know exactly where the code is loaded */

	bl	1f

	.globl	core_id
core_id:
	.string "PPC Core 1.0"
	.balign	4,0

1:	mflr	r20		/* Extract the laod address */
	addi	r20,r20,-4
	
        mr	r30,r3		/* Residual/boot data */
        mr	r31,r5		/* OFW pointer */

        li	r3,'B'
        bl	com1_putc

        mfmsr	r29
	/* Enable FPU and chose the 0xFFFxxxxx prefix for exception table */
	li	r3,(MSR_FP|MSR_IP)
	mtmsr	r3

        li	r3,'O'
        bl	com1_putc

	lis	r4,startup@h	/* Calculate the size of kernel */
	ori	r4,r4,startup@l
	lis	r5,_END@h
	ori	r5,r5,_END@l
	addi	r5,r5,3
	srwi	r21,r21,2
	sub	r21,r5,r4	/* r21 contains the size of kernel in words */
	mr	r22,r4

        li	r3,'O'
        bl	com1_putc
	
	/* Is the required address same as the load address? */
	cmp	0,r20,r4
	beq	address_ok

reloc:
	addi	r3,r20,-4	/* r3 contains the target - 4 */
	addi	r4,r4,-4	/* r4 contains the source - 4 */
	mtctr	r21		/* length of data to move */
0:	lwzu	r5,4(r3)
	stwu	r5,4(r4)
	bdnz	0b

	/* Jump to the relocated code */
	lis	r5,address_ok@h
	ori	r5,r5,address_ok@l
	mtlr	r5
	blr
	
address_ok:
        li	r3,'T'
        bl	com1_putc

/*
    Setup the stack pointer. It will start just a little bit before the 	
    kernel. We may relocate it later.

    I know this is a bit tricky hack, but I really want to avoid any .data
    or .bss sections in kernel.
*/

	lis	r9,startup@h
	la	r3,startup@l(r9)
	addi	r1,r3,-32
        rlwinm	r1,r1,0,0,27

/*
    Oh my.... if it happend at all, that we have some .bbs data, I should clear
    it now. Other possibility would be to halt the kernel, display the message
    that the kernel is f**ked and restart ;)
*/

	li	r0,0
	lis	r3,_EDATA@h
	ori	r3,r3,_EDATA@l
	lis	r4,_END@h
	ori	r4,r4,_END@l
	sub.	r4,r4,r3
	beq+	1f
	subi	r3,r3,4
	mtctr	r4
0:	stwu	r0,4(r3)
	bdnz	0b

1:
        li	r0,0
	/* Clear all BAT registers as we don't need any memory translation */
	mtspr	DBAT0U,r0
	mtspr	DBAT0L,r0
	mtspr	DBAT1U,r0
	mtspr	DBAT1L,r0
	mtspr	DBAT2U,r0
	mtspr	DBAT2L,r0
	mtspr	DBAT3U,r0
	mtspr	DBAT3L,r0

	mtspr	IBAT0U,r0
	mtspr	IBAT0L,r0
	mtspr	IBAT1U,r0
	mtspr	IBAT1L,r0
	mtspr	IBAT2U,r0
	mtspr	IBAT2L,r0
	mtspr	IBAT3U,r0
	mtspr	IBAT3L,r0

/* Setup the segment registers - Linear memory area 0x00000000 - 0xffffffff */

	li	r3,16
	mtctr	r3
	lis	r3,0x2000
	li	r4,0
2:	mtsrin	r3,r4
	addi	r3,r3,0x0111
	addis	r4,r4,0x1000
	bdnz	2b

	mr	r3,r20	    /* load address */
	mr	r4,r22	    /* Real kernel address */
	mr	r5,r21	    /* Size of kernel in words */
	mr	r6,r30	    /* Residual/boot data */
	mr	r7,r31	    /* OFW pointer */
	mr	r8,r29	    /* Original MSR for OFW interface */
	b	cstart	    /* Jump to the C-part of kernel */


	.globl	com1_putc
	.type com1_putc,@function
com1_putc:
	lis	r9,0x8000
	ori	r9,r9,0x3f8
1:	lbz	r0,5(r9)
	andi.	r11,r0,32
	bc	12,2,1b
	stb	r3,0(r9)
	sync
	blr
