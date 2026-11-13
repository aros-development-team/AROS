#include <aros/i386/asm.h>

	.text
	.align  16

	.globl  cpu_SuperState
	.type   cpu_SuperState, @function

	// This is a SuperState() helper. It's called via Supervisor().
	// Its job is to restore a stack pointer and return to user's code.
cpu_SuperState:
	movl	%esp, %eax		// return int handler stack
	movl	12(%eax),%esp		// use user stack
	pushl	(%eax)			// push return address
	ret				// return from SuperState() call

	.size  cpu_SuperState, .-cpu_SuperState

	.globl  AROS_SLIB_ENTRY(UserState, Exec, 26)
	.type   AROS_SLIB_ENTRY(UserState, Exec, 26), @function

AROS_SLIB_ENTRY(UserState, Exec, 26):
	popl	%ecx			// Get return address
	movl	(%esp), %eax		// Get supervisor return stack
	movl	%ecx, (%eax)		// Set return address
	movl	%esp, 12(%eax)		// Set USP in exception stack frame
	movl	%eax, %esp		// SSP = SP
	iret				// Exit from interrupt

	.size  AROS_SLIB_ENTRY(UserState, Exec, 26), .-AROS_SLIB_ENTRY(UserState, Exec, 26)
