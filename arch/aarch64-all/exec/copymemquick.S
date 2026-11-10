/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CopyMemQuick() - AArch64 version, same as CopyMem().
*/

	#include <aros/aarch64/asm.h>

	.text
	_ALIGNMENT
	.globl	AROS_SLIB_ENTRY(CopyMemQuick,Exec,105)
	_FUNCTION(AROS_SLIB_ENTRY(CopyMemQuick,Exec,105))

AROS_SLIB_ENTRY(CopyMemQuick,Exec,105):

	b	AROS_SLIB_ENTRY(CopyMem,Exec,104)
