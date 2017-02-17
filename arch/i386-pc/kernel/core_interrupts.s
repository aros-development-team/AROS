#include <aros/i386/asm.h>

#include "segments.h"

#define BUILD_IRQ(nr)					\
	.align 16, 0x90;				\
	.globl IRQ##nr##_intr;				\
	.type IRQ##nr##_intr, @function;		\
IRQ##nr##_intr:					\
	pushl $0;					\
	pushl $##nr;					\
	jmp core_EnterInterrupt;				\
	.size IRQ##nr##_intr, .-IRQ##nr##_intr;

#define BUILD_IRQ_ERR(nr)				\
	.align 16, 0x90;				\
	.globl IRQ##nr##_intr;				\
	.type IRQ##nr##_intr, @function;		\
IRQ##nr##_intr:					\
	pushl $##nr;					\
        jmp core_EnterInterrupt;				\
	.size IRQ##nr##_intr, .-IRQ##nr##_intr;

#define B(x,y) BUILD_IRQ(x##y)
#define BUILD_16(x) \
    B(x,0) B(x,1) B(x,2) B(x,3) B(x,4) B(x,5) B(x,6) B(x,7) \
    B(x,8) B(x,9) B(x,A) B(x,B) B(x,C) B(x,D) B(x,E) B(x,F)

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

	.align 16, 0x90
	.globl core_EnterInterrupt
	.type core_EnterInterrupt,@function

core_EnterInterrupt:				// At this point two ULONGs for segment registers are
					// already reserved. They are occupied by error code and IRQ number
	pushl	$0			// Reserve two more ULONGs (for ES and DS)
	pushl	$0
	pushl	%ebp			// Now save GPRs
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	pushl	$ECF_SEGMENTS		// Flags. We have no FPU context here and even no pointer for it.
	movl    %esp, %ebx		// Fixate context pointer in EBX
	movl	reg_fs(%ebx), %eax	// IRQ number - third argument to core_IRQHandle()
	pushl	%eax
	movl	reg_gs(%ebx), %eax	// Error number - second argument
	pushl	%eax
	pushl	%ebx			// Context pointer - first argument
	xorl	%eax, %eax		// Zero-pad segments
	mov	%ds, %ax		// Now save segment registers
	movl	%eax, reg_ds(%ebx)
	mov	%es, %ax
	movl	%eax, reg_es(%ebx)
	mov	%fs, %ax
	movl	%eax, reg_fs(%ebx)
	mov	%gs, %ax
	movl	%eax, reg_gs(%ebx)
	mov	$KERNEL_DS, %ax		// We are supervisor now
	mov	%ax, %ds
	mov	%ax, %es

	call	core_IRQHandle		// Call C handler. EBX will be preserved.
restoreRegs:
	movl	Flags(%ebx), %eax	// Test if the context contains segment registers
	test	$ECF_SEGMENTS, %eax
	je	noSegments
	movl	reg_ds(%ebx), %eax	// Restore segment registers if present
	mov	%ax, %ds
	movl	reg_es(%ebx), %eax
	mov	%ax, %es
	movl	reg_fs(%ebx), %eax
	mov	%ax, %fs
	movl	reg_gs(%ebx), %eax
	mov	%ax, %gs
noSegments:
	movl	%ebx, %esp		// Load context pointer into SP, we will pop everything
	popl	%eax			// These were flags, just remove them
	popl	%eax			// Restore GPRs
	popl	%ebx
	popl	%ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebp
        addl	$16, %esp		// Remove segments

	.globl	core_DefaultIRET
	.type	core_DefaultIRET, @function
core_DefaultIRET:
	iret
	.size core_EnterInterrupt, .-core_EnterInterrupt

	.globl core_LeaveInterrupt
	.type core_LeaveInterrupt, @function
core_LeaveInterrupt:
	popl	%ebx			// Remove return address
	popl	%ebx			// Get argument
	jmp	restoreRegs
	.size core_LeaveInterrupt, .-core_LeaveInterrupt

	.globl core_Supervisor
	.type core_Supervisor, @function

core_Supervisor:
	popl	%ebx			// Similar to above, but chains to the routine
	popl	%ebx			// pointed to by EDI
	movl	Flags(%ebx), %eax	// Note that data segments will be reset back to user-mode values
	test	$ECF_SEGMENTS, %eax
	je	sv_noSegments
	movl	reg_ds(%ebx), %eax
	mov	%ax, %ds
	movl	reg_es(%ebx), %eax
	mov	%ax, %es
	movl	reg_fs(%ebx), %eax
	mov	%ax, %fs
	movl	reg_gs(%ebx), %eax
	mov	%ax, %gs
sv_noSegments:
	movl	%ebx, %esp
	popl	%eax
	popl	%eax
	popl	%ebx
	popl	%ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebp
        addl	$16, %esp
	jmp	*%edi
