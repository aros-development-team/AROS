	.globl	_trapvec
_trapvec:
	cmpl	#8,sp@
	jeq	pv
	movel	_storedtrap,sp@-
	rts
pv:	addqw	#4,sp
	jra	_TrapLevel8

	| The only trap I catch is privilege violation
	.globl	_TrapHandler
_TrapHandler:
	movel	#8,sp@-
	movel	_storedtrap,sp@-
	rts
