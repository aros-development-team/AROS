/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the stack of a task.
    Lang: english
*/

/******************************************************************************

    NAME
        AROS_LH1(void, StackSwap,

    SYNOPSIS
	AROS_LHA(struct StackSwapStruct *, newStack, A0),

    LOCATION
        struct ExecBase *, SysBase, 122, Exec)

    FUNCTION
        Change the stack of a task.
	
    INPUTS
	sss - The description of the new stack
	
    RESULT
	There will be a new stack.

    NOTES
	Calling this routine the first time will change sss and
	calling it a second time, the changes will be undone.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	This is a symmetrical routine. If you call it twice, then
	everything will be as it was before.

    HISTORY

******************************************************************************/

        #include "machine.i"

        .text
        .balign 16
        .globl  AROS_SLIB_ENTRY(StackSwap,Exec)
        .type   AROS_SLIB_ENTRY(StackSwap,Exec),@function

	/* The stack looks like this:

	    8 SysBase
	    4 sss
	    0 return address
	*/

#define sss	4
#define SysBase	8

AROS_SLIB_ENTRY(StackSwap,Exec):
#if !UseRegisterArgs
	/* Read parameter sss */
	move.l sss(%sp),%a0
#endif

	/* copy new SP into a1 */
	move.l stk_Pointer(%a0),%a1

	/* Pop return address and sss from the current stack and copy them
	    onto the one specified in sss */
	move.l (%sp)+,%d0	    /* pop Return address */
#if !UseRegisterArgs
	move.l %d0,-12(%a1)         /* Push return address on new stack */
	move.l (%sp)+,-8(%a1)	    /* pop sss and push on new stack */
#else
	move.l %d0,-(%a1)	    /* Push return address on new stack */
#endif

#if !UseRegisterArgs
	/* Copy SysBase from the current stack onto the one in sss */
	move.l (%sp),%a6
	move.l %a6,-4(%a1)          /* Push SysBase on new stack */

	/* Calc new start of stack in sss */
	add.l #-12,%a1
#endif

	/* Call Disable() (SysBase is still on the stack) */
	jsr Disable(%a6)
#if !UseRegisterArgs
	move.l (%sp)+,%a6	    /* Remove SysBase from current stack */
#endif

	move.l %sp,stk_Pointer(%a0) /* Save current SP in sss */
	movel %a1,%sp		    /* Load the new stack */

	move.l ThisTask(%a6),%a1
	lea.l tc_SPLower(%a1),%a1   /* a1 = &SysBase->ThisTask->tc_SPLower */

	/* Swap ThisTask->tc_SPLower and sss->stk_Lower */
	move.l stk_Lower(%a0),%d0
	move.l (%a1),%d1
	move.l %d0,(%a1)
	move.l %d1,stk_Lower(%a0)

	/* Swap tc_SPUpper and sss->stk_Upper, too */
	move.l stk_Upper(%a0),%d0
	move.l 4(%a1),%d1
	move.l %d0,4(%a1)
	move.l %d1,stk_Upper(%a0)

	/* Call Enable() */
#if !UseRegisterArgs
	move.l SysBase(%sp),%a6
	move.l %a6,-(%sp)	    /* push SysBase on new stack */
#endif
	jsr Enable(%a6)	            /* call enable */
#if !UseRegisterArgs
	addq.w #4,%sp		    /* Clean stack */
#endif

	/* Note that at this time, the new stack from sss contains the
	   same values as the previous stack */
	rts
