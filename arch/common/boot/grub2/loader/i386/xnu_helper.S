/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/symbol.h>


	.p2align	4	/* force 16-byte alignment */

VARIABLE(grub_xnu_launcher_start)
base:
	cli

#ifndef __x86_64__
	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(grub_xnu_heap_will_be_at)
	.long 0
	mov %eax, %edi

	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(grub_xnu_heap_start)
	.long 0
	mov %eax, %esi

	/* mov imm32, %ecx */
	.byte	0xb9
VARIABLE(grub_xnu_heap_size)
	.long 0
	mov %edi, %eax
	add %ecx, %eax
	/* %rax now contains our starting position after relocation. */
	/* One more page to copy: ourselves. */
	add $0x403, %ecx
	shr $2, %ecx

	/* Forward copy.  */
	cld
	rep
	movsl

	mov %eax, %esi
	add $(cont0-base), %eax
	jmp *%eax
cont0:
#else
	xorq %rax, %rax

	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(grub_xnu_heap_will_be_at)
	.long 0
	mov %rax, %rdi

	/* mov imm32, %rax */
	.byte	0x48
	.byte	0xb8
VARIABLE(grub_xnu_heap_start)
	.long 0
	.long 0
	mov %rax, %rsi

	/* mov imm32, %rcx */
	.byte	0x48
	.byte	0xb9
VARIABLE(grub_xnu_heap_size)
	.long 0
	.long 0
	mov %rdi, %rax
	add %rcx, %rax
	/* %rax now contains our starting position after relocation. */
	/* One more page to copy: ourselves. */
	add $0x403, %rcx
	shr $2, %rcx

	/* Forward copy.  */
	cld
	rep
	movsl

	mov %rax, %rsi
#ifdef APPLE_CC
	add $(cont0-base), %eax
#else
	add $(cont0-base), %rax
#endif
	jmp *%rax

cont0:
#ifdef APPLE_CC
	lea (cont1 - base) (%esi, 1), %eax
	mov %eax, (jump_vector - base) (%esi, 1)

	lea (gdt - base) (%esi, 1), %eax
	mov %eax, (gdt_addr - base) (%esi, 1)

	/* Switch to compatibility mode. */

	lgdt (gdtdesc - base) (%esi, 1)

	/* Update %cs. Thanks to David Miller for pointing this mistake out. */
	ljmp *(jump_vector - base) (%esi,1)
#else
	lea (cont1 - base) (%rsi, 1), %rax
	mov %eax, (jump_vector - base) (%rsi, 1)

	lea (gdt - base) (%rsi, 1), %rax
	mov %rax, (gdt_addr - base) (%rsi, 1)

	/* Switch to compatibility mode. */

	lgdt (gdtdesc - base) (%rsi, 1)

	/* Update %cs. Thanks to David Miller for pointing this mistake out. */
	ljmp *(jump_vector - base) (%rsi, 1)
#endif

cont1:
	.code32

	/* Update other registers. */
	mov $0x18, %eax
	mov %eax, %ds
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs
	mov %eax, %ss

	/* Disable paging. */
	mov %cr0, %eax
	and $0x7fffffff, %eax
	mov %eax, %cr0

	/* Disable amd64. */
	mov $0xc0000080, %ecx
	rdmsr
	and $0xfffffeff, %eax
	wrmsr

	/* Turn off PAE. */
	movl %cr4, %eax
	and $0xffffffcf, %eax
	mov %eax, %cr4

	jmp cont2
cont2:
#endif
	.code32

	/* Registers on XNU boot: eip, esp and eax. */
	/* mov imm32, %ecx */
	.byte	0xb9
VARIABLE (grub_xnu_entry_point)
	.long 0
	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE (grub_xnu_arg1)
	.long 0
	/* mov imm32, %ebx */
	.byte	0xbb
VARIABLE (grub_xnu_stack)
	.long 0

	movl %ebx, %esp

	jmp *%ecx

#ifdef __x86_64__
	/* GDT. Copied from loader/i386/linux.c. */
	.p2align 4
gdt:
	/* NULL.  */
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

	/* Reserved.  */
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

	/* Code segment.  */
	.byte 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00

	/* Data segment.  */
	.byte 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00

gdtdesc:
	.word 31
gdt_addr:
	/* Filled by the code. */
	.quad 0

	.p2align 4
jump_vector:
	/* Jump location. Is filled by the code */
	.long 0
	.long 0x10
#endif
VARIABLE(grub_xnu_launcher_end)
