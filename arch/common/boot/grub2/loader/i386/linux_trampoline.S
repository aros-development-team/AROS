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
VARIABLE(grub_linux_trampoline_start)
	cli
	/* %rdi contains protected memory start and %rsi
	contains real memory start. */

	mov %rsi, %rbx

	call base
base:
	pop %rsi

#ifdef APPLE_CC
	lea (cont1 - base) (%esi, 1), %rax
	mov %eax, (jump_vector - base) (%esi, 1)

	lea (gdt - base) (%esi, 1), %rax
	mov %rax, (gdtaddr - base) (%esi, 1)

	/* Switch to compatibility mode. */

	lidt (idtdesc - base) (%esi, 1)
	lgdt (gdtdesc - base) (%esi, 1)

	/* Update %cs. Thanks to David Miller for pointing this mistake out. */
	ljmp *(jump_vector - base) (%esi, 1)
#else
	lea (cont1 - base) (%rsi, 1), %rax
	mov %eax, (jump_vector - base) (%rsi, 1)

	lea (gdt - base) (%rsi, 1), %rax
	mov %rax, (gdtaddr - base) (%rsi, 1)

	/* Switch to compatibility mode. */

	lidt (idtdesc - base) (%rsi, 1)
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
	.code32

	mov %ebx, %esi

	jmp *%edi

	/* GDT. */
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
gdtaddr:
	.quad gdt

idtdesc:
	.word 0
idtaddr:
	.quad 0

	.p2align 4
jump_vector:
	/* Jump location. Is filled by the code */
	.long 0
	.long 0x10
VARIABLE(grub_linux_trampoline_end)
