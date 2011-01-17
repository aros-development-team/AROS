| $Id$
|
| Code taken from libnix
|

		.globl	_div
		.globl	_ldiv
		.globl	___modsi3
		.globl	___divsi3

| D1.L = D0.L % D1.L signed

___modsi3:	moveml	sp@(4:W),d0/d1
		jbsr	___divsi4
		movel	d1,d0
		rts

| D0.L = D0.L / D1.L signed

_div:
_ldiv:
___divsi3:	moveml	sp@(4:W),d0/d1
___divsi4:	movel	d3,sp@-
		movel	d2,sp@-
		moveq	#0,d2
		tstl	d0
		bpls	LC5
		negl	d0
		addql	#1,d2
LC5:		movel	d2,d3
		tstl	d1
		bpls	LC4
		negl	d1
		eoriw	#1,d3
LC4:		jbsr	___udivsi4
LC3:		tstw	d2
		beqs	LC2
		negl	d0
LC2:		tstw	d3
		beqs	LC1
		negl	d1
LC1:		movel	sp@+,d2
		movel	sp@+,d3
		rts
