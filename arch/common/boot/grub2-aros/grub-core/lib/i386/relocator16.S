/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009,2010  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */
	
/* The code segment of the protected mode.  */
#define CODE_SEGMENT	0x08

/* The data segment of the protected mode.  */
#define DATA_SEGMENT	0x10

#define PSEUDO_REAL_CSEG 0x18

#define PSEUDO_REAL_DSEG 0x20

#include "relocator_common.S"
	
	.p2align	4	/* force 16-byte alignment */

VARIABLE(grub_relocator16_start)
	PREAMBLE

	movl 	%esi, %eax
	movw	%ax, (LOCAL (cs_base_bytes12) - LOCAL (base)) (RSI, 1)
	shrl	$16, %eax
	movb	%al, (LOCAL (cs_base_byte3) - LOCAL (base)) (RSI, 1)

	RELOAD_GDT
	.code32
	/* Update other registers. */
	movl	$DATA_SEGMENT, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %fs
	movl	%eax, %gs
	movl	%eax, %ss

	DISABLE_PAGING

#ifdef __x86_64__
	/* Disable amd64. */
	movl	$GRUB_MEMORY_CPU_AMD64_MSR, %ecx
	rdmsr
	andl	$(~GRUB_MEMORY_CPU_AMD64_MSR_ON), %eax
	wrmsr
#endif

	/* Turn off PAE. */
	movl	%cr4, %eax
	andl	$(~GRUB_MEMORY_CPU_CR4_PAE_ON), %eax
	movl	%eax, %cr4

	/* Update other registers. */
	movl	$PSEUDO_REAL_DSEG, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %fs
	movl	%eax, %gs
	movl	%eax, %ss

	movl 	%esi, %eax
	shrl	$4, %eax
	movw	%ax, (LOCAL (segment) - LOCAL (base)) (%esi, 1)
	
	/* jump to a 16 bit segment */
	ljmp	$PSEUDO_REAL_CSEG, $(LOCAL (cont2) - LOCAL(base))
LOCAL(cont2):
	.code16

	/* clear the PE bit of CR0 */
	movl	%cr0, %eax
	andl 	$(~GRUB_MEMORY_CPU_CR0_PE_ON), %eax
	movl	%eax, %cr0

	/* flush prefetch queue, reload %cs */
	/* ljmp  */
	.byte	0xea
	.word 	LOCAL(cont3)-LOCAL(base)
LOCAL(segment):
	.word	0

LOCAL(cont3):
	/* we are in real mode now
	 * set up the real mode segment registers : DS, SS, ES
	 */
	/* movw imm16, %ax.  */
	.byte	0xb8
VARIABLE(grub_relocator16_ds)
	.word	0
	movw	%ax, %ds

	/* movw imm16, %ax.  */
	.byte	0xb8
VARIABLE(grub_relocator16_es)
	.word	0
	movw	%ax, %es

	/* movw imm16, %ax.  */
	.byte	0xb8
VARIABLE(grub_relocator16_fs)
	.word	0
	movw	%ax, %fs

	/* movw imm16, %ax.  */
	.byte	0xb8
VARIABLE(grub_relocator16_gs)
	.word	0
	movw	%ax, %gs

	/* movw imm16, %ax.  */
	.byte	0xb8
VARIABLE(grub_relocator16_ss)
	.word	0
	movw	%ax, %ss

	/* movw imm16, %ax.  */
	.byte	0xb8
VARIABLE(grub_relocator16_sp)
	.word	0
	movw	%ax, %ss

	/* movw imm32, %edx.  */
	.byte	0x66, 0xba
VARIABLE(grub_relocator16_edx)
	.long	0
	
	/* Cleared direction flag is of no problem with any current
	   payload and makes this implementation easier.  */
	cld

	/* ljmp */
	.byte	0xea
VARIABLE(grub_relocator16_ip)
	.word	0
VARIABLE(grub_relocator16_cs)
	.word	0

	.code32

	/* GDT. Copied from loader/i386/linux.c. */
	.p2align	4
LOCAL(gdt):
	.word	0, 0
	.byte	0, 0, 0, 0

	/* -- code segment --
	 * base = 0x00000000, limit = 0xFFFFF (4 KiB Granularity), present
	 * type = 32bit code execute/read, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte 	0, 0x9A, 0xCF, 0

	/* -- data segment --
	 * base = 0x00000000, limit 0xFFFFF (4 KiB Granularity), present
	 * type = 32 bit data read/write, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0xCF, 0

	/* -- 16 bit real mode CS --
	 * base = 0x00000000, limit 0x0FFFF (1 B Granularity), present
	 * type = 16 bit code execute/read only/conforming, DPL = 0
	 */
	.word	0xFFFF
LOCAL(cs_base_bytes12):
	.word	0
LOCAL(cs_base_byte3):
	.byte	0

	.byte	0x9E, 0, 0

	/* -- 16 bit real mode DS --
	 * base = 0x00000000, limit 0x0FFFF (1 B Granularity), present
	 * type = 16 bit data read/write, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0, 0
LOCAL(gdt_end):

VARIABLE(grub_relocator16_end)
