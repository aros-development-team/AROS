*** ScR ***

* This is a quick & dirty example of how to read data from
* a sound card. A real program would of course not mess with
* the hardware registers.
* Press Fire on joystick to exit. Effect will only show on
* a native amiga video mode (obviously). 68020+ only.
* Updated (but untested, unfortunately) for beta release 2.


	incdir	include:
	include	lvo/exec_lib.i
	include	dos/dos.i
	include	lvo/dos_lib.i
	include	devices/ahi.i
	include	lvo/ahi_lib.i

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

push	MACRO
	move.l	\1,-(sp)
	ENDM

pop	MACRO
	move.l	(sp)+,\1
	ENDM

pushm	MACRO
	movem.l	\1,-(sp)
	ENDM

popm	MACRO
	movem.l	(sp)+,\1
	ENDM

BOOST=3

start
	base	exec
	moveq	#-1,d0
	moveq	#0,d1
	call	AllocMem		;flushlibs

	moveq	#0,d0
	lea	dosname(pc),a1
	call	OpenLibrary
	move.l	d0,dosbase
	beq.w	nodos
	OPENAHI	1
	move.l	d0,ahibase
	beq.w	.noahi
	move.l	d0,a6

	lea	tags(pc),a1
	call	AHI_AllocAudioA
	move.l	d0,ctrlblock
	beq.w	.noctrlblock

	move.l	ctrlblock(pc),a2

	lea	ctrltags(pc),a1
	call	AHI_ControlAudioA
	tst.l	d0
	bne	.exit

	move.w	#$80,$dff096			;turn off copper...
	move.w	#$1000,$dff100			;1 bpl...

.l
	base	dos
	moveq	#1,d1
	call	Delay
	btst	#7,$bfe001
	bne.b	.l

	move.w	#$8080,$dff096			;turn on copper

.exit
	base	ahi
	move.l	ctrlblock(pc),a2
	call	AHI_FreeAudio

.noctrlblock
.noahi
	CLOSEAHI
	base	exec
	move.l	dosbase(pc),a1
	call	CloseLibrary
nodos
	rts

RecordFunc:
	blk.b	MLN_SIZE
	dc.l	VU
	dc.l	0

;in:
* A0	(struct Hook *)
* A2	(struct AHIAudioCtrl *)
* A1	(struct AHIRecordMessage *)

VU:
	pushm	d2-d3

	cmp.l	#AHIST_S16S,ahirm_Type(a1)
	bne.b	.exit				;unknown buffer type

	move.l	ahirm_Length(a1),d0
	move.l	d0,d3
	lsl.l	#1,d0
	move.l	ahirm_Buffer(a1),a0
	moveq	#0,d1
.vuloop
	moveq	#0,d2
	move.w	(a0)+,d2
	bpl.b	.pos
	neg.w	d2
.pos
	add.l	d2,d1
	sub.l	#1,d0
	bne.b	.vuloop

	mulu.w	#32768/16/BOOST,d3
	divu.l	d3,d1
	cmp.w	#15,d1
	bmi.b	.inrange
	move.w	#$f00,d1
.inrange
	move.w	d1,$dff180
	move.w	d1,$dff182
.exit
	moveq	#0,d0				;VERY IMPORTANT!
	popm	d2-d3
	rts

tags
	dc.l	AHIA_MixFreq,3000
	dc.l	AHIA_AudioID,$0002000D
	dc.l	AHIA_Channels,1
	dc.l	AHIA_Sounds,1
	dc.l	AHIA_RecordFunc,RecordFunc
	dc.l	TAG_DONE

ctrltags
	dc.l	AHIC_MonitorVolume,$10000		; 1.0, full volume
	dc.l	AHIC_Record,TRUE
	dc.l	TAG_DONE

ctrlblock	dc.l	0
ahibase		dc.l	0
dosbase		dc.l	0
ahiname		AHINAME
dosname		DOSNAME

