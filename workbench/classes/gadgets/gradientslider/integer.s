
	machine 68020

	xdef	___mulsi3

___mulsi3:
	move.l	4(sp),d0
	muls.l	8(sp),d0	
	rts

	xdef	___divsi3
			
___divsi3:
	move.l	4(sp),d0
	divs.l	8(sp),d0
	rts

	xdef	___udivsi3

___udivsi3:
	move.l	4(sp),d0
	divu.l	8(sp),d0
	rts

	END
