/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: StackSwap() - swap the task's stack, AArch64 version.

    Translated from the arm-all version. Uses the build-generated symbolic
    offsets from <aros/aarch64/asm.h> (Disable, Enable, tc_SPLower, tc_SPUpper,
    stk_Lower, stk_Upper, stk_Pointer, ThisTask) -- no magic numbers.

    FindTask(NULL)'s vector offset is out of the load immediate range, so
    its address is formed with a sub first.
*/

	#include <aros/aarch64/asm.h>

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec,122)

AROS_SLIB_ENTRY(StackSwap,Exec,122):

	// x0 = newStack (struct StackSwapStruct *)
	// x1 = SysBase
	// x30 = return address
	//
	// Keep live values in registers across the calls; never hold a stack
	// frame that spans the SP swap (it would move with the stack).

	mov	x3, x0			// x3 = newStack

	// FindTask(NULL), not SysBase->ThisTask: on an SMP build the running
	// task is per-CPU and asm.h has no ThisTask at all.
	stp	x1, x3, [sp, #-32]!
	str	x30, [sp, #16]
	mov	x0, #0
	sub	x16, x1, #(0 - FindTask)
	ldr	x16, [x16]
	blr	x16
	mov	x2, x0
	ldr	x30, [sp, #16]
	ldp	x1, x3, [sp], #32

	// Disable() -- arg0 = SysBase. Preserve x1,x2,x3,x30 across the call.
	stp	x1, x2, [sp, #-32]!
	stp	x3, x30, [sp, #16]
	mov	x0, x1
	ldr	x16, [x1, #Disable]
	blr	x16
	ldp	x3, x30, [sp, #16]
	ldp	x1, x2, [sp], #32

	// Swap lower stack boundary: task->tc_SPLower <-> newStack->stk_Lower
	ldr	x0,  [x2, #tc_SPLower]
	ldr	x16, [x3, #stk_Lower]
	str	x16, [x2, #tc_SPLower]
	str	x0,  [x3, #stk_Lower]

	// Swap upper stack boundary
	ldr	x0,  [x2, #tc_SPUpper]
	ldr	x16, [x3, #stk_Upper]
	str	x16, [x2, #tc_SPUpper]
	str	x0,  [x3, #stk_Upper]

	// Swap the stack pointer itself (nothing live on the stack here)
	mov	x0, sp
	ldr	x16, [x3, #stk_Pointer]
	mov	sp, x16
	str	x0, [x3, #stk_Pointer]

	// Enable() -- running on the new stack now. Preserve x1,x30.
	stp	x1, x30, [sp, #-16]!
	mov	x0, x1
	ldr	x16, [x1, #Enable]
	blr	x16
	ldp	x1, x30, [sp], #16

	ret
