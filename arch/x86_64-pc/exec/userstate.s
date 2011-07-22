#include <aros/x86_64/asm.h>

	.text
	.align  16

	.globl  cpu_SuperState
	.type   cpu_SuperState, @function

	// This is a SuperState() helper. It's called via Supervisor().
	// Its job is to restore a stack pointer and return to user's code.
cpu_SuperState:
	movq	%rsp, %rax		// return int handler stack
	movq	24(%rax),%rsp		// use user stack
	pushq	(%rax)			// push return address
	ret				// return from SuperState() call

	.size  cpu_SuperState, .-cpu_SuperState

	.globl  AROS_SLIB_ENTRY(UserState, Exec, 26)
	.type   AROS_SLIB_ENTRY(UserState, Exec, 26), @function

AROS_SLIB_ENTRY(UserState, Exec, 26):
	// Supervisor return stack is already in rdi
	movq	%rsp, 24(%rdi)			// put USP onto exception stack frame
	movq	%rdi, %rsp			// SSP = SP
	leaq	cpu_UserState(%rip), %rax
	movq	%rax, (%rsp)			// return at this address
	iretq

	.size  AROS_SLIB_ENTRY(UserState, Exec, 26), .-AROS_SLIB_ENTRY(UserState, Exec, 26)

	.type   cpu_UserState, @function
cpu_UserState:
	mov	%ss, %ax		// SS now contains user-mode data segment. Copy it to DS.
	mov	%ax, %ds
	ret

	.size  cpu_UserState, .-cpu_UserState
