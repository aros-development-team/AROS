#include <aros/i386/asm.h>

#include "segments.h"

#define BUILD_IRQ(nr)					\
	.align 16, 0x90;				\
	.globl TRAP##nr##_trap;				\
	.type TRAP##nr##_trap, @function;		\
TRAP##nr##_trap:					\
	pushl $0;					\
	pushl $##nr;					\
	jmp core_Interrupt;				\
	.size TRAP##nr##_trap, .-TRAP##nr##_trap;

#define BUILD_IRQ_ERR(nr)				\
	.align 16, 0x90;				\
	.globl TRAP##nr##_trap;				\
	.type TRAP##nr##_trap, @function;		\
TRAP##nr##_trap:					\
	pushl $##nr;					\
        jmp core_Interrupt;				\
	.size TRAP##nr##_trap, .-TRAP##nr##_trap;

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
BUILD_IRQ_ERR(0x0a)     // Invalid-TSS Exception
BUILD_IRQ_ERR(0x0b)     // Segment-Not-Present Exception
BUILD_IRQ_ERR(0x0c)     // Stack Exception
BUILD_IRQ_ERR(0x0d)     // General-Protection Exception
BUILD_IRQ_ERR(0x0e)     // Page-Fault Exception
BUILD_IRQ(0x0f)         // Reserved
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
BUILD_IRQ(0x1a)
BUILD_IRQ(0x1b)
BUILD_IRQ(0x1c)
BUILD_IRQ(0x1d)
BUILD_IRQ(0x1e)
BUILD_IRQ(0x1f)
BUILD_IRQ(0x20)
BUILD_IRQ(0x21)
BUILD_IRQ(0x22)
BUILD_IRQ(0x23)
BUILD_IRQ(0x24)
BUILD_IRQ(0x25)
BUILD_IRQ(0x26)
BUILD_IRQ(0x27)
BUILD_IRQ(0x28)
BUILD_IRQ(0x29)
BUILD_IRQ(0x2a)
BUILD_IRQ(0x2b)
BUILD_IRQ(0x2c)
BUILD_IRQ(0x2d)
BUILD_IRQ(0x2e)
BUILD_IRQ(0x2f)

BUILD_IRQ(0x80)		// SysCall exception
BUILD_IRQ(0xFE)         // APIC Error Exception

	.align 16, 0x90
	.globl core_Interrupt
	.type core_Interrupt,@function

core_Interrupt:				// At this point two ULONGs for segment registers are
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

	call	handleException		// Call C handler. EBX will be preserved.
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

	.globl	core_Unused_Int
	.type	core_Unused_Int, @function
core_Unused_Int:
	iret
	.size core_Interrupt, .-core_Interrupt

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
