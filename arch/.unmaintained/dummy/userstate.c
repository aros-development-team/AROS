|*****************************************************************************
|
|   NAME
|
|	__AROS_LH1(void, UserState,
|
|   SYNOPSIS
|	__AROS_LA(APTR, sysStack, D0),
|
|   LOCATION
|	struct ExecBase *, SysBase, 26, Exec)
|
|   FUNCTION
|	Return to user mode after a call to SuperState().
|
|   INPUTS
|	sysStack - The returncode from SuperState().
|
|   RESULT
|
|   NOTES
|
|   EXAMPLE
|
|   BUGS
|
|   SEE ALSO
|	SuperState(), Supervisor()
|
|   INTERNALS
|
|   HISTORY
|
|******************************************************************************

	.globl	_Exec_UserState
_Exec_UserState:
	| simply return if argument is NULL
	tstl	d0
	jne	nonzero
	rts
nonzero:
	| Transfer sp
	movel	sp,usp

	| Set old supervisor sp
	movel	d0,sp

	| And return. This jumps directly to a rts.
	rte

