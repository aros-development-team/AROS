|*****************************************************************************
|
|   NAME
|
|	__AROS_LH1(void, StackSwap,
|
|   SYNOPSIS
|	__AROS_LA(struct StackSwapStruct *, newStack, A0),
|
|   LOCATION
|	struct ExecBase *, SysBase, 122, Exec)
|
|   FUNCTION
|	This function switches to the new stack given by the parameters in the
|	stackswapstruct structure. The old stack parameters are returned in
|	the same structure so that the stack can be restored later
|
|   INPUTS
|	newStack - parameters for the new stack
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

	Disable     =	-0x78
	Enable	    =	-0x7e
	ThisTask    =	0x114
	tc_SPLower  =	0x3a

	.globl	_Exec_StackSwap
_Exec_StackSwap:
	| Preserve returnaddress and fix sp
	movel	sp@+,d0

	| Get pointer to tc_SPLower in a1 (tc_SPUpper is next)
	movel	a6@(ThisTask),a1
	leal	a1@(tc_SPLower),a1

	| Just to be sure interrupts always find a good stackframe
	jsr	a6@(Disable)

	| Swap Lower boundaries
	movel	a1@,d1
	movel	a0@,a1@+
	movel	d1,a0@+

	| Swap higher boundaries
	movel	a1@,d1
	movel	a0@,a1@
	movel	d1,a0@+

	| Swap stackpointers
	movel	sp,d1
	movel	a0@,sp
	movel	d1,a0@

	| Reenable interrupts.
	jsr	a6@(Enable)

	| Restore returnaddress and return
	movel	d1,sp@-
	rts

