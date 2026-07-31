/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stubs to call C functions while preserving all registers, RISC-V version
    Lang: english

    The library entry (e.g. Exec_20_Disable) saves every caller-saved
    integer register, calls the real C implementation (_Exec_20_Disable),
    then restores and returns - so Disable()/Enable()/Forbid()/Permit()
    and the semaphore fast paths stay register-transparent to their
    callers, as on the other arches. The ILP32D caller-saved integer set
    is ra, t0-t6 and a0-a7 (the C impl preserves s0-s11/sp). As on
    AArch64, caller-saved FP registers are not preserved here.
*/

#include "aros/riscv/asm.h"

#define STUB(cname,name)                          \
	.globl	cname                           ; \
	.type	cname, %function                ; \
cname:                                          ; \
	addi	sp, sp, -64                     ; \
	sw	ra, 0(sp)                       ; \
	sw	t0, 4(sp)                       ; \
	sw	t1, 8(sp)                       ; \
	sw	t2, 12(sp)                      ; \
	sw	t3, 16(sp)                      ; \
	sw	t4, 20(sp)                      ; \
	sw	t5, 24(sp)                      ; \
	sw	t6, 28(sp)                      ; \
	sw	a0, 32(sp)                      ; \
	sw	a1, 36(sp)                      ; \
	sw	a2, 40(sp)                      ; \
	sw	a3, 44(sp)                      ; \
	sw	a4, 48(sp)                      ; \
	sw	a5, 52(sp)                      ; \
	sw	a6, 56(sp)                      ; \
	sw	a7, 60(sp)                      ; \
	call	name                            ; \
	lw	a7, 60(sp)                      ; \
	lw	a6, 56(sp)                      ; \
	lw	a5, 52(sp)                      ; \
	lw	a4, 48(sp)                      ; \
	lw	a3, 44(sp)                      ; \
	lw	a2, 40(sp)                      ; \
	lw	a1, 36(sp)                      ; \
	lw	a0, 32(sp)                      ; \
	lw	t6, 28(sp)                      ; \
	lw	t5, 24(sp)                      ; \
	lw	t4, 20(sp)                      ; \
	lw	t3, 16(sp)                      ; \
	lw	t2, 12(sp)                      ; \
	lw	t1, 8(sp)                       ; \
	lw	t0, 4(sp)                       ; \
	lw	ra, 0(sp)                       ; \
	addi	sp, sp, 64                      ; \
	ret

	.text

	/* Call functions and preserve registers */
#ifdef  UseExecstubs
	STUB(AROS_SLIB_ENTRY(Disable,Exec,20),AROS_CSYMNAME(_Exec_20_Disable))
	STUB(AROS_SLIB_ENTRY(Enable,Exec,21),AROS_CSYMNAME(_Exec_21_Enable))
	STUB(AROS_SLIB_ENTRY(Forbid,Exec,22),AROS_CSYMNAME(_Exec_22_Forbid))
	STUB(AROS_SLIB_ENTRY(Permit,Exec,23),AROS_CSYMNAME(_Exec_23_Permit))

	STUB(AROS_SLIB_ENTRY(ObtainSemaphore,Exec,94),AROS_CSYMNAME(_Exec_94_ObtainSemaphore))
	STUB(AROS_SLIB_ENTRY(ReleaseSemaphore,Exec,95),AROS_CSYMNAME(_Exec_95_ReleaseSemaphore))
	STUB(AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec,113),AROS_CSYMNAME(_Exec_113_ObtainSemaphoreShared))
#endif
