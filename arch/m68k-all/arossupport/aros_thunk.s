/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

/******************************************************************************

    NAME

	aros_thunk_relbase

    FUNCTION
	Save the current relbase, and call the function at the LVO
	specified.

    INPUTS

    RESULT
	None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/


#include "aros/m68k/asm.h"

	.text
	.balign 4

	.globl	aros_thunk_libfunc

	// %sp(+12..) - Arguments to called function
	// %sp@(0) - Return address
	// D0 - New base address
	// D1 - LVO offset

aros_thunk_libfunc:
	movem.l	%d0-%d1/%a6,%sp@-

	// Get task's tc_SPLower
	move.l	SysBase, %a6
	sub.l	%a1,%a1
	jsr	%a6@(FindTask)
	move.l	%d0,%a6
	move.l	%a6@(tc_SPLower),%a6

	// Get relstack pointer
	move.l	%a6@,%a0	// %a0 is now relstack pointer

	// A0 - Relstack pointer
	// A6 - Relstack pointer pointer
	// SP - New libbase
	//      Offset of LVO
	//      Old A6
	//      Return address
	//      arguments to LVO to call
aros_thunk_common:
	movem.l	%sp@+,%d0-%d1/%a1
	addq.l	#4,%a0
	move.l	%a1,%a0@+		// Save old A6 off-stack
	move.l	%sp@+,%a0@+		// Save return address off-stack
	move.l	%d0,%a0@		// Save new libbase
	move.l	%a0,%a6@		// Adjust relstack pointer

	add.l	%d0,%d1			// Get LVO address

	move.l	%d1,%a1
	jsr 	%a1@			// Call routine
	movem.l	%d0-%d1,%sp@-		// Save returned values
	clr.l	%d0
	move.l	%a6@,%a0		// %a0 is now relstack pointer again
	movem.l	%a0@(-8),%d0-%d1/%a1	// Get saved values
	subq.l	#8,%a0			// Adjust relstack pointer
	subq.l	#4,%a0			// Adjust relstack pointer
	movem.l	%a0,%a6@		// Save relstack pointer
	move.l	%d0,%a6			// Restore A6
	move.l	%d1,%a1			// A1 is the return address
	movem.l	%sp@+,%d0-%d1		// Restore values
	jmp	%a1@			// Return



	.globl	aros_thunk_rellibfunc

	// %sp(+12..) - Arguments to called function
	// %sp@(0) - Return address
	// D0 - Offset from current libbase to new libbase
	// D1 - LVO index from new libbase

aros_thunk_rellibfunc:
	// Save old A6, D0, and D1
	movem.l	%d0-%d1/%a6,%sp@-

	// Get task's tc_SPLower
	move.l	SysBase, %a6
	sub.l	%a1,%a1
	jsr	%a6@(FindTask)
	move.l	%d0,%a6
	move.l	%a6@(tc_SPLower),%a6

	// Retrieve current libbase into D0
	move.l	%a6@,%a0
	move.l	%a0@,%d0
	
	// Calculate new sysbase and LVO function to call
	add.l	%d0,%sp@		// New libbase

	//  A0 - Relstack pointer
	//  A6 - Relstack pointer pointer
	// @SP - New libbase
	//       Offset of LVO
	//       Old A6
	//       Return address
	//       arguments to LVO to call
	jmp	aros_thunk_common
