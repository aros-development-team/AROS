/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
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

/*
	XDEF AROS_SLIB_ENTRY(GetCC,Exec)	68000  version
	XDEF AROS_SLIB_ENTRY(GetCC_10,Exec)	68010+ version
*/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(GetCC,Exec)
	.type	AROS_SLIB_ENTRY(GetCC,Exec),@function

	/* This function is implemented directly in the jumptable - but it
	   does no harm to see what it looks like.
	   68000 version */
AROS_SLIB_ENTRY(GetCC,Exec):
	move.w	sr,d0
	rts

	/* 68010 and up */
	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(GetCC_10,Exec)
	.type   AROS_SLIB_ENTRY(GetCC_10,Exec),@function
AROS_SLIB_ENTRY(GetCC_10,Exec):
	move.w	ccr,d0
	rts

