/******************************************************************************
|
|   NAME
|
|	__AROS_LH2(ULONG, SetSR,
|
|   SYNOPSIS
|	__AROS_LA(ULONG, newSR, D0),
|	__AROS_LA(ULONG, mask,  D1),
|
|   LOCATION
|	struct ExecBase *, SysBase, 24, Exec)
|
|   FUNCTION
|	Read/Modify the CPU status register in an easy way. Only the bits set in
|	the mask parameter will be changed.
|
|   INPUTS
|	newSR - New contents of sr.
|	mask  - Bits to change.
|
|   RESULT
|	Old contents of sr.
|
|   NOTES
|
|   EXAMPLE
|
|   BUGS
|
|   SEE ALSO
|
|   INTERNALS
|
|   HISTORY
|
******************************************************************************/

	.text
	.align	16
	.globl	Exec_SetSR
	.type	Exec_SetSR,@function
Exec_SetSR:
	/* Dummy */
	ret

