/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: StackSwap() - swap the task's stack, RISC-V version.

    Translated from the aarch64-all version. Uses the build-generated
    symbolic offsets from <aros/riscv/asm.h> (Disable, Enable,
    tc_SPLower, tc_SPUpper, stk_Lower, stk_Upper, stk_Pointer, ThisTask)
    - no magic numbers. On a non-SMP build FindTask(NULL) simply returns
    SysBase->ThisTask, so ThisTask is read directly.
*/

	#include <aros/riscv/asm.h>

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec,122)

AROS_SLIB_ENTRY(StackSwap,Exec,122):

	// a0 = newStack (struct StackSwapStruct *)
	// a1 = SysBase
	// ra = return address
	//
	// Keep live values on the (old) stack across the Disable() call and
	// in registers across the SP swap itself; never hold a stack frame
	// that spans the swap (it would move with the stack).

	mv	t0, a0			// t0 = newStack
	lw	t1, ThisTask(a1)	// t1 = current task

	// Disable() - arg0 = SysBase. t0/t1 are caller-saved, so preserve
	// a1, t0, t1 and ra across the call.
	addi	sp, sp, -16
	sw	a1, 0(sp)
	sw	t0, 4(sp)
	sw	t1, 8(sp)
	sw	ra, 12(sp)
	mv	a0, a1
	lw	t2, Disable(a1)
	jalr	t2
	lw	a1, 0(sp)
	lw	t0, 4(sp)
	lw	t1, 8(sp)
	lw	ra, 12(sp)
	addi	sp, sp, 16

	// Swap lower stack boundary: task->tc_SPLower <-> newStack->stk_Lower
	lw	t2, tc_SPLower(t1)
	lw	t3, stk_Lower(t0)
	sw	t3, tc_SPLower(t1)
	sw	t2, stk_Lower(t0)

	// Swap upper stack boundary
	lw	t2, tc_SPUpper(t1)
	lw	t3, stk_Upper(t0)
	sw	t3, tc_SPUpper(t1)
	sw	t2, stk_Upper(t0)

	// Swap the stack pointer itself (nothing live on the stack here)
	mv	t2, sp
	lw	t3, stk_Pointer(t0)
	mv	sp, t3
	sw	t2, stk_Pointer(t0)

	// Enable() - running on the new stack now. Preserve a1 and ra.
	addi	sp, sp, -16
	sw	a1, 0(sp)
	sw	ra, 4(sp)
	mv	a0, a1
	lw	t2, Enable(a1)
	jalr	t2
	lw	a1, 0(sp)
	lw	ra, 4(sp)
	addi	sp, sp, 16

	ret
