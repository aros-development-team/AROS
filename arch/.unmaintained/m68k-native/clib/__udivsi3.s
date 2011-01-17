| $Id$
|
| Code taken from libnix
|

		.globl	___umodsi3
		.globl	___udivsi3
		.globl	___udivsi4

| D1.L = D0.L % D1.L unsigned

___umodsi3:	moveml	sp@(4:W),d0/d1
		jbsr	___udivsi4
		movel	d1,d0
		rts

| D0.L = D0.L / D1.L unsigned

___udivsi3:	moveml	sp@(4:W),d0/d1
___udivsi4:	movel	d3,sp@-
		movel	d2,sp@-
		movel	d1,d3
		swap	d1
		tstw	d1
		bnes	LC4
		movew	d0,d2
		clrw	d0
		swap	d0
		divu	d3,d0
		movel	d0,d1
		swap	d0
		movew	d2,d1
		divu	d3,d1
		movew	d1,d0
		clrw	d1
		swap	d1
		jra	LC1
LC4:		movel	d0,d1
		swap	d0
		clrw	d0
		clrw	d1
		swap	d1
		moveq	#16-1,d2
LC3:		addl	d0,d0
		addxl	d1,d1
		cmpl	d1,d3
		bhis	LC2
		subl	d3,d1
		addqw	#1,d0
LC2:		dbra	d2,LC3
LC1:		movel	sp@+,d2
		movel	sp@+,d3
		rts
