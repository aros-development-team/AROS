
	INCLUDE	"libraries/reqtools.i"
	INCLUDE	"intuition/intuition.i"

	XDEF		_CloseWindowSafely

TDNestCnt			equ	$127

_LVORemove			equ	-$fc
_LVOCloseWindow	equ	-$48
_LVOModifyIDCMP	equ	-$96
_LVOPermit			equ	-$8A
_LVOReplyMsg		equ	-$17A

* window in A0
_CloseWindowSafely:
	movem.l	a0/a2/a5-a6,-(a7)
	move.l	rt_IntuitionBase(a6),a5
	move.l	$4.w,a6
	addq.b	#1,TDNestCnt(a6)
lab2:
	move.l	(a7),a0
	move.l	wd_UserPort(a0),d0
	beq.s		exitclosewinsafely

	movea.l	d0,a2
	move.l	MP_MSGLIST+LH_HEAD(a2),a2

removemsgloop:
	move.l	a2,a1
	move.l	im_ExecMessage+LN_SUCC(a1),d0
	beq.b		clearport
	move.l	d0,a2

	move.l	(a7),a0
	cmp.l		im_IDCMPWindow(a1),a0
	bne.b		nomsgfromourwin

	move.l	a1,-(a7)
	jsr		_LVORemove(a6)
	move.l	(a7)+,a1
	jsr		_LVOReplyMsg(a6)

nomsgfromourwin:
	bra.b		removemsgloop

clearport:
	move.l	(a7),a0
	clr.l		wd_UserPort(a0)
	moveq		#0,d0
	exg.l		a5,a6
	jsr		_LVOModifyIDCMP(a6)
	exg.l		a5,a6

exitclosewinsafely:
	movea.l	(a7),a0
	jsr		_LVOPermit(a6)
	exg.l		a5,a6
	jsr		_LVOCloseWindow(a6)
	movem.l	(a7)+,a0/a2/a5-a6
	rts

