|*****************************************************************************
|
|   NAME
|
|	__AROS_LH0(void, CacheClearU,
|
|   LOCATION
|	struct ExecBase *, SysBase, 106, Exec)
|
|   FUNCTION
|	Flushes the contents of all CPU chaches in a simple way.
|
|   INPUTS
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
|
|   INTERNALS
|
|   HISTORY
|
|******************************************************************************

	Supervisor  =	-0x1e

	| Simple 68000s have no chaches
	.globl	_Exec_CacheClearU
_Exec_CacheClearU:
	rts

	| Is this the same routine for 20?
	.globl	_Exec_CacheClearU_30
_Exec_CacheClearU_30:
	| Do the real work in supervisor mode
	| Preserve a5 in a1 (faster than stack space)
	movel	a5,a1
	leal	cacheclearusup,a5
	jsr	a6@(Supervisor)
	movel	a1,a5
	rts

cacheclearusup:
	| Set CD and CI bit in cacr
	movec	cacr,d0
	orw	#0x0808,d0
	movec	d0,cacr
	rte

