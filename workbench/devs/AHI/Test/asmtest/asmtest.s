; Run me to assemble with PhxAss

	IF	0
PhxAss SYMDEBUG LINEDEBUG QUIET OPT NRQBPSMD __test.s
IF NOT WARN
  __test
ENDIF
Quit
	ENDC

	incdir	include:
	include	lvo/exec_lib.i
	include	dos/dos.i
	include	lvo/dos_lib.i
	include	hardware/all.i
	include	exec/io.i
	include	devices/ahi.i
	include	lvo/ahi_lib.i
	include	macros.i

start
	base	exec
	moveq	#-1,d0
	moveq	#0,d1
;	call	AllocMem		;flushlibs

	moveq	#0,d0
	lea	dosname(pc),a1
	call	OpenLibrary
	move.l	d0,dosbase
	beq.w	nodos


	OPENAHI	4
	move.l	d0,ahibase
	beq.w	noahi
	move.l	d0,a6

;	call	AHI_KillAudio
	lea	tags(pc),a1
	call	AHI_AllocAudioA
	move.l	d0,ctrlblock
	beq.w	noctrlblock

	move.l	ctrlblock(pc),a2

	moveq	#0,d0
	moveq	#AHIST_SAMPLE,d1
	lea	Sound0Info,a0
	call	AHI_LoadSound

	moveq	#1,d0
	moveq	#AHIST_SAMPLE,d1
	lea	Sound1Info,a0
	call	AHI_LoadSound

;	moveq	#2,d0
;	move.l	#AHIST_INPUT,d1
;	suba.l	a0,a0
;	call	AHI_LoadSound

	lea	ctrltags(pc),a1
	call	AHI_ControlAudioA

*** S
	moveq	#0,d0		;channel
	moveq	#1,d1		;sound
;	move.l	#samp2_len,d2		;offset
;	move.l	#-samp2_len,d3		;length
	move.l	#0,d2		;offset
	move.l	#0,d3		;length
	moveq	#AHISF_IMM,d4	;flags
	call	AHI_SetSound

	moveq	#0,d0
	move.l	#$10000,d1	;volume
	move.l	#$10000,d2	;panning
	moveq	#AHISF_IMM,d3
	call	AHI_SetVol

	moveq	#0,d0
	move.l	#17640,d1
	moveq	#AHISF_IMM,d2
	call	AHI_SetFreq

	move.l	sp,d7
	move.l	#TAG_DONE,-(sp)
	move.l	#50<<16,-(sp)
	move.l	#AHIA_PlayerFreq,-(sp)
	move.l	#TRUE,-(sp)
	move.l	#AHIC_Play,-(sp)

	move.l	sp,a1
	jsr	_LVOAHI_ControlAudioA(a6)
	move.l	d7,sp

*** L
;	moveq	#1,d0		;channel
;	moveq	#1,d1		;sound
;	moveq	#0,d2		;offset
;	moveq	#0,d3		;length
;	moveq	#AHISF_IMM,d4	;flags
;	call	AHI_SetSound
;
;	moveq	#1,d0
;	move.l	#$10000,d1	;volume
;	move.l	#$c000,d2	;panning
;	moveq	#AHISF_IMM,d3
;	call	AHI_SetVol
;
;	moveq	#1,d0
;	move.l	#17640,d1
;	moveq	#AHISF_IMM,d2
;	call	AHI_SetFreq

*** L Wave
;	lea	mvstruct(pc),a0
;	call	AHI_SetEffect

	lea	mask1struct(pc),a0
	call	AHI_SetEffect

	lea	echostruct(pc),a0
	call	AHI_SetEffect

	lea	mask2struct(pc),a0
	call	AHI_SetEffect

	moveq	#1,d0		;channel
	moveq	#0,d1		;sound
	moveq	#0,d2
	moveq	#0,d3
	moveq	#AHISF_IMM,d4	;flags
	call	AHI_SetSound

	moveq	#1,d0
	move.l	#$10000,d1	;volume
	move.l	#$f000,d2	;panning
	moveq	#AHISF_IMM,d3
	call	AHI_SetVol

	moveq	#1,d0
	move.l	#8000,d1
	moveq	#AHISF_IMM,d2
	call	AHI_SetFreq

	moveq	#1,d0		;channel
	moveq	#AHI_NOSOUND,d1	;sound
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d4		;flags
	call	AHI_SetSound

********** INPUT *************************

;	moveq	#2,d0		;channel
;	moveq	#2,d1		;sound
;	moveq	#0,d2		;offset
;	moveq	#0,d3		;length
;	moveq	#AHISF_IMM,d4	;flags
;	call	AHI_SetSound
;
;	moveq	#2,d0
;	move.l	#$10000,d1	;volume
;	move.l	#$f000,d2	;panning
;	moveq	#AHISF_IMM,d3
;	call	AHI_SetVol
;
;	moveq	#2,d0
;	move.l	#AHI_MIXFREQ,d1
;	moveq	#AHISF_IMM,d2
;	call	AHI_SetFreq

.lp
	base	dos
	moveq	#1,d1
	call	Delay
	btst	#7,$bfe001
	bne.b	.lp
exit:
	base	ahi
	move.l	ctrlblock(pc),a2
	call	AHI_FreeAudio

noctrlblock
noahi
	CLOSEAHI

	base	exec
	move.l	dosbase(pc),a1
	call	CloseLibrary
nodos
	rts

Sound0Info:
	dc.l	AHIST_S8S
	dc.l	samp1
	dc.l	samp1_len
Sound1Info:
	dc.l	AHIST_M16S
	dc.l	samp2
	dc.l	samp2_len

mvstruct
	dc.l	AHIET_MASTERVOLUME
	dc.l	$10000

echostruct
	dc.l	AHIET_DSPECHO			; ahie_Effect
	dc.l	6000				; ahiede_Delay
	dc.l	$8000				; ahiede_Feedback
	dc.l	$8000				; ahiede_Mix
	dc.l	$10000				; ahiede_Cross

mask1struct
	dc.l	AHIET_DSPMASK
	dc.w	4
	dc.b	AHIEDM_DRY, AHIEDM_WET, AHIEDM_DRY, AHIEDM_DRY

mask2struct
	dc.l	AHIET_DSPMASK
	dc.w	4
	dc.b	AHIEDM_WET, AHIEDM_DRY, AHIEDM_DRY, AHIEDM_DRY

tags
	dc.l	AHIA_MixFreq,17640
	dc.l	AHIA_AudioID,$00020018
	dc.l	AHIA_Channels,4
	dc.l	AHIA_Sounds,10
        dc.l	AHIA_PlayerFreq, 50<<16
        dc.l	AHIA_MinPlayerFreq, 20<<16
        dc.l	AHIA_MaxPlayerFreq, 70<<16
	dc.l	TAG_DONE

ctrltags
	dc.l	AHIC_Play,TRUE
	dc.l	TAG_DONE

port		dc.l	0
ior		dc.l	0
ctrlblock	dc.l	0
ahibase		dc.l	0
dosbase		dc.l	0
ahiname		AHINAME
dosname		DOSNAME

	section	samples,data
	incdir	Projekt:ahi/samples/

	dc.b	$7f
samp1
;	DC.B	$0D,$25,$3C,$51,$63,$71,$7A,$7F,$7F,$7A,$71,$63,$51,$3C,$25,$0D
;	DC.B	$F3,$DB,$C4,$AF,$9D,$8F,$86,$81,$81,$86,$8F,$9D,$AF,$C4,$DB,$F3
;	incbin	sine.sb
	incbin	ASS-14.sb
;	incbin	stereo8.sb
samp1_len	EQU	(*-samp1)/2
	dc.b	$80

samp2
;	incbin	cocacola.sw
	incbin	LouiseR.sw
samp2_len	EQU	(*-samp2)/2
	incbin	LouiseR.sw
