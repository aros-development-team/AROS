/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stubs to call C functions while preserving all registers, RISC-V version
    Lang: english

    The library entry (e.g. Exec_20_Disable) saves every caller-saved
    integer register, calls the real C implementation (_Exec_20_Disable),
    then restores and returns - so Disable()/Enable()/Forbid()/Permit()
    and the semaphore fast paths stay register-transparent to their
    callers, as on the other arches. The caller-saved integer set (ILP32D
    and LP64D alike) is ra, t0-t6 and a0-a7 (the C impl preserves
    s0-s11/sp); each takes an SZREG slot. As on AArch64, caller-saved FP
    registers are not preserved here.
*/

#include "aros/riscv/xlen.h"

#define STUB(cname,name)                          \
	.globl	cname                           ; \
	.type	cname, %function                ; \
cname:                                          ; \
	addi	sp, sp, -16*SZREG               ; \
	REG_S	ra, 0(sp)                       ; \
	REG_S	t0, 1*SZREG(sp)                 ; \
	REG_S	t1, 2*SZREG(sp)                 ; \
	REG_S	t2, 3*SZREG(sp)                 ; \
	REG_S	t3, 4*SZREG(sp)                 ; \
	REG_S	t4, 5*SZREG(sp)                 ; \
	REG_S	t5, 6*SZREG(sp)                 ; \
	REG_S	t6, 7*SZREG(sp)                 ; \
	REG_S	a0, 8*SZREG(sp)                 ; \
	REG_S	a1, 9*SZREG(sp)                 ; \
	REG_S	a2, 10*SZREG(sp)                ; \
	REG_S	a3, 11*SZREG(sp)                ; \
	REG_S	a4, 12*SZREG(sp)                ; \
	REG_S	a5, 13*SZREG(sp)                ; \
	REG_S	a6, 14*SZREG(sp)                ; \
	REG_S	a7, 15*SZREG(sp)                ; \
	call	name                            ; \
	REG_L	a7, 15*SZREG(sp)                ; \
	REG_L	a6, 14*SZREG(sp)                ; \
	REG_L	a5, 13*SZREG(sp)                ; \
	REG_L	a4, 12*SZREG(sp)                ; \
	REG_L	a3, 11*SZREG(sp)                ; \
	REG_L	a2, 10*SZREG(sp)                ; \
	REG_L	a1, 9*SZREG(sp)                 ; \
	REG_L	a0, 8*SZREG(sp)                 ; \
	REG_L	t6, 7*SZREG(sp)                 ; \
	REG_L	t5, 6*SZREG(sp)                 ; \
	REG_L	t4, 5*SZREG(sp)                 ; \
	REG_L	t3, 4*SZREG(sp)                 ; \
	REG_L	t2, 3*SZREG(sp)                 ; \
	REG_L	t1, 2*SZREG(sp)                 ; \
	REG_L	t0, 1*SZREG(sp)                 ; \
	REG_L	ra, 0(sp)                       ; \
	addi	sp, sp, 16*SZREG                ; \
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
