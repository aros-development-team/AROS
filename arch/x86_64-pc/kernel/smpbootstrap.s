# This is the bootstrap code for secondary cores (or APs - Application Processors)
# Every AP starts ip in real mode from the address we specify. The address is page-
# aligned, and it is converted by hardware to segment:offset notation and loaded
# into CS:IP. IP is effectively zero and CS is the address divided by 16.
# This code is statically linked at address 0. But segmentation makes if effectively
# position-indepentent. The only thing which is true, is that the code starts at
# zero offset in the segment. This is why we can safely use absolute addressing here.
# 
# This code doesn not use main AROS 64-bit GDT, but uses a temporary one instead.
# This is done because theoretically AROS GDT can be placed anywhere in 64-bit memory,
# but our GDT must be reachable at least from within 32-bit mode, to be able to
# load its address. AROS replaces this GDT with own one in core_CPUSetup() function.

	.code16
	cli
	jmp	boot16

	.align	8			# The config variables are passed here
arg1:	.long	0
	.long	0
arg2:	.long	0
	.long	0
arg3:	.long	0
	.long	0
arg4:	.long	0
	.long	0
pml4:	.long	0
	.long	0
sp:	.long	0
	.long	0
ip:	.long	0
	.long	0
gdt32:	.short	0			# Temporary GDT for 32-bit mode.
	.short 	0			# Just two supervisor-mode segments of the same length,
	.short	0			# one for code and one for data.
	.short	0			# Zero descriptor is never used.
	.short	0x1000
	.short	0x0000
	.short	0x9a00
	.short	0x0040
	.short	0x1000
	.short	0x0000
	.short	0x9200
	.short	0x0040
gdt64:  .short	0			# Temporary 64-bit GDT. Again just two segments.
	.short 	0
	.short	0
	.short	0
	.short	0xffff
	.short	0x0000
	.short	0x9a00
	.short	0x00af
	.short	0xffff
	.short	0x0000
	.short	0x9200
	.short	0x00af
gdtr32:					# 32-bit GDTR
	.short	23			# Limit. We use only first three descriptors here.
	.long	0			# Base
gdtr64:					# 64-bit GDTR.
	.short	23			# Limit
	.long	0			# Base (32-bit notation)
target:					# Far jump destination
	.long	0			# Address (32-bit notation)
	.short	8			# Segment selector

# Well, here we are in real mode. We want to reach long mode, but we cannot do that directly.
# We need to jump to 32-bit protected mode first. We already have a GDT for this. But in order
# not to lose ourselves, we need to adjust base addresses of 32-bit segments to be equal to those
# of 16-bit segments, so that our code stays at logical address zero.
# The only truly absolute (physical) addresses here are segment starts in GDT and
# 64-bit code address. We fix them up in this part, using initial CS value to get our physical location.

boot16:	mov	$0, %eax
	mov	%cs, %ax		# First set up data segment
	mov	%ax, %ds
	shl	$4, %eax		# Determine our location in 32-bit form (EAX)
	leal	gdt32(%eax), %ebx	# Load physical address of 32-bit gdt
	movl	%ebx, gdtr32+2		# Set up 32-bit gdt address
	leal	gdt64(%eax), %ebx	# Load physical address of 64-bit gdt
	movl	%ebx, gdtr64+2		# Set up 64-bit gdt address
	leal	boot64(%eax), %ebx	# Load physical address of 64-bit code
	movl	%ebx, target		# Set up 64-bit entry address
	movw	%ax, gdt32+10		# Set base address of 32-bit code segment (bits 0:15)
	movw	%ax, gdt32+18		# Set base address of 32-bit data segment (bits 0:15)
	shr	$16, %eax
	movb	%al, gdt32+12		# Set base address of 32-bit code segment (bits 16:23)
	movb	%al, gdt32+20		# Set base address of 32-bit data segment (bits 16:23)
	ADDR32 DATA32 lgdt gdtr32	# Load 32-bit gdt
	movl	%cr0, %eax		# Enter protected mode
	orb	$1, %al
	movl	%eax, %cr0
rel1:	ljmp	$0x08, $boot32		# Jump into 32-bit mode, CS = 8.

# Here we are in 32-bit mode and can proceed to 64-bit one.
# In 32-bit mode segmentation is still active, and base addresses of our segments
# have been adjusted to match 16-bit ones. So we still start at logical address zero
# and still can use absolute addressing.

	.code32
boot32:	movw	$0x10, %ax		# Setup the 32-bit data selectors
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	lgdt	gdtr64			# Load 64-bit gdt
	movl	$0x000000A0, %eax	# Enable PAE
	movl	%eax, %cr4
	movl	pml4, %ebx		# Enable pages
	movl	%ebx, %cr3
	movl	$0xC0000080, %ecx	# EFER MSR number
	rdmsr
	orl	$0x00000100, %eax	# Enable long mode
	wrmsr
	movl	$0x80000001, %eax	# Enable paging and activate long mode
	movl	%eax, %cr0
rel2:  	ljmp	*target

# Phew, we are in 64-bit mode at last!
# In 64-bit mode there is effectively no segmentation. Offsets and limits are ignored.
# Segment descriptors are used only to specify CPU privilege level. This is why we
# needed to find an an absolute physical address of this point.
# The simplest way to achieve position independency here is to use RIP-relative addressing.
# Here we effectively do the same as core_Kick() function does, just with no .bss clearing.

	.code64
boot64:	movq	sp(%rip), %rsp		# Set stack pointer
	movq 	ip(%rip), %rax		# Get address to jump to
	movq	arg1(%rip), %rdi	# Load arguments into registers
	movq	arg2(%rip), %rsi
	movq	arg3(%rip), %rdx
	movq	arg4(%rip), %rcx
	movq	$0, %rbp		# Mark end of stack backtrace
	call	*%rax			# GO!!! This must never return.
