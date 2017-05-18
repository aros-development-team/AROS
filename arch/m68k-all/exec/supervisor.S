/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Supervisor() - Execute some code in a privileged environment.
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
	Supervisor will allow a short privileged instruction sequence to
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
	m68k (native)               Runs the process in supervisor mode.
				    The process must end with an RTE
				    instruction. It should save any
				    registers which is uses.

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
	.globl AROS_SLIB_ENTRY(Supervisor,Exec,5)
	.type	AROS_SLIB_ENTRY(Supervisor,Exec,5),@function
AROS_SLIB_ENTRY(Supervisor,Exec,5):
	/* CRITICAL SECTION - DO NOT USE THE STACK */
Exec_Supervisor_Entry:
	or.w	#0x2000, %sr	// Caused exception on 68000

	/* If we're here, we need to fake a stack frame,
	 * because we were already supervisor.
	 */
	btst	#0, %a6@(AttnFlags+1)	// If 68010, push frame id
	beq.s	0f
	move.w	#0x0020,%sp@-   // push the 68010/20 frame id
0:
	pea	Exec_Supervisor_Exit
	move.w	%sr,%sp@-	// Fix up return mode & flags
	jmp	(%a5)
Exec_Supervisor_Exit:
	/* END CRITICAL SECTION - CALLER'S STACK */
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
	/* 68010+ version */
	.globl Exec_Supervisor_Trap
Exec_Supervisor_Trap:
	/* check if this was issuper call */
	cmp.l #Kernel_Is_Super, %sp@(2)
	beq.s 3f

	cmp.l	#Exec_Supervisor_Entry, %sp@(2)
	bne.s	1f

	/* No need to wait - we're supervisor,
	 * we have the right frame, all we need
	 * now is to fix up the return address
	 * and jump to the user's function in A5
	 */
2:	move.l	#Exec_Supervisor_Exit, %sp@(2)
	jmp	(%a5)
	/* Jump to standard exception handler */
1:	jmp M68KTrapHelper_10

	.globl Exec_Supervisor_Trap_00
Exec_Supervisor_Trap_00:
	/* 68000 version */
	cmp.l	#Exec_Supervisor_Entry, %sp@(2)
	beq.s	2b
	jmp M68KFaultTable_00+8*4

3:
	/* Get SR from stack */
	move.w %sp@,%d0
	/* Skip move.w %sr,d0 in Kernel_Is_Super */
	addq.l #2,%sp@(2)
	rte

