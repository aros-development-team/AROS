/*
     (C) 1995-96 AROS - The Amiga Research OS
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
 	Flushes the contents of all CPU caches in a simple way.
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 	Left out until I decide about PPC memory model
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheClearU,Exec)
	.type	AROS_SLIB_ENTRY(CacheClearU,Exec),@function
AROS_SLIB_ENTRY(CacheClearU,Exec):
	subr
	rts
