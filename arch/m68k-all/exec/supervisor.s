/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Supervisor() - Execute some code in a priviledged environment.
    Lang: english
*/
/*****************************************************************************

    NAME
#include <proto/exec.h>

	AROS_LH1(ULONG, Supervisor,

    SYNOPSIS
	AROS_LHA(ULONG_FUNC, userFunction, A5),

    LOCATION
	struct ExecBase *, SysBase, 5, Exec)

    FUNCTION
	Supervisor will allow a short priviledged instruction sequence to
	be called from user mode. This has very few uses, and it is probably
	better to use any system supplied method to do anything.

	The function supplied will be called as if it was a system interrupt,
	notably this means that you must *NOT* make any system calls or
	use any system data structures, and on certain systems you must
	use special methods to leave the code.

	The code will not be passed any parameters.

    INPUTS
	userFunc    -   The address of the code you want called in supervisor
			mode.

    RESULT
	The code will be called.

    NOTES
	This function has completely different effects on different
	processors and architectures.

	Currently defined effects are:

	Kernel                      Effect
	-------------------------------------------------------------------
	i386 (under emulation)      None
	m68k (native)               Runs the process in supervisor mode.
				    The process must end with an RTE
				    instruction. It should save any
				    registers which is uses.
	m68k (under emulation)

    EXAMPLE

    BUGS
	You can very easily make the system unusable with this function.
	In fact it is recommended that you do not use it at all.

    SEE ALSO
	SuperState(), UserState()

    INTERNALS
	You can do what you want with this function, even ignoring it if
	you don't think it makes any sense. But it could be quite useful
	to make it run something under different protection levels.

	You should trust that the programmer know what they are doing :-)

******************************************************************************/
    /* If this fails (called via user mode), then the trap code below
     * will get called to finish the job. Regardless, we can't use
     * anything on the stack, since if we *did* switch from User
     * to Supervisor, our stack swapped!
     */
	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl AROS_SLIB_ENTRY(Supervisor,Exec)
AROS_SLIB_ENTRY(Supervisor,Exec):
#ifndef DoRegisterCalls
	move.l	%a5,%sp@-
	move.l	%sp@(-8),%a5
#endif
	btst	#0, %a6@(AttnFlags)	// If 68010, push frame id
	bne.s	0f
	btst	#1, %a6@(AttnFlags)	// If not 68020, skip
	beq.s	Exec_Supervisor_Entry_10
0:	move.w	#0x0020,%sp@-   // push the 68010/20 frame id

	/* CRITICAL SECTION - DO NOT USE THE STACK */
Exec_Supervisor_Entry_10:
	move.w	%sr, %d0	// Caused exception on 68010/20
Exec_Supervisor_Entry_00:
	or.w	#0x2000, %sr	// Caused exception on 68000
Exec_Supervisor_Entered:	// %d0 will always have the old SR
	/* CRITICAL SECTION - SUPERVISOR STACK */
	pea	1f
	move.w	%d0,%sp@-	// Fix up return mode & flags
	jmp	(%a5)		// D0 will be set after the
1:				//  caller's RTE gets back here.
	/* END CRITICAL SECTION - CALLER'S STACK */
#ifndef DoRegisterCalls
	move.l	%sp@+,%a5
#endif
    	rts

	/* 68000 privilege violation stack frame:
	 *   ULONG PC
	 *   UWORD STATUS
	 *
	 * 68010 privilege violation stack frame:
	 *   UWORD 0x0020
	 *   ULONG PC
	 *   UWORD STATUS
	 */
	/* Special case - if we were trying to do the
	 * Supervisor() call, then give it permission.
	 *
	 * Both the asmcall and stackcall interface
	 * should work with this code.
	 */
	.text
	.align 4
	.globl Exec_Supervisor_Trap
Exec_Supervisor_Trap:
	cmp.l	#Exec_Supervisor_Entry_00, %sp@(2)
	beq.s	0f
	cmp.l	#Exec_Supervisor_Entry_10, %sp@(2)
	bne.s	1f
0:
	move.w	%sr,%d0
	bclr	#13, %d0	// Make it look userish
	move.l	#Exec_Supervisor_Entered, %sp@(2)
	or.w	#(1 << 13),%sp@(0)
	rte
1:
	move.l	#0x80000008,%d7	// ACPU_PrivErr
	move.l	(0x4),%a6
	jsr	%a6@(Alert)
2:
	jmp	2b
