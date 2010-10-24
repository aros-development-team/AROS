/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Supervisor() - Execute some code in a priviledged environment.
    Lang: english
*/

#include <exec/execbase.h>
#include <exec/alerts.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(ULONG, Supervisor,

/*  SYNOPSIS */
	AROS_LHA(ULONG_FUNC, userFunction, A5),

/*  LOCATION */
	struct ExecBase *, SysBase, 5, Exec)

/*  FUNCTION
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
{
    AROS_LIBFUNC_INIT

    ULONG ret;

    /* If this fails (called via user mode), then
     * the trap code below will get called to
     * finish the job.
     */
    asm volatile ("Exec_Supervisor_Entry:\n"
    		  "or.w #0x2000, %sr\n"
    		  "Exec_Supervisor_Entered:\n");
    if (SysBase->AttnFlags & (AFB_68010 | AFB_68020)) {
    	    asm volatile (
    	    	"move.w #0x0020,%%sp@-\n"
    	    	"pea 0f\n"
    	    	"move.w %%sr,%%sp@-\n"
    	    	"jmp (%1)\n"
    	    	"0:\n"
    	    	: "=r" (ret)
    	    	: "a" (userFunction)
    	    	:);
    } else {
    	    asm volatile (
    	    	"pea 0f\n"
    	    	"move.w %%sr,%%sp@-\n"
    	    	"jmp (%1)\n"
    	    	"0:\n"
    	    	: "=r" (ret)
    	    	: "a" (userFunction)
    	    	:);
    }

    return ret;

    AROS_LIBFUNC_EXIT
} /* Supervisor() */

	/* 68000 privilege violation stack frame:
	 *   ULONG PC
	 *   UWORD STATUS
	 */
	/* Special case - if we were trying to do the
	 * Supervisor() call, then give it permission.
	 *
	 * Both the asmcall and stackcall interface
	 * should work with this code.
	 */
asm (
	".text\n"
	".align 4\n"
	".globl Exec_Supervisor_Trap\n"
	"Exec_Supervisor_Trap:\n"
	"	cmp.l #Exec_Supervisor_Entry, %sp@(2)\n"
	"	bne.s 0f\n"
	"	move.l #Exec_Supervisor_Entered, %sp@(2)\n"
	"	or.w #(1 << 13),%sp@(0)\n"
	"	rte\n"
	"0:\n"
	"	move.l #0x80000008,%d7\n"	// ACPU_PrivErr
	"	jsr	Exec_Alert\n"
	"1:\n"
	"	jmp	1b\n"
	);
