	| The following functions are guaranteed to preserve
	| all registers. But I don't want to write them completely
	| in assembly - C is generally more readable.
	| So I use those stubs to preserve the registers.

	.globl	_Exec__ObtainSemaphore
_Exec__ObtainSemaphore:
	moveml	d0/d1/a0/a1,sp@-
	jbsr	_Exec_ObtainSemaphore
	moveml	sp@+,d0/d1/a0/a1
	rts

	.globl	_Exec__ReleaseSemaphore
_Exec__ReleaseSemaphore:
	moveml	d0/d1/a0/a1,sp@-
	jbsr	_Exec_ReleaseSemaphore
	moveml	sp@+,d0/d1/a0/a1
	rts

	.globl	_Exec__ObtainSemaphoreShared
_Exec__ObtainSemaphoreShared:
	moveml	d0/d1/a0/a1,sp@-
	jbsr	_Exec_ObtainSemaphoreShared
	moveml	sp@+,d0/d1/a0/a1
	rts

