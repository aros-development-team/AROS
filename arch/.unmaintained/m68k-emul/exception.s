/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Exec function Exception
    Lang: english
*/

/******************************************************************************

    NAME
        AROS_LH0(void, Exception,

    LOCATION
        struct ExecBase *, SysBase, 11, Exec)

    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

        #include "machine.i"

        .text
        .balign 16
        .globl  AROS_SLIB_ENTRY(Exception,Exec)
        .type   AROS_SLIB_ENTRY(Exception,Exec),@function
AROS_SLIB_ENTRY(Exception,Exec):
	linkw	%fp,#0
	movem.l	%a2/%d2,-(%sp)
	/* First clear task exception bit. */
	move.l	8(%fp),%a0
	move.l	ThisTask(%a0),%a2
	bclr	#TB_EXCEPT,tc_Flags(%a2)

	/* If the exception is raised out of Wait IDNestCnt may be >0 */
	move.b	IDNestCnt(%a0),%d2
	/* Set it to a defined value */
	clr.b	IDNestCnt(%a0)

exloop:	/* Get mask of signals causing the exception */
	move.l	tc_SigExcept(%a2),%d0
	and.l	tc_SigRecvd(%a2),%d0
	beq	excend

	/* Clear bits */
	eor.l	%d0,tc_SigExcept(%a2)
	eor.l	%d0,tc_SigRecvd(%a2)

	/* Raise exception. Enable Interrupts */
	move.l	tc_ExceptData(%a2),%a1
	move.l	%a0,-(%sp)
	jsr	Enable(%a0)
	move.l	%a0,-(%sp)
	move.l	%a1,-(%sp)
	move.l	%d0,-(%sp)
	move.l	tc_ExceptCode(%a2),%a1
	jsr	(%a1)
	move.l	%a0,-(%sp)
	jsr	Disable(%a0)
	add.w	#20,%sp

	/* Re-use returned bits */
	or.l	%d0,tc_SigExcept(%a2)
	bra	exloop

excend:	/* Restore IDNestCnt and return */
	move.b	%d2,IDNestCnt(%a0)
	movem.l	-8(%fp),%a2/%d2
	unlk	%fp
	rts

