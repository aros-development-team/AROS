	IDNestCnt   =	0x126

	.globl	_Exec_Disable
_Exec_Disable:
	| increment nesting count and return
	addqb	#1,a6@(IDNestCnt)
	rts

