	Switch	    =	-0x24
	IDNestCnt   =	0x126
	TDNestCnt   =	0x127
	AttnResched =	0x12a

	.globl	_Exec_Enable
_Exec_Enable:
	| decrement nesting count and return if there are Disable()s left
	subqb	#1,a6@(IDNestCnt)
	jpl	end

	| return if there are no delayed switches pending.
	tstb	a6@(AttnResched+1)
	jpl	end

	| if TDNestCnt is not -1 taskswitches are still forbidden
	tstb	a6@(TDNestCnt)
	jpl	end

	| Unset delayed switch bit and do the delayed switch
	bclr	#7,a6@(AttnResched+1)
	jsr	a6@(Switch)

	| all done.
end:	rts

