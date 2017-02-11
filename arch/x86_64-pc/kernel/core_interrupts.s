/* CPU interrupt handling entry points */

#include <aros/x86_64/asm.h>

#include "segments.h"

#define BUILD_IRQ(nr)					\
	.balign 8, 0x90;				\
	.globl IRQ##nr##_intr;				\
	.type IRQ##nr##_intr, @function;		\
IRQ##nr##_intr:						\
	pushq $0;					\
	pushq $##nr;					\
	jmp core_EnterInterrupt;			\
	.size IRQ##nr##_intr, .-IRQ##nr##_intr;

#define BUILD_IRQ_ERR(nr)				\
	.balign 8, 0x90;				\
        .globl IRQ##nr##_intr;				\
        .type IRQ##nr##_intr, @function;		\
IRQ##nr##_intr:						\
	pushq $##nr;					\
        jmp core_EnterInterrupt;			\
        .size IRQ##nr##_intr, .-IRQ##nr##_intr;

#define B(x,y) BUILD_IRQ(x##y)
#define BUILD_16(x) \
    B(x,0) B(x,1) B(x,2) B(x,3) B(x,4) B(x,5) B(x,6) B(x,7) \
    B(x,8) B(x,9) B(x,a) B(x,b) B(x,c) B(x,d) B(x,e) B(x,f)

BUILD_IRQ(0x00)         // Divide-By-Zero Exception
BUILD_IRQ(0x01)         // Debug Exception
BUILD_IRQ(0x02)         // NMI Exception
BUILD_IRQ(0x03)         // Breakpoint Exception
BUILD_IRQ(0x04)         // Overflow Exception
BUILD_IRQ(0x05)         // Bound-Range Exception
BUILD_IRQ(0x06)         // Invalid-Opcode Exception
BUILD_IRQ(0x07)         // Device-Not-Available Exception
BUILD_IRQ_ERR(0x08)     // Double-Fault Exception
BUILD_IRQ(0x09)         // Unused (used to be Coprocesor-Segment-Overrun)
BUILD_IRQ_ERR(0x0A)     // Invalid-TSS Exception
BUILD_IRQ_ERR(0x0B)     // Segment-Not-Present Exception
BUILD_IRQ_ERR(0x0C)     // Stack Exception
BUILD_IRQ_ERR(0x0D)     // General-Protection Exception
BUILD_IRQ_ERR(0x0E)     // Page-Fault Exception
BUILD_IRQ(0x0F)         // Reserved
BUILD_IRQ(0x10)         // Floating-Point Exception
BUILD_IRQ_ERR(0x11)     // Alignment-Check Exception
BUILD_IRQ(0x12)         // Machine-Check Exception
BUILD_IRQ(0x13)         // SIMD-Floating-Point Exception
BUILD_IRQ(0x14)
BUILD_IRQ(0x15)
BUILD_IRQ(0x16)
BUILD_IRQ(0x17) 
BUILD_IRQ(0x18)
BUILD_IRQ(0x19)
BUILD_IRQ(0x1A)
BUILD_IRQ(0x1B)
BUILD_IRQ(0x1C)
BUILD_IRQ(0x1D)
BUILD_IRQ(0x1E)
BUILD_IRQ(0x1F)

BUILD_16(0x2)           // Hardware IRQs...
BUILD_16(0x3)
BUILD_16(0x4)
BUILD_16(0x5)
BUILD_16(0x6)
BUILD_16(0x7)
BUILD_16(0x8)
BUILD_16(0x9)
BUILD_16(0xA)
BUILD_16(0xB)
BUILD_16(0xC)
BUILD_16(0xD)
BUILD_16(0xE)
BUILD_IRQ(0xF0)
BUILD_IRQ(0xF1)
BUILD_IRQ(0xF2)
BUILD_IRQ(0xF3)
BUILD_IRQ(0xF4)
BUILD_IRQ(0xF5)
BUILD_IRQ(0xF6)
BUILD_IRQ(0xF7) 
BUILD_IRQ(0xF8)
BUILD_IRQ(0xF9)
BUILD_IRQ(0xFA)
BUILD_IRQ(0xFB)
BUILD_IRQ(0xFC)
BUILD_IRQ(0xFD)
BUILD_IRQ(0xFE)
BUILD_IRQ(0xFF)

	.balign 32, 0x90
	.globl core_EnterInterrupt
	.type core_EnterInterrupt,@function

core_EnterInterrupt:			// At this point two UQUADs for segment registers are
					// already reserved. They are occupied by error code and IRQ number
	pushq	$0			// Reserve two more UQUADs (for ES and DS)
	pushq	$0
	pushq	%rbp			// Now save GPRs
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%r11
	pushq	%r10
	pushq	%r9
	pushq	%r8
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbx
	pushq	%rax
	pushq	$ECF_SEGMENTS		// Flags. We have no FPU context here and even no pointer for it.
	movq    %rsp, %rdi		// Supply context pointer to core_IRQHandle (first argument)
	movq	reg_gs(%rdi), %rsi	// Error number - second argument
	movq	reg_fs(%rdi), %rdx	// IRQ number - third argument
	xorq	%rax, %rax		// Zero-pad segments
	mov	%ds, %ax		// Now save segment registers
	movq	%rax, reg_ds(%rdi)
	mov	%es, %ax
	movq	%rax, reg_es(%rdi)
	mov	%fs, %ax
	movq	%rax, reg_fs(%rdi)
	mov	%gs, %ax
	movq	%rax, reg_gs(%rdi)
					// In x86-64 only CS is used to determine current CPL.
					// Also SS is used as an indicator into which mode iretq returns
					// (set to zero when interrupt raises privilege level).
					// So we do not have to manipulate segment registers here (unlike on i386).
	jmp	core_IRQHandle		// Proceed to C handler
	.size core_EnterInterrupt, .-core_EnterInterrupt

	.globl core_LeaveInterrupt
	.type core_LeaveInterrupt, @function

core_LeaveInterrupt:
	movl	Flags(%rdi), %eax	// Test if the context contains segment registers
	test	$ECF_SEGMENTS, %eax
	je	noSegments
	movq	reg_ds(%rdi), %rax	// Restore segment registers if present
	mov	%ax, %ds
	movq	reg_es(%rdi), %rax
	mov	%ax, %es
	movq	reg_fs(%rdi), %rax
	mov	%ax, %fs
	movq	reg_gs(%rdi), %rax
	mov	%ax, %gs
noSegments:
	movq	%rdi, %rsp		// Load context pointer into SP, we will pop everything
	popq	%rax			// These were flags, just remove them
	popq	%rax			// Restore GPRs
	popq	%rbx
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	popq	%r8
	popq	%r9
	popq	%r10
	popq	%r11
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
        addq	$32, %rsp		// Remove segments

	.globl core_DefaultIRETQ
	.type core_DefaultIRETQ, @function

core_DefaultIRETQ:
	iretq				// Exit
	.size core_LeaveInterrupt, .-core_LeaveInterrupt

	.globl core_Supervisor
	.type core_Supervisor, @function

core_Supervisor:
	movq	%rdi, %rsp		// Similar to above, but does not restore segment registers, and
	popq	%rax			// chains to the routine pointed to by RDI.
	popq	%rax			// Used for Supervisor() implementation.
	popq	%rbx			// We could use int 0x81 with the only jmpq *%rdi instruction,
	popq	%rcx			// but it's easier to jump to such code ocassionally. I hope
	popq	%rdx			// it's not a big speed loss
	popq	%rsi
	popq	%rdi
	popq	%r8
	popq	%r9
	popq	%r10
	popq	%r11
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
        addq	$32, %rsp
        jmpq	*%rdi
	.size core_Supervisor, .-core_Supervisor
