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
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	SetSR()
 
    INTERNALS
	I don't think this function will have any real use on PPC...
	This function maps PPC:
		CA to C
		OV to V
		EQ to Z
		?? to N
		CA to X
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(GetCC,Exec)
	.type	AROS_SLIB_ENTRY(GetCC,Exec),@function

AROS_SLIB_ENTRY(GetCC,Exec):
	subr
	push	scr
	li	ret,0
	bne	_notequal	# Z=0
	ori	ret,ret,4	# Z=1
_notequal:
	bge	_positive	# N=0
	ori	ret,ret,8	# N=1
_positive:
	mfxer	scr
	rlwinm.	scr,scr,1,0,0
	beq	_noovfl		# 0? so no Overflow
	ori	ret,ret,2	# V=1
_noovfl:
	mfxer	scr
	rlwinm.	scr,scr,2,0,0
	beq	_nocarry	# 0? so no Carry	
	ori	ret,ret,17	# X=1 C=1
_nocarry:	
	pop	scr
	rts
