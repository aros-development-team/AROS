This is Supervisor function. Between lines there is exception handler part.

	push	scr
	mflr	scr	/* save lr, so we can move it to srr0 later */
	push	scr
	/* try to cause a trap */
_Supervisor_trp:
	mfmsr	r0
exception caused, so:
------------------------------------------------------------------------
exception handler
	mfsrr0	scr
	/* was it called from Supervisor function? (pseudocode) */
	cmp	scr,_Supervisor_trp	/* supervisor */
	beq	ok
	/* was it called from Superstate function? (pseudocode) */
	cmp	scr,_Superstate_trp	/* superstate */
	beq	ok
	.
	.
	.
ok:
	/* fetch the instruction that is after the one causing exception */
	addi	scr,scr,4
	mtlr	scr
	blr
exception handler
------------------------------------------------------------------------
next line of Supervisor  function
	pop	scr	/* pop lr */
	mtsrr0	scr
	pop	scr
	/* Jump to user procedure. It will return by rfi using value from lr */
 	ljmp	arg0
	/* user procedure returns by the means of rfi, so no rts */