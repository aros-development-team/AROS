#include "segments.h"

	.code16
	cli
	jmp	boot16

	.align 8			# The config variables are passed here
arg1:	.long 0
	.long 0
arg2:	.long 0
	.long 0
arg3:	.long 0
	.long 0
arg4:	.long 0
	.long 0
gdt64:	.long 0
	.long 0
pml4:	.long 0
	.long 0
sp:	.long 0
	.long 0
ip:	.long 0
	.long 0
reloc1:	.long rel1 + 1			# Unfortunately ljmp instruction requires absolute address.
reloc2:	.long rel2 + 2			# We work around this by patching this code after installation.
					# These two LONGs define relocation points.
gdt:	.short 0			# This is our 32-bit GDT
	.short 0
	.short 0
	.short 0
	.short 0x1000
	.short 0x0000
	.short 0x9a00
	.short 0x0040
	.short 0x1000
	.short 0x0000
	.short 0x9200
	.short 0x0040
gdt_sel:				# Table descriptor to load into GDTR
	.short 23			# Limit. We use only first three descriptors here.
	.long 0				# Base
	.long 0				# Base high (for 64 bits)
target:	.long 0				# Far jump destination
	.short KERNEL_CS

boot16:	mov	$0, %eax
	mov	%cs, %ax		# First set up data segment
	mov	%ax, %ds
	shl	$4, %eax		# Determine our location (EAX). Upon startup, CS was set to our base address,
					# and IP was set to zero.
	leal	gdt(%eax), %ebx		# Load physical address of 32-bit gdt
	movl	%ebx, gdt_sel+2(%eax)	# Set up 32-bit gdt address
	movw	%ax, gdt+10(%eax)	# Set base address of 32-bit code segment (bits 0:15)
	movw	%ax, gdt+18(%eax)	# Set base address of 32-bit data segment (bits 0:15)
	mov	$0, %ebx
	mov	%cs, %bx
	shr	$12, %ebx
	movb	%bl, gdt+12(%eax)	# Set base address of 32-bit code segment (bits 16:23)
	movb	%bl, gdt+20(%eax)	# Set base address of 32-bit data segment (bits 16:23)
	ADDR32 DATA32 lgdt gdt_sel(%eax)	# Load gdt
	movl	%cr0, %ebx		# Enter protected mode
	orb	$1, %bl
	movl	%ebx, %cr0
rel1:	ljmp	$0x08, $boot32		# Jump into 32-bit mode, CS = 8.

	.code32
boot32:	movw	$0x10, %ax		# Setup the 32-bit data selectors. EAX is still our base address.
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movl	gdt64(%eax), %ebx	# Load physical address of 64-bit gdt
	movl	%ebx, gdt_sel+2(%eax)	# Set up 64-bit gdt address
	leal	boot64(%eax), %ebx	# Load physical address of 64-bit code
	movl	%ebx, target(%eax)	# Set up 64-bit entry address
	lgdt	gdt_sel(%eax)
	movl	$0xA0, %ebx		# Enable PAE
	movl	%ebx, %cr4
	movl	pml4(%eax), %ebx	# Enable pages
	movl	%ebx, %cr3
	movl	$0xC0000080, %ecx	# EFER MSR number
	rdmsr				# This clobbered our EAX
	orl	$0x0100, %eax		# Enable long mode
	wrmsr
	movl	$0x80000001, %ebx	# Enable paging and activate long mode
	movl	%ebx, %cr0
rel2:  	ljmp	*target

	.code64
boot64:					# We are in 64-bit mode at last! RIP-relative addressing and no segments, cool!!!
	movq	sp(%rip), %rsp		# Set stack pointer
	movq 	ip(%rip), %rax		# Get address to jump to
	movq	arg1(%rip), %rdi	# Load arguments into registers
	movq	arg2(%rip), %rsi
	movq	arg3(%rip), %rdx
	movq	arg4(%rip), %rcx
	movq	$0, %rbp		# Mark end of stack backtrace
	call	*%rax			# GO!!! This must never return
