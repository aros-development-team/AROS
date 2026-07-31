/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: S-mode entry point for the opensbi-riscv64 target.

    OpenSBI (fw_jump/fw_payload, or QEMU -kernel) enters here in S-mode
    with:
        a0 = boot hart id
        a1 = physical address of the device tree blob

    Both are passed through untouched to kernel_cstart().
*/

	.section .aros.init,"ax"
	.globl	_start
	.type	_start,%function

/*
 * RISC-V Linux boot image header. Real hardware (e.g. the Milk-V Titan)
 * is booted through U-Boot/UEFI rather than by OpenSBI directly; both
 * load a flat "Image" and enter it at offset 0 with the same register
 * handoff we already use (a0 = hart id, a1 = DTB). The header lets them
 * identify and place the image; the first word is the branch over it, so
 * the ELF entry point works identically.
 */
_start:
	j	_start_kernel			/* code0                     */
	.word	0				/* code1                     */
	.balign	8
	.dword	0x200000			/* text_offset (2MiB)        */
	.dword	__kernel_end - _start		/* image_size                */
	.dword	0				/* flags: little endian      */
	.word	0x00000002			/* version 0.2               */
	.word	0				/* res1                      */
	.dword	0				/* res2                      */
	.dword	0x5643534952			/* magic "RISCV"             */
	.word	0x05435352			/* magic2 "RSC\x05"          */
	.word	0				/* res4 (PE/COFF offset)     */

_start_kernel:
	/* Mask S-mode interrupts until the interrupt dispatcher exists */
	csrw	sie, zero

	/* Set up the global pointer (linker relaxation must be off while
	   gp itself is being loaded) */
.option push
.option norelax
	la	gp, __global_pointer$
.option pop

	/* Boot stack */
	la	sp, __boot_stack_end

	/* Install the trap handler (needs a valid sp/gp) */
	la	t0, __trap_entry
	csrw	stvec, t0

	/* Enable FPU state - LP64D kernel code may use FP registers at
	   any time. Vector state stays off; the scheduler enables and
	   tracks it per-task (see kernel_cpu.h) */
	li	t0, 0x2000		/* SSTATUS_FS_INITIAL */
	csrs	sstatus, t0

	/* Clear .bss (the ldscript 8-aligns __bss_start/__bss_end) */
	la	t0, __bss_start
	la	t1, __bss_end
1:	bgeu	t0, t1, 2f
	sd	zero, 0(t0)
	addi	t0, t0, 8
	j	1b
2:
	/* kernel_cstart(hartid, fdt) - a0/a1 still hold the SBI values */
	call	kernel_cstart

	/* Should never return; fall through to the spinner */
	.balign	4
__trap_hang:
3:	wfi
	j	3b

	.section .bss
	.balign	16
__boot_stack:
	.space	65536
__boot_stack_end:
