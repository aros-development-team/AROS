/*
     (C) 1995-96 AROS - The Amiga Replacement OS
     $Id$
 
     Desc:
     Lang:
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(UWORD, GetCC,
 
    LOCATION
 	struct ExecBase *, SysBase, 88, Exec)
 
    FUNCTION
 	Read the contents of the sr in a easy and compatible way.
 
    INPUTS
 
    RESULT
 	The contents of sr as a UWORD.
 
    NOTES
 	This function will most likely be implemented by a few instructions
 	directly in the jumptable.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	SetSR()
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(GetCC,Exec)
	.type	AROS_SLIB_ENTRY(GetCC,Exec),@function

	/* This function is implemented directly in the jumptable - but it
	   does not harm to see what it looks like.
	   68000 version */
AROS_SLIB_ENTRY(GetCC,Exec):
	move.w	sr,d0
	rts

	/* 68010 and up */
	.globl	_Exec_GetCC_01
AROS_SLIB_ENTRY(GetCC_01,Exec):
	move.w	ccr,d0
	rts

