# smpbootstrap.c
#
#  Created on: Nov 19, 2009
#      Author: misc

	.code16
	cli
	jmp	boot16

	.align	4			# The config variables are passed here...
arg1:	.long	0
arg2:	.long	0
arg3:	.long	0
sp:	.long	0
ip:	.long	0
gdt:	.short	0
	.short	0
	.short	0
	.short	0
	.short	0x1000			# 0x08: small CS selector
	.short	0x0000
	.short	0x9a00
	.short	0x0040 
	.short	0x1000			# 0x10: small DS selector
	.short	0x0000
	.short	0x9200
	.short	0x0040
	.short	0xffff			# 0x18: 4GB CS selector
	.short	0x0000
	.short	0x9a00
	.short	0x00cf 
	.short	0xffff			# 0x20: 4GB DS selector
	.short	0x0000
	.short	0x9200
	.short	0x00cf
gdtr:	.short	39
	.long	0
target:	.long	0
	.short	0x18
	
boot16:	mov	$0, %eax
	mov	%cs, %ax		# Find out where the code resides
	mov	%ax, %ds
	shl	$4, %eax
	leal	gdt(%eax), %ebx		# Load physical address of 32-bit gdt
	movl	%ebx, gdtr+2		# Set up 32-bit gdt address
	leal	kick(%eax), %ebx	# Load physical address of flat-mode code
	movl	%ebx, target		# Set up entry address
	movw	%ax, gdt+10		# Set base address of 32-bit code segment (bits 0:15)
	movw	%ax, gdt+18		# Set base address of 32-bit data segment (bits 0:15)
	shr	$16, %eax
	movb	%al, gdt+12		# Set base address of 32-bit code segment (bits 16:23)
	movb	%al, gdt+20		# Set base address of 32-bit data segment (bits 16:23)
	ADDR32 DATA32 lgdt gdtr		# Load gdt
	movl	%cr0, %eax		# Enter protected mode
	orb	$1, %al
	movl	%eax, %cr0
	ljmp	$0x8, $boot32		# Please note that the %cs segment selector has to have it's base address properly adjusted

	.code32
boot32:	movw	$0x10, %ax		# Setup the temporary 32-bit data selector
	movw	%ax, %ds
	movw	$0x20, %ax		# Set stack to large 32-bit data segment
	movw	%ax, %ss
	movl	sp, %esp		# Load stack pointer
	movl	arg3, %ebx		# Prepare arguments for the C entry point
	pushl	%ebx
	movl	arg2, %ebx
	pushl	%ebx
	movl	arg1, %ebx
	pushl	%ebx
	movl	ip, %ebx		# Prepare entry address
	ljmp	*target

kick:	mov	%ax, %ds		# Switch to large data segments
	mov	%ax, %es
	movl	$0, %ebp
	call	*%ebx			# Jump to the C entry point
