/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stubs to call C functions while preserving all registers, 64bit RISC-V version
    Lang: english

    The library entry (e.g. Exec_20_Disable) saves every caller-saved
    integer register, calls the real C implementation (_Exec_20_Disable),
    then restores and returns - so Disable()/Enable()/Forbid()/Permit()
    and the semaphore fast paths stay register-transparent to their
    callers, as on the other arches. The LP64D caller-saved integer set is
    ra, t0-t6 and a0-a7 (the C impl preserves s0-s11/sp). As on AArch64,
    caller-saved FP registers are not preserved here.
*/

#include "aros/riscv64/asm.h"

#define STUB(cname,name)                          \
	.globl	cname                           ; \
	.type	cname, %function                ; \
cname:                                          ; \
	addi	sp, sp, -128                    ; \
	sd	ra, 0(sp)                       ; \
	sd	t0, 8(sp)                       ; \
	sd	t1, 16(sp)                      ; \
	sd	t2, 24(sp)                      ; \
	sd	t3, 32(sp)                      ; \
	sd	t4, 40(sp)                      ; \
	sd	t5, 48(sp)                      ; \
	sd	t6, 56(sp)                      ; \
	sd	a0, 64(sp)                      ; \
	sd	a1, 72(sp)                      ; \
	sd	a2, 80(sp)                      ; \
	sd	a3, 88(sp)                      ; \
	sd	a4, 96(sp)                      ; \
	sd	a5, 104(sp)                     ; \
	sd	a6, 112(sp)                     ; \
	sd	a7, 120(sp)                     ; \
	call	name                            ; \
	ld	a7, 120(sp)                     ; \
	ld	a6, 112(sp)                     ; \
	ld	a5, 104(sp)                     ; \
	ld	a4, 96(sp)                      ; \
	ld	a3, 88(sp)                      ; \
	ld	a2, 80(sp)                      ; \
	ld	a1, 72(sp)                      ; \
	ld	a0, 64(sp)                      ; \
	ld	t6, 56(sp)                      ; \
	ld	t5, 48(sp)                      ; \
	ld	t4, 40(sp)                      ; \
	ld	t3, 32(sp)                      ; \
	ld	t2, 24(sp)                      ; \
	ld	t1, 16(sp)                      ; \
	ld	t0, 8(sp)                       ; \
	ld	ra, 0(sp)                       ; \
	addi	sp, sp, 128                     ; \
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
