/*
     (C) 1995-96 AROS - The Amiga Replacement OS
     $Id$
 
     Desc:
     Lang:
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(void, CacheClearU,
 
    LOCATION
 	struct ExecBase *, SysBase, 106, Exec)
 
    FUNCTION
 	Flushes the contents of all CPU chaches in a simple way.
 
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
	.globl	AROS_SLIB_ENTRY(CacheClearU,Exec)
	.type	AROS_SLIB_ENTRY(CacheClearU,Exec),@function
AROS_SLIB_ENTRY(CacheClearU,Exec):
	/* Simple 68000s have no chaches */
	rts

	/* Is this the same routine for 20? */
	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(CacheClearU_30,Exec)
	.type	AROS_SLIB_ENTRY(CacheClearU_30,Exec),@function
AROS_SLIB_ENTRY(CacheClearU_30,Exec):
	/* Do the real work in supervisor mode
	   Preserve a5 in a1 (faster than stack space) */
	move.l	a5,a1
	lea.l	cacheclearusup,a5
	jsr	Supervisor(a6)
	move.l	a1,a5
	rts

cacheclearusup:
	/* Set CD and CI bit in cacr */
	movec	cacr,d0
	or.w	#0x0808,d0
	movec	d0,cacr
	rte

