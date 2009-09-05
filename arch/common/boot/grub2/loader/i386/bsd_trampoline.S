/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (c) 2003  Peter Wemm <peter@FreeBSD.org>
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

/* Based on the code from FreeBSD originally distributed under the
   following terms: */

/*-
 * Copyright (c) 2003  Peter Wemm <peter@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */


#define MSR_EFER	0xc0000080
#define EFER_LME	0x00000100
#define CR4_PAE		0x00000020
#define CR4_PSE		0x00000010
#define CR0_PG		0x80000000

#include <grub/symbol.h>

	.p2align	2

	.code32


VARIABLE(grub_bsd64_trampoline_start)

	/* Discard `grub_unix_real_boot' return address.  */
        popl    %eax

        /* entry  */
        popl	%edi

        /* entry_hi  */
        popl	%esi

	cli

	/* Turn on EFER.LME.  */
	movl	$MSR_EFER, %ecx
	rdmsr
	orl	$EFER_LME, %eax
        wrmsr

	/* Turn on PAE.  */
	movl	%cr4, %eax
	orl	$(CR4_PAE | CR4_PSE), %eax
	movl	%eax, %cr4

	/* Set %cr3 for PT4.  */
	popl	%eax
	movl    %eax, %cr3

	/* Push a dummy return address.  */
	pushl	%eax

	/* Turn on paging (implicitly sets EFER.LMA).  */
	movl	%cr0, %eax
	orl	$CR0_PG, %eax
	movl	%eax, %cr0

	/* Now we're in compatibility mode. set %cs for long mode.  */
	/* lgdt */
	.byte 0x0f
	.byte 0x01
	.byte 0x15
VARIABLE (grub_bsd64_trampoline_gdt)
	.long 0x0

	/* ljmp */
	.byte 0xea
VARIABLE (grub_bsd64_trampoline_selfjump)
	.long 0x0
	.word 0x08

	.code64

bsd64_longmode:
         /* We're still running V=P, jump to entry point.  */
	movl	%esi, %eax
	salq	$32, %rax
	orq	%rdi, %rax
	pushq	%rax
	ret
VARIABLE(grub_bsd64_trampoline_end)
