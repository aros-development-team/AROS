	IFND MACROS_I
MACROS_I	SET	1

TRUE	equ	1
FALSE	equ	0
NULL	equ	0

call	MACRO
	jsr	_LVO\1(a6)
	ENDM


skipw	MACRO
	IFNE	NARG
	FAIL	!!! NO ARGUMENTS ALLOWED !!!
	ENDC

	dc.w	$0c40					;CMPI.W #????,d0
	ENDM

skipl	MACRO
	IFNE	NARG
	FAIL	!!! NO ARGUMENTS ALLOWED !!!
	ENDC

	dc.w	$0c80					;CMPI.L #????????,d0
	ENDM


base	MACRO
	IFC	'\1','exec'
		move.l	4.w,a6
	ELSE
		move.l	\1base(pc),a6
	ENDC
	ENDM

abase	MACRO
	IFC	'\1','exec'
		move.l	4.w,a6
	ELSE
		move.l	\1base,a6
	ENDC
	ENDM


push	MACRO
	move.l	\1,-(sp)
	ENDM

pop	MACRO
	move.l	(sp)+,\1
	ENDM

pushm	MACRO
	IFC	'\1','std'
		movem.l	d2-d7/a2-a6,-(sp)
	ELSE
		movem.l	\1,-(sp)
	ENDC
	ENDM

popm	MACRO
	IFC	'\1','std'
		movem.l	(sp)+,d2-d7/a2-a6
	ELSE
		movem.l	(sp)+,\1
	ENDC
	ENDM

mpush	MACRO
	pushm	\1
	ENDM

mpop	MACRO
	popm	\1
	ENDM


flash	MACRO
	push	d0
	moveq	#-1,d0
.loop\@
	move.l	d0,$dff180
	dbf	d0,.loop\@
	pop	d0
	ENDM
	
wait	MACRO
.loop\@
	btst	#7,$bfe001
	bne.b	.loop\@
	ENDM

	ENDC ; MACROS_I
