/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: StackSwap() - swap the task's stack, RISC-V version.

    Translated from the aarch64-all version. Uses the build-generated
    symbolic offsets from asm.h (Disable, Enable, tc_SPLower, tc_SPUpper,
    stk_Lower, stk_Upper, stk_Pointer, ThisTask) - no magic numbers;
    aros/riscv/xlen.h includes the one for the register width. On a
    non-SMP build FindTask(NULL) simply returns SysBase->ThisTask, so
    ThisTask is read directly; on SMP builds the running task is per
    hart, in the native TLS (riscv-native tls.h). The TLS offsets there
    are LP64 ones, so the SMP path is rv64-only for now.
*/

	#include <aros/config.h>
	#include "aros/riscv/xlen.h"
#if defined(__AROSEXEC_SMP__)
	/* The running task is per hart, in the TLS tp points to
	   (rv64-only for now: TLS_THISTASK_OFFSET is the LP64 offset) */
	#include "tls.h"
#endif

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
#if defined(__AROSEXEC_SMP__)
	REG_L	t1, TLS_THISTASK_OFFSET(tp)	// t1 = current task (this hart's)
#else
	REG_L	t1, ThisTask(a1)	// t1 = current task
#endif

	// Disable() - arg0 = SysBase. t0/t1 are caller-saved, so preserve
	// a1, t0, t1 and ra across the call (4*SZREG keeps sp 16-aligned).
	addi	sp, sp, -4*SZREG
	REG_S	a1, 0(sp)
	REG_S	t0, 1*SZREG(sp)
	REG_S	t1, 2*SZREG(sp)
	REG_S	ra, 3*SZREG(sp)
	mv	a0, a1
	REG_L	t2, Disable(a1)
	jalr	t2
	REG_L	a1, 0(sp)
	REG_L	t0, 1*SZREG(sp)
	REG_L	t1, 2*SZREG(sp)
	REG_L	ra, 3*SZREG(sp)
	addi	sp, sp, 4*SZREG

	// Swap lower stack boundary: task->tc_SPLower <-> newStack->stk_Lower
	REG_L	t2, tc_SPLower(t1)
	REG_L	t3, stk_Lower(t0)
	REG_S	t3, tc_SPLower(t1)
	REG_S	t2, stk_Lower(t0)

	// Swap upper stack boundary
	REG_L	t2, tc_SPUpper(t1)
	REG_L	t3, stk_Upper(t0)
	REG_S	t3, tc_SPUpper(t1)
	REG_S	t2, stk_Upper(t0)

	// Swap the stack pointer itself (nothing live on the stack here)
	mv	t2, sp
	REG_L	t3, stk_Pointer(t0)
	mv	sp, t3
	REG_S	t2, stk_Pointer(t0)

	// Enable() - running on the new stack now. Preserve a1 and ra
	// (two slots fit the 16-byte frame on both widths).
	addi	sp, sp, -16
	REG_S	a1, 0(sp)
	REG_S	ra, 1*SZREG(sp)
	mv	a0, a1
	REG_L	t2, Enable(a1)
	jalr	t2
	REG_L	a1, 0(sp)
	REG_L	ra, 1*SZREG(sp)
	addi	sp, sp, 16

	ret
