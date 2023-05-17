/*
    Copyright © 2023, The AROS Development Team. All rights reserved.
    $Id$
*/

	#include <aros/riscv/asm.h>

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec,122)

AROS_SLIB_ENTRY(StackSwap,Exec,122):

	// A0 contains parameter 'newStack'
    // A1 contains ExecBase
	// RA contains the return address
	ret
