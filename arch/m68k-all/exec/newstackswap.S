/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NewStackSwap() - Call a function with swapped stack.
    Lang: english
*/
/*****************************************************************************

    NAME  
#include <exec/tasks.h>
#include <proto/exec.h>

	AROS_LH3(IPTR, NewStackSwap,

    SYNOPSIS
	AROS_LHA(struct StackSwapStruct *,  sss, A0),
	AROS_LHA(LONG_FUNC, entry, A1),
	AROS_LHA(struct StackSwapArgs *, args, A2),

    LOCATION
	struct ExecBase *, SysBase, 134, Exec)

    FUNCTION
	Calls a function with a new stack.

    INPUTS
	sss     -   A structure containing the values for the upper, lower
		    and current bounds of the stack you wish to use.
	entry	-   Address of the function to call.
	args	-   A structure (actually an array) containing up to 8
		    function arguments. The function is called using C calling
		    convention (no AROS_UHFx macro needed).

    RESULT
	A value actually returned by your function. The function will be
	running on a new stack.

    NOTES

    EXAMPLE

    BUGS
        Do not attempt to pass in a prebuilt stack - it will be erased.

    SEE ALSO
	StackSwap()

    INTERNALS
	This function is be replaced in $(ARCH).

******************************************************************************/
	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(NewStackSwap,Exec,134)
AROS_SLIB_ENTRY(NewStackSwap,Exec,134):
    /* Stackswap will clobber %d0, %d1, %a0, and %a1,
     * so we need to save %a0/%a1 in %a3/%a4
     * also save %d2 to use the altstack top, and
     * %a5 will hold the current Task.
     */
    movem.l     %d2/%a3-%a5,%sp@-
    move.l      %a0,%a3
    move.l      %a1,%a4

    // Get current task pointer, save in A5
    sub.l       %a1,%a1
    jsr         %a6@(FindTask)
    move.l      %d0,%a5

    // Remember old stack top, save in D2
    move.l      %a2,%sp@-
    jsr         aros_get_altstack       /* SP: task */
    addq.l      #4,%sp
    move.l      %d0,%d2    

    move.l      %a3,%a0
    jsr         %a6@(StackSwap)

    // Initialize alternative stack
    move.l      %a5,%sp@-
    jsr         aros_init_altstack      /* SP: task */

    // Set new top to old top */
    move.l      %d2,%sp@
    move.l      %a5,%sp@-
    jsr         aros_set_altstack       /* SP: task, value */

    // Put the C arguments on the stack, overwrite first two
    move.l      %a2@(7*4),%sp@(1*4)
    move.l      %a2@(6*4),%sp@(0*4)
    move.l      %a2@(5*4),%sp@-
    move.l      %a2@(4*4),%sp@-
    move.l      %a2@(3*4),%sp@-
    move.l      %a2@(2*4),%sp@-
    move.l      %a2@(1*4),%sp@-
    move.l      %a2@(0*4),%sp@-

    jsr         %a4@            // Call the C function

    lea.l       %sp@(8*4),%sp   // Remove the C arguments

    move.l      %d0,%a4         // save C function returncode
    move.l      %a3,%a0
    jsr         %a6@(StackSwap)
    move.l      %a4,%d0

    /* Now we can restore %d2/%a3-%a5
     */
    movem.l     %sp@+,%d2/%a3-%a5
    rts
