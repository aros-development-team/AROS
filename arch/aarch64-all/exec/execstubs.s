/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Stubs to call C functions while preserving all registers, AArch64 version.
    Lang: english

    The library entry (e.g. Exec_20_Disable) saves every caller-saved register,
    tail-calls the real C implementation (_Exec_20_Disable), then restores and
    returns -- so Disable()/Enable()/Forbid()/Permit() and the semaphore fast
    paths stay register-transparent to their callers, as on the other arches.
    AAPCS64 caller-saved set is x0-x18 + x30 (the C impl preserves x19-x28/fp/sp).
*/

	#include "aros/aarch64/asm.h"

#define STUB(cname,name)                          \
	.globl	cname                           ; \
	.type	cname, %function                ; \
cname:                                          ; \
	stp	x0,  x1,  [sp, #-160]!          ; \
	stp	x2,  x3,  [sp, #16]             ; \
	stp	x4,  x5,  [sp, #32]             ; \
	stp	x6,  x7,  [sp, #48]             ; \
	stp	x8,  x9,  [sp, #64]             ; \
	stp	x10, x11, [sp, #80]             ; \
	stp	x12, x13, [sp, #96]             ; \
	stp	x14, x15, [sp, #112]           ; \
	stp	x16, x17, [sp, #128]           ; \
	stp	x18, x30, [sp, #144]           ; \
	ldr	x16, 1f                        ; \
	blr	x16                            ; \
	ldp	x18, x30, [sp, #144]           ; \
	ldp	x16, x17, [sp, #128]           ; \
	ldp	x14, x15, [sp, #112]           ; \
	ldp	x12, x13, [sp, #96]            ; \
	ldp	x10, x11, [sp, #80]            ; \
	ldp	x8,  x9,  [sp, #64]            ; \
	ldp	x6,  x7,  [sp, #48]            ; \
	ldp	x4,  x5,  [sp, #32]            ; \
	ldp	x2,  x3,  [sp, #16]            ; \
	ldp	x0,  x1,  [sp], #160           ; \
	ret                                    ; \
1:	.quad	name

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
