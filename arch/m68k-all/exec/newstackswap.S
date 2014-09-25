/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NewStackSwap() - Call a function with swapped stack.
    Lang: english
*/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(NewStackSwap,Exec,134)
AROS_SLIB_ENTRY(NewStackSwap,Exec,134):

    /* Stackswap will clobber %d0, %d1, %a0, and %a1,
     * so we need to save %a0/%a1 in %a3/%a4
     */
    movem.l     %a3/%a4,%sp@-
    move.l      %a0,%a3
    move.l      %a1,%a4

    jsr         %a6@(StackSwap)

    tst.l       %a2
    beq.b       nopushargs
    move.l      %a2@(7*4),%sp@- // Put the C arguments on the stack
    move.l      %a2@(6*4),%sp@-
    move.l      %a2@(5*4),%sp@-
    move.l      %a2@(4*4),%sp@-
    move.l      %a2@(3*4),%sp@-
    move.l      %a2@(2*4),%sp@-
    move.l      %a2@(1*4),%sp@-
    move.l      %a2@(0*4),%sp@-

nopushargs:
    jsr         %a4@            // Call the C function

    tst.l       %a2
    beq.b       nopopargs

    lea.l       %sp@(8*4),%sp   // Remove the C arguments

nopopargs:
    move.l      %d0,%a4         // save C function returncode
    move.l      %a3,%a0
    jsr         %a6@(StackSwap)
    move.l      %a4,%d0

    /* Now we can restore %a3/%a4
     */
    movem.l     %sp@+,%a3/%a4
    rts
