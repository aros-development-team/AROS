;Run me to assemble with PhxAss (sets TEST, too)
	IF	0
PhxAss SYMDEBUG LINEDEBUG OPT NRQBPSMD SET "TEST=1" PT-AHIPlay.s
Quit
	ENDC

;TEST	=1		; Set to 1 to enable small test code
			; The test code in not reentrant!
			; Change MixFreq AudioID module name around line 140.


**************************************************
*    ----- Protracker V2.3A Playroutine -----    *
**************************************************

* Adapted for 'ahi.device' by Martin Blom.
* FilterOnOff and Funk commands are not supported.
* FilterOnOff cannot be be supported.
* Funk can be supported by using AHI_DYNAMICSAMPLE, but shouldn't.
* The player part is fully reentrant.
*
* The audio mode you allocate MUST have these tags:
*	AHIA_Channels		AT LEAST 4 (No. 0-3 are used by the replayer)
*	AHIA_Sounds,1		AT LEAST 1 (No. 0 is used by the replayer)
*	AHIA_MinPlayerFreq	Should be (32*2/5)<<16
*	AHIA_MaxPlayerFreq	Should be (255*2/5)<<16
*
*
* Available functions:
*
*********************************************************************
*	success = mt_init( ptdata );
*	d0                 a2
*
*	Call to initialize module. d0= FALSE if error.
*********************************************************************
*	success = mt_start( ptdata );
*	d0                  a2
*
*	Call to start module. d0= FALSE if error.
*********************************************************************
*	mt_stop( ptdata );
*	         a2
*
*	Call to stop module.
*********************************************************************
*	mt_end( ptdata );
*	        a2
*
*	Call to deallocate resources taken by mt_init.
*********************************************************************
*	mt_music( ptdata );
*	          a2
*	Your PlayerFunc() should call this function.
*********************************************************************



	IFND	TRUE
TRUE	EQU	1
	ENDC
	IFND	FALSE
FALSE	EQU	0
	ENDC
	IFND	NULL
NULL	EQU	0
	ENDC

	incdir	include:
	include	devices/ahi.i
	include	lvo/ahi_lib.i

	include	PT-AHIPlay.i

	IFEQ	TEST

	XDEF	mt_init
	XDEF	mt_start
	XDEF	mt_stop
	XDEF	mt_end
	XDEF	mt_music

	XDEF	_mt_init
	XDEF	_mt_start
	XDEF	_mt_stop
	XDEF	_mt_end
	XDEF	_mt_music

	ELSE ; TEST

	include	lvo/exec_lib.i
	include	lvo/dos_lib.i
	include	utility/hooks.i

main
	lea	ptdata(pc),a2
	move.l	#mt_data,ptd_ModuleAddress(a2)			;Initialize

	OPENAHI	4
	move.l	d0,ptd_AHIBase(a2)				;Initialize
	beq.b	close_ahi
	move.l	d0,a6
	lea	ahi_tags(pc),a1
	jsr	_LVOAHI_AllocAudioA(a6)
	move.l	d0,ptd_AudioCtrl(a2)				;Initialize
	beq.w	close_ahi

	lea	ptdata(pc),a2
	jsr	mt_init
	tst.l	d0
	beq	close_ahi
	lea	ptdata(pc),a2
	jsr	mt_start
	tst.l	d0
	beq	exit

	MOVE.L	4.W,A6
	LEA	DOSname(PC),A1
	MOVEQ	#0,D0
	JSR	_LVOOpenLibrary(A6)
	TST.L	D0
	BEQ.S	exit
	MOVE.L	D0,A6
.1
	MOVEQ	#10,D1
	JSR	_LVODelay(A6)
	BTST	#6,$BFE001
	BNE.S	.1
	BTST	#2,$DFF016
	BNE.S	.1
	MOVE.L	A6,A1
	MOVE.L	4.W,A6
	JSR	_LVOCloseLibrary(A6)

exit
	lea	ptdata(pc),a2
	jsr	mt_end
close_ahi
	lea	ptdata(pc),a2
	move.l	ptd_AHIBase(a2),d0
	beq.b	.1
	move.l	d0,a6
	move.l	ptd_AudioCtrl(a2),a2
	jsr	_LVOAHI_FreeAudio(a6)
.1
	CLOSEAHI
	RTS

DOSname	dc.b "dos.library",0

ptdata	blk.b	PTData_SIZEOF,0

ahi_tags
	dc.l	AHIA_MixFreq,22050
	dc.l	AHIA_Channels,4
	dc.l	AHIA_Sounds,1
	dc.l	AHIA_AudioID,$00020002		; Just an example! No hardcoded values permitted!
	dc.l	AHIA_PlayerFunc,PlayerFunc
	dc.l	AHIA_PlayerFreq,50<<16
	dc.l	AHIA_MinPlayerFreq,(32*2/5)<<16
	dc.l	AHIA_MaxPlayerFreq,(255*2/5)<<16
	dc.l	TAG_DONE

PlayerFunc:
	blk.b	MLN_SIZE
	dc.l	.code
	dc.l	0
	dc.l	ptdata
.code
	move.l	a2,-(sp)
	move.l	h_Data(a0),a2
	jsr	mt_music
	move.l	(sp)+,a2
	rts

mt_data:
	incbin "st-00:Modules/mod/mod.avoidtune"

	ENDC ; TEST






;---- Playroutine ----

	section	ptplayer,code

;in:
* a2	Filled PTData structure
;out:
* d0	TRUE on success
_mt_init
mt_init
	movem.l	d1-d7/a0-a6,-(sp)
	move.l	ptd_ModuleAddress(a2),a0
	move.l	a0,a1
	lea	952(a1),a1
	moveq	#127,d0
	moveq	#0,d1
mtloop
	move.l	d1,d2
	subq.w	#1,d0
mtloop2
	move.b	(a1)+,d1
	cmp.b	d2,d1
	bgt.s	mtloop
	dbra	d0,mtloop2
	addq.b	#1,d2
	asl.l	#8,d2
	asl.l	#2,d2
	add.l	#1084,d2
	move.l	d2,d3
	add.l	ptd_ModuleAddress(a2),d3	;d3 is now start of samples

	lea	ptd_SampleStarts(a2),a1
	moveq	#30,d0
	moveq	#0,d2
mtloop3
	move.l	d2,(a1)+
	moveq	#0,d1
	move.w	42(a0),d1
	asl.l	#1,d1
	add.l	d1,d2
	add.l	#30,a0
	dbra	d0,mtloop3

;d2 is now total sample length
;Build AHISampleInfo strcuture on stack and declare it:
	move.l	d2,-(sp)
	move.l	d3,-(sp)
	move.l	#AHIST_M8S,-(sp)

	move.l	ptd_AHIBase(a2),a6
	move.l	a2,a3
	move.l	ptd_AudioCtrl(a2),a2
	moveq	#0,d0
	moveq	#AHIST_SAMPLE,d1
	move.l	sp,a0
	jsr	_LVOAHI_LoadSound(a6)
	move.l	a3,a2
	lea	3*4(sp),sp
	tst.l	d0
	bne	.error

	move.w	#$0001,n_dmabit+ptd_chan1temp(a2)
	move.w	#$0002,n_dmabit+ptd_chan2temp(a2)
	move.w	#$0004,n_dmabit+ptd_chan3temp(a2)
	move.w	#$0008,n_dmabit+ptd_chan4temp(a2)

	moveq	#TRUE,d0
.exit
	movem.l	(sp)+,d1-d7/a0-a6
	rts
.error
	moveq	#FALSE,d0
	bra	.exit


;in:
* a2	Filled PTData structure
;out:
* d0	TRUE on success
_mt_start
mt_start
	movem.l	d1-d7/a0-a6,-(sp)

	move.b	#6,ptd_speed(a2)
	clr.b	ptd_counter(a2)
	clr.b	ptd_SongPos(a2)
	clr.b	ptd_PBreakPos(a2)
	clr.b	ptd_PosJumpFlag(a2)
	clr.b	ptd_PBreakFlag(a2)
	clr.b	ptd_LowMask(a2)
	clr.b	ptd_PattDelTime(a2)
	clr.b	ptd_PattDelTime2(a2)
	st	ptd_Enable(a2)
	clr.w	ptd_PatternPos(a2)
	clr.w	ptd_DMACONtemp(a2)

	move.l	#TAG_DONE,-(sp)
	move.l	#50<<16,-(sp)
	move.l	#AHIA_PlayerFreq,-(sp)
	move.l	#TRUE,-(sp)
	move.l	#AHIC_Play,-(sp)

	move.l	ptd_AHIBase(a2),a6
	move.l	ptd_AudioCtrl(a2),a2
	move.l	sp,a1
	jsr	_LVOAHI_ControlAudioA(a6)
	lea	5*4(sp),sp
	tst.l	d0
	bne	.error
	moveq	#TRUE,d0
.exit
	movem.l	(sp)+,d1-d7/a0-a6
	rts
.error
	moveq	#FALSE,d0
	bra	.exit

;in:
* a2	Filled PTData structure
_mt_stop
mt_stop
	movem.l	d0-d7/a0-a6,-(sp)

	sf	ptd_Enable(a2)
	move.l	#TAG_DONE,-(sp)
	move.l	#FALSE,-(sp)
	move.l	#AHIC_Play,-(sp)

	move.l	ptd_AHIBase(a2),a6
	move.l	ptd_AudioCtrl(a2),a2
	move.l	sp,a1
	jsr	_LVOAHI_ControlAudioA(a6)
	lea	3*4(sp),sp
	movem.l	(sp)+,d0-d7/a0-a6
	rts

;in:
* a2	Filled PTData structure
_mt_end
mt_end
	movem.l	d0-d7/a0-a6,-(sp)
	bsr	mt_stop
	move.l	ptd_AHIBase(a2),a6
	move.l	ptd_AudioCtrl(a2),a2
	moveq	#0,d2
	moveq	#0,d3
	moveq	#AHISF_IMM,d4
	moveq	#0,d0
	moveq	#AHI_NOSOUND,d1
	jsr	_LVOAHI_SetSound(a6)
	moveq	#1,d0
	moveq	#AHI_NOSOUND,d1
	jsr	_LVOAHI_SetSound(a6)
	moveq	#2,d0
	moveq	#AHI_NOSOUND,d1
	jsr	_LVOAHI_SetSound(a6)
	moveq	#3,d0
	moveq	#AHI_NOSOUND,d1
	jsr	_LVOAHI_SetSound(a6)
	moveq	#0,d0
	jsr	_LVOAHI_UnloadSound(a6)
	moveq	#0,d0
	movem.l	(sp)+,d0-d7/a0-a6
	rts

;---- Tempo ----

SetTempo
	CMP.W	#32,D0
	BHS.S	setemsk
	MOVEQ	#32,D0
setemsk
	addq.w	#1,ptd_NewTempo(a2)
	move.w	d0,ptd_Tempo(a2)
	RTS

;in:
* a2	Filled PTData structure
_mt_music
mt_music
	movem.l	d0-d7/a0-a6,-(sp)

	bsr	.org

	move.l	a2,a3
	move.l	ptd_AHIBase(a3),a6
	move.l	ptd_AudioCtrl(a3),a2
	lea	ptd_Chs(a3),a4
	moveq	#0,d7			;channel
.loop
	tst.w	pe_NewSample(a4)
	beq.b	.nosample
	clr.w	pe_NewSample(a4)
	move.l	d7,d0
	moveq	#0,d1
	move.l	pe_Offset(a4),d2
	moveq	#0,d3
	move.w	pe_Length(a4),d3
	lsl.l	#1,d3
	cmp.l	#2,d3
	bne	.1
	moveq	#AHI_NOSOUND,d1
.1
	moveq	#AHISF_IMM,d4
	jsr	_LVOAHI_SetSound(a6)
.nosample
	tst.w	pe_NewLoopSample(a4)
	beq.b	.noloopsample
	clr.w	pe_NewLoopSample(a4)
	move.l	d7,d0
	moveq	#0,d1
	move.l	pe_LoopOffset(a4),d2
	moveq	#0,d3
	move.w	pe_LoopLength(a4),d3
	lsl.l	#1,d3
	cmp.l	#2,d3
	bne	.2
	moveq	#AHI_NOSOUND,d1
.2
	moveq	#NULL,d4
	jsr	_LVOAHI_SetSound(a6)
.noloopsample
	tst.w	pe_NewPeriod(a4)
	beq.b	.noperiod
	clr.w	pe_NewPeriod(a4)
	move.l	d7,d0
	move.w	pe_Period(a4),d2
	beq	.noperiod
	move.l	#3546895,d1
	divu.w	d2,d1
	ext.l	d1
	moveq	#AHISF_IMM,d2
	jsr	_LVOAHI_SetFreq(a6)
.noperiod
	tst.w	pe_NewVolume(a4)
	beq.b	.novolume
	clr.w	pe_NewVolume(a4)
	move.l	d7,d0
	move.w	pe_Volume(a4),d1
	ext.l	d1
	lsl.l	#8,d1
	lsl.l	#2,d1
	move.l	d0,d2
	lsl.l	#2,d2
	move.l	.pantable(pc,d2.l),d2
	moveq	#AHISF_IMM,d3
	jsr	_LVOAHI_SetVol(a6)
.novolume
	add.w	#PaulaEmul_SIZEOF,a4
	addq.w	#1,d7
	cmp.w	#4,d7
	bne	.loop

	tst.w	ptd_NewTempo(a3)
	beq	.notempo
	clr.w	ptd_NewTempo(a3)
	move.w	ptd_Tempo(a3),d0
	ext.l	d0
	lsl.l	#1,d0
	divu	#5,d0
	swap.w	d0
	clr.w	d0
	move.l	#TAG_DONE,-(sp)
	move.l	d0,-(sp)
	move.l	#AHIA_PlayerFreq,-(sp)
	move.l	sp,a1
	jsr	_LVOAHI_ControlAudioA(a6)
	lea	3*4(sp),sp
.notempo

	movem.l	(sp)+,d0-d7/a0-a6
	rts
.pantable
	dc.l	$00000,$10000,$00000,$10000

.org
	MOVEM.L	D0-D4/A0-A6,-(SP)
	TST.B	ptd_Enable(a2)
	BEQ	mt_exit
	ADDQ.B	#1,ptd_counter(a2)
	MOVE.B	ptd_counter(a2),D0
	CMP.B	ptd_speed(a2),D0
	BLO.S	mt_NoNewNote
	CLR.B	ptd_counter(a2)
	TST.B	ptd_PattDelTime2(a2)
	BEQ.S	mt_GetNewNote
	BSR.S	mt_NoNewAllChannels
	BRA	mt_dskip

mt_NoNewNote
	BSR.S	mt_NoNewAllChannels
	BRA	mt_NoNewPosYet

mt_NoNewAllChannels
	LEA	ptd_Ch1(a2),A5
	LEA	ptd_chan1temp(a2),A6
	BSR	mt_CheckEfx
	LEA	ptd_Ch2(a2),A5
	LEA	ptd_chan2temp(a2),A6
	BSR	mt_CheckEfx
	LEA	ptd_Ch3(a2),A5
	LEA	ptd_chan3temp(a2),A6
	BSR	mt_CheckEfx
	LEA	ptd_Ch4(a2),A5
	LEA	ptd_chan4temp(a2),A6
	BRA	mt_CheckEfx

mt_GetNewNote
	MOVE.L	ptd_ModuleAddress(a2),A0
	LEA	12(A0),A3
	LEA	952(A0),A1	;pattpo
	LEA	1084(A0),A0	;patterndata
	MOVEQ	#0,D0
	MOVEQ	#0,D1
	MOVE.B	ptd_SongPos(a2),D0
	MOVE.B	(A1,D0.W),D1
	ASL.L	#8,D1
	ASL.L	#2,D1
	ADD.W	ptd_PatternPos(a2),D1
	CLR.W	ptd_DMACONtemp(a2)

	LEA	ptd_Ch1(a2),A5
	LEA	ptd_chan1temp(a2),A6
	BSR.S	mt_PlayVoice
	LEA	ptd_Ch2(a2),A5
	LEA	ptd_chan2temp(a2),A6
	BSR.S	mt_PlayVoice
	LEA	ptd_Ch3(a2),A5
	LEA	ptd_chan3temp(a2),A6
	BSR.S	mt_PlayVoice
	LEA	ptd_Ch4(a2),A5
	LEA	ptd_chan4temp(a2),A6
	BSR.S	mt_PlayVoice
	BRA	mt_SetDMA

mt_PlayVoice
	TST.L	(A6)
	BNE.S	mt_plvskip
	BSR	mt_PerNop
mt_plvskip
	MOVE.L	(A0,D1.L),(A6)
	ADDQ.L	#4,D1
	MOVEQ	#0,D2
	MOVE.B	n_cmd(A6),D2
	AND.B	#$F0,D2
	LSR.B	#4,D2
	MOVE.B	(A6),D0
	AND.B	#$F0,D0
	OR.B	D0,D2
	TST.B	D2
	BEQ	mt_SetRegs

	MOVEQ	#0,D3
	LEA	ptd_SampleStarts(a2),A1
	MOVE	D2,D4
	SUBQ.L	#1,D2
	ASL.L	#2,D2
	MULU	#30,D4
	MOVE.L	(A1,D2.L),n_start(A6)
	MOVE.W	(A3,D4.L),n_length(A6)
	MOVE.W	(A3,D4.L),n_reallength(A6)
	MOVE.B	2(A3,D4.L),n_finetune(A6)
	MOVE.B	3(A3,D4.L),n_volume(A6)
	MOVE.W	4(A3,D4.L),D3 ; Get repeat
	TST.W	D3
	BEQ.S	mt_NoLoop
	MOVE.L	n_start(A6),D2	; Get start
	ASL.W	#1,D3
	ADD.L	D3,D2		; Add repeat
	MOVE.L	D2,n_loopstart(A6)
	MOVE.L	D2,n_wavestart(A6)
	MOVE.W	4(A3,D4.L),D0	; Get repeat
	ADD.W	6(A3,D4.L),D0	; Add replen
	MOVE.W	D0,n_length(A6)
	MOVE.W	6(A3,D4.L),n_replen(A6)	; Save replen
	MOVEQ	#0,D0
	MOVE.B	n_volume(A6),D0
	move.w	d0,pe_Volume(a5)
	addq.w	#1,pe_NewVolume(a5)
;	MOVE.W	D0,8(A5)	; Set volume
	BRA.S	mt_SetRegs

mt_NoLoop
	MOVE.L	n_start(A6),D2
	ADD.L	D3,D2
	MOVE.L	D2,n_loopstart(A6)
	MOVE.L	D2,n_wavestart(A6)
	MOVE.W	6(A3,D4.L),n_replen(A6)	; Save replen
	MOVEQ	#0,D0
	MOVE.B	n_volume(A6),D0
	move.w	d0,pe_Volume(a5)
	addq.w	#1,pe_NewVolume(a5)
;	MOVE.W	D0,8(A5)	; Set volume
mt_SetRegs
	MOVE.W	(A6),D0
	AND.W	#$0FFF,D0
	BEQ	mt_CheckMoreEfx	; If no note
	MOVE.W	2(A6),D0
	AND.W	#$0FF0,D0
	CMP.W	#$0E50,D0
	BEQ.S	mt_DoSetFineTune
	MOVE.B	2(A6),D0
	AND.B	#$0F,D0
	CMP.B	#3,D0	; TonePortamento
	BEQ.S	mt_ChkTonePorta
	CMP.B	#5,D0
	BEQ.S	mt_ChkTonePorta
	CMP.B	#9,D0	; Sample Offset
	BNE.S	mt_SetPeriod
	BSR	mt_CheckMoreEfx
	BRA.S	mt_SetPeriod

mt_DoSetFineTune
	BSR	mt_SetFineTune
	BRA.S	mt_SetPeriod

mt_ChkTonePorta
	BSR	mt_SetTonePorta
	BRA	mt_CheckMoreEfx

mt_SetPeriod
	MOVEM.L	D0-D1/A0-A1,-(SP)
	MOVE.W	(A6),D1
	AND.W	#$0FFF,D1
	LEA	mt_PeriodTable(PC),A1
	MOVEQ	#0,D0
	MOVEQ	#36,D2
mt_ftuloop
	CMP.W	(A1,D0.W),D1
	BHS.S	mt_ftufound
	ADDQ.L	#2,D0
	DBRA	D2,mt_ftuloop
mt_ftufound
	MOVEQ	#0,D1
	MOVE.B	n_finetune(A6),D1
	MULU	#36*2,D1
	ADD.L	D1,A1
	MOVE.W	(A1,D0.W),n_period(A6)
	MOVEM.L	(SP)+,D0-D1/A0-A1

	MOVE.W	2(A6),D0
	AND.W	#$0FF0,D0
	CMP.W	#$0ED0,D0 ; Notedelay
	BEQ	mt_CheckMoreEfx

;	MOVE.W	n_dmabit(A6),$DFF096
	BTST	#2,n_wavecontrol(A6)
	BNE.S	mt_vibnoc
	CLR.B	n_vibratopos(A6)
mt_vibnoc
	BTST	#6,n_wavecontrol(A6)
	BNE.S	mt_trenoc
	CLR.B	n_tremolopos(A6)
mt_trenoc
	move.l	n_start(A6),pe_Offset(a5)
	move.w	n_length(A6),pe_Length(a5)
	addq.w	#1,pe_NewSample(a5)
	move.w	n_period(A6),pe_Period(a5)
	addq.w	#1,pe_NewPeriod(a5)

;	MOVE.L	n_start(A6),(A5)	; Set start
;	MOVE.W	n_length(A6),4(A5)	; Set length

;	MOVE.W	n_period(A6),D0
;	MOVE.W	D0,6(A5)		; Set period
;	MOVE.W	n_dmabit(A6),D0
;	OR.W	D0,ptd_DMACONtemp(a2)
	BRA	mt_CheckMoreEfx

mt_SetDMA
;	MOVE.W	#DMAWait,D0
;mt_WaitDMA
;	DBRA	D0,mt_WaitDMA
;	MOVE.W	mt_DMACONtemp(PC),D0
;	OR.W	#$8000,D0
;	MOVE.W	D0,$DFF096
;	MOVE.W	#DMAWait,D0
;mt_WaitDMA2
;	DBRA	D0,mt_WaitDMA2
;
;	LEA	$DFF000,A5
;	LEA	mt_chan4temp(PC),A6
;	MOVE.L	n_loopstart(A6),$D0(A5)
;	MOVE.W	n_replen(A6),$D4(A5)
;	LEA	mt_chan3temp(PC),A6
;	MOVE.L	n_loopstart(A6),$C0(A5)
;	MOVE.W	n_replen(A6),$C4(A5)
;	LEA	mt_chan2temp(PC),A6
;	MOVE.L	n_loopstart(A6),$B0(A5)
;	MOVE.W	n_replen(A6),$B4(A5)
;	LEA	mt_chan1temp(PC),A6
;	MOVE.L	n_loopstart(A6),$A0(A5)
;	MOVE.W	n_replen(A6),$A4(A5)

	move.l	n_loopstart+ptd_chan1temp(A2),pe_LoopOffset+ptd_Ch1(a2)
	move.w	n_replen+ptd_chan1temp(A2),pe_LoopLength+ptd_Ch1(a2)
	addq.w	#1,pe_NewLoopSample+ptd_Ch1(a2)

	move.l	n_loopstart+ptd_chan2temp(A2),pe_LoopOffset+ptd_Ch2(a2)
	move.w	n_replen+ptd_chan2temp(A2),pe_LoopLength+ptd_Ch2(a2)
	addq.w	#1,pe_NewLoopSample+ptd_Ch2(a2)

	move.l	n_loopstart+ptd_chan3temp(A2),pe_LoopOffset+ptd_Ch3(a2)
	move.w	n_replen+ptd_chan3temp(A2),pe_LoopLength+ptd_Ch3(a2)
	addq.w	#1,pe_NewLoopSample+ptd_Ch3(a2)

	move.l	n_loopstart+ptd_chan4temp(A2),pe_LoopOffset+ptd_Ch4(a2)
	move.w	n_replen+ptd_chan4temp(A2),pe_LoopLength+ptd_Ch4(a2)
	addq.w	#1,pe_NewLoopSample+ptd_Ch4(a2)


mt_dskip
	ADD.W	#16,ptd_PatternPos(a2)
	MOVE.B	ptd_PattDelTime(a2),D0
	BEQ.S	mt_dskc
	MOVE.B	D0,ptd_PattDelTime2(a2)
	CLR.B	ptd_PattDelTime(a2)
mt_dskc	TST.B	ptd_PattDelTime2(a2)
	BEQ.S	mt_dska
	SUBQ.B	#1,ptd_PattDelTime2(a2)
	BEQ.S	mt_dska
	SUB.W	#16,ptd_PatternPos(a2)
mt_dska	TST.B	ptd_PBreakFlag(a2)
	BEQ.S	mt_nnpysk
	SF	ptd_PBreakFlag(a2)
	MOVEQ	#0,D0
	MOVE.B	ptd_PBreakPos(a2),D0
	CLR.B	ptd_PBreakPos(a2)
	LSL.W	#4,D0
	MOVE.W	D0,ptd_PatternPos(a2)
mt_nnpysk
	CMP.W	#1024,ptd_PatternPos(a2)
	BLO.S	mt_NoNewPosYet
mt_NextPosition	
	MOVEQ	#0,D0
	MOVE.B	ptd_PBreakPos(a2),D0
	LSL.W	#4,D0
	MOVE.W	D0,ptd_PatternPos(a2)
	CLR.B	ptd_PBreakPos(a2)
	CLR.B	ptd_PosJumpFlag(a2)
	ADDQ.B	#1,ptd_SongPos(a2)
	AND.B	#$7F,ptd_SongPos(a2)
	MOVE.B	ptd_SongPos(a2),D1
	MOVE.L	ptd_ModuleAddress(a2),A0
	CMP.B	950(A0),D1
	BLO.S	mt_NoNewPosYet
	CLR.B	ptd_SongPos(a2)
mt_NoNewPosYet	
	TST.B	ptd_PosJumpFlag(a2)
	BNE.S	mt_NextPosition
mt_exit	MOVEM.L	(SP)+,D0-D4/A0-A6
	RTS

mt_CheckEfx
	BSR	mt_UpdateFunk
	MOVE.W	n_cmd(A6),D0
	AND.W	#$0FFF,D0
	BEQ.S	mt_PerNop
	MOVE.B	n_cmd(A6),D0
	AND.B	#$0F,D0
	BEQ.S	mt_Arpeggio
	CMP.B	#1,D0
	BEQ	mt_PortaUp
	CMP.B	#2,D0
	BEQ	mt_PortaDown
	CMP.B	#3,D0
	BEQ	mt_TonePortamento
	CMP.B	#4,D0
	BEQ	mt_Vibrato
	CMP.B	#5,D0
	BEQ	mt_TonePlusVolSlide
	CMP.B	#6,D0
	BEQ	mt_VibratoPlusVolSlide
	CMP.B	#$E,D0
	BEQ	mt_E_Commands
SetBack
;	MOVE.W	n_period(A6),6(A5)
	move.w	n_period(a6),pe_Period(a5)
	addq.w	#1,pe_NewPeriod(a5)
	CMP.B	#7,D0
	BEQ	mt_Tremolo
	CMP.B	#$A,D0
	BEQ	mt_VolumeSlide
mt_Return
	RTS

mt_PerNop
;	MOVE.W	n_period(A6),6(A5)
	move.w	n_period(a6),pe_Period(a5)
	addq.w	#1,pe_NewPeriod(a5)
	RTS

mt_Arpeggio
	MOVEQ	#0,D0
	MOVE.B	ptd_counter(a2),D0
	DIVS	#3,D0
	SWAP	D0
	CMP.W	#0,D0
	BEQ.S	mt_Arpeggio2
	CMP.W	#2,D0
	BEQ.S	mt_Arpeggio1
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	LSR.B	#4,D0
	BRA.S	mt_Arpeggio3

mt_Arpeggio1
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#15,D0
	BRA.S	mt_Arpeggio3

mt_Arpeggio2
	MOVE.W	n_period(A6),D2
	BRA.S	mt_Arpeggio4

mt_Arpeggio3
	ASL.W	#1,D0
	MOVEQ	#0,D1
	MOVE.B	n_finetune(A6),D1
	MULU	#36*2,D1
	LEA	mt_PeriodTable(PC),A0
	ADD.L	D1,A0
	MOVEQ	#0,D1
	MOVE.W	n_period(A6),D1
	MOVEQ	#36,D3
mt_arploop
	MOVE.W	(A0,D0.W),D2
	CMP.W	(A0),D1
	BHS.S	mt_Arpeggio4
	ADDQ.L	#2,A0
	DBRA	D3,mt_arploop
	RTS

mt_Arpeggio4
;	MOVE.W	D2,6(A5)
	move.w	d2,pe_Period(a5)
	addq.w	#1,pe_NewPeriod(a5)
	RTS

mt_FinePortaUp
	TST.B	ptd_counter(a2)
	BNE	mt_Return
	MOVE.B	#$0F,ptd_LowMask(a2)
mt_PortaUp
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	AND.B	ptd_LowMask(a2),D0
	MOVE.B	#$FF,ptd_LowMask(a2)
	SUB.W	D0,n_period(A6)
	MOVE.W	n_period(A6),D0
	AND.W	#$0FFF,D0
	CMP.W	#113,D0
	BPL.S	mt_PortaUskip
	AND.W	#$F000,n_period(A6)
	OR.W	#113,n_period(A6)
mt_PortaUskip
	MOVE.W	n_period(A6),D0
	AND.W	#$0FFF,D0
;	MOVE.W	D0,6(A5)
	move.w	d0,pe_Period(a5)
	addq.w	#1,pe_NewPeriod(a5)
	RTS	
 
mt_FinePortaDown
	TST.B	ptd_counter(a2)
	BNE	mt_Return
	MOVE.B	#$0F,ptd_LowMask(a2)
mt_PortaDown
	CLR.W	D0
	MOVE.B	n_cmdlo(A6),D0
	AND.B	ptd_LowMask(a2),D0
	MOVE.B	#$FF,ptd_LowMask(a2)
	ADD.W	D0,n_period(A6)
	MOVE.W	n_period(A6),D0
	AND.W	#$0FFF,D0
	CMP.W	#856,D0
	BMI.S	mt_PortaDskip
	AND.W	#$F000,n_period(A6)
	OR.W	#856,n_period(A6)
mt_PortaDskip
	MOVE.W	n_period(A6),D0
	AND.W	#$0FFF,D0
	move.w	d0,pe_Period(a5)
	addq.w	#1,pe_NewPeriod(a5)
;	MOVE.W	D0,6(A5)
	RTS

mt_SetTonePorta
	MOVE.L	A0,-(SP)
	MOVE.W	(A6),D2
	AND.W	#$0FFF,D2
	MOVEQ	#0,D0
	MOVE.B	n_finetune(A6),D0
	MULU	#36*2,D0 ;37?
	LEA	mt_PeriodTable(PC),A0
	ADD.L	D0,A0
	MOVEQ	#0,D0
mt_StpLoop
	CMP.W	(A0,D0.W),D2
	BHS.S	mt_StpFound
	ADDQ.W	#2,D0
	CMP.W	#36*2,D0 ;37?
	BLO.S	mt_StpLoop
	MOVEQ	#35*2,D0
mt_StpFound
	MOVE.B	n_finetune(A6),D2
	AND.B	#8,D2
	BEQ.S	mt_StpGoss
	TST.W	D0
	BEQ.S	mt_StpGoss
	SUBQ.W	#2,D0
mt_StpGoss
	MOVE.W	(A0,D0.W),D2
	MOVE.L	(SP)+,A0
	MOVE.W	D2,n_wantedperiod(A6)
	MOVE.W	n_period(A6),D0
	CLR.B	n_toneportdirec(A6)
	CMP.W	D0,D2
	BEQ.S	mt_ClearTonePorta
	BGE	mt_Return
	MOVE.B	#1,n_toneportdirec(A6)
	RTS

mt_ClearTonePorta
	CLR.W	n_wantedperiod(A6)
	RTS

mt_TonePortamento
	MOVE.B	n_cmdlo(A6),D0
	BEQ.S	mt_TonePortNoChange
	MOVE.B	D0,n_toneportspeed(A6)
	CLR.B	n_cmdlo(A6)
mt_TonePortNoChange
	TST.W	n_wantedperiod(A6)
	BEQ	mt_Return
	MOVEQ	#0,D0
	MOVE.B	n_toneportspeed(A6),D0
	TST.B	n_toneportdirec(A6)
	BNE.S	mt_TonePortaUp
mt_TonePortaDown
	ADD.W	D0,n_period(A6)
	MOVE.W	n_wantedperiod(A6),D0
	CMP.W	n_period(A6),D0
	BGT.S	mt_TonePortaSetPer
	MOVE.W	n_wantedperiod(A6),n_period(A6)
	CLR.W	n_wantedperiod(A6)
	BRA.S	mt_TonePortaSetPer

mt_TonePortaUp
	SUB.W	D0,n_period(A6)
	MOVE.W	n_wantedperiod(A6),D0
	CMP.W	n_period(A6),D0
	BLT.S	mt_TonePortaSetPer
	MOVE.W	n_wantedperiod(A6),n_period(A6)
	CLR.W	n_wantedperiod(A6)

mt_TonePortaSetPer
	MOVE.W	n_period(A6),D2
	MOVE.B	n_glissfunk(A6),D0
	AND.B	#$0F,D0
	BEQ.S	mt_GlissSkip
	MOVEQ	#0,D0
	MOVE.B	n_finetune(A6),D0
	MULU	#36*2,D0
	LEA	mt_PeriodTable(PC),A0
	ADD.L	D0,A0
	MOVEQ	#0,D0
mt_GlissLoop
	CMP.W	(A0,D0.W),D2
	BHS.S	mt_GlissFound
	ADDQ.W	#2,D0
	CMP.W	#36*2,D0
	BLO.S	mt_GlissLoop
	MOVEQ	#35*2,D0
mt_GlissFound
	MOVE.W	(A0,D0.W),D2
mt_GlissSkip
;	MOVE.W	D2,6(A5) ; Set period
	move.w	d2,pe_Period(a5)
	addq.w	#1,pe_NewPeriod(a5)
	RTS

mt_Vibrato
	MOVE.B	n_cmdlo(A6),D0
	BEQ.S	mt_Vibrato2
	MOVE.B	n_vibratocmd(A6),D2
	AND.B	#$0F,D0
	BEQ.S	mt_vibskip
	AND.B	#$F0,D2
	OR.B	D0,D2
mt_vibskip
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$F0,D0
	BEQ.S	mt_vibskip2
	AND.B	#$0F,D2
	OR.B	D0,D2
mt_vibskip2
	MOVE.B	D2,n_vibratocmd(A6)
mt_Vibrato2
	MOVE.B	n_vibratopos(A6),D0
	LEA	mt_VibratoTable(PC),A4
	LSR.W	#2,D0
	AND.W	#$001F,D0
	MOVEQ	#0,D2
	MOVE.B	n_wavecontrol(A6),D2
	AND.B	#$03,D2
	BEQ.S	mt_vib_sine
	LSL.B	#3,D0
	CMP.B	#1,D2
	BEQ.S	mt_vib_rampdown
	MOVE.B	#255,D2
	BRA.S	mt_vib_set
mt_vib_rampdown
	TST.B	n_vibratopos(A6)
	BPL.S	mt_vib_rampdown2
	MOVE.B	#255,D2
	SUB.B	D0,D2
	BRA.S	mt_vib_set
mt_vib_rampdown2
	MOVE.B	D0,D2
	BRA.S	mt_vib_set
mt_vib_sine
	MOVE.B	(A4,D0.W),D2
mt_vib_set
	MOVE.B	n_vibratocmd(A6),D0
	AND.W	#15,D0
	MULU	D0,D2
	LSR.W	#7,D2
	MOVE.W	n_period(A6),D0
	TST.B	n_vibratopos(A6)
	BMI.S	mt_VibratoNeg
	ADD.W	D2,D0
	BRA.S	mt_Vibrato3
mt_VibratoNeg
	SUB.W	D2,D0
mt_Vibrato3
;	MOVE.W	D0,6(A5)
	move.w	d0,pe_Period(a5)
	addq.w	#1,pe_NewPeriod(a5)
	MOVE.B	n_vibratocmd(A6),D0
	LSR.W	#2,D0
	AND.W	#$003C,D0
	ADD.B	D0,n_vibratopos(A6)
	RTS

mt_TonePlusVolSlide
	BSR	mt_TonePortNoChange
	BRA	mt_VolumeSlide

mt_VibratoPlusVolSlide
	BSR.S	mt_Vibrato2
	BRA	mt_VolumeSlide

mt_Tremolo
	MOVE.B	n_cmdlo(A6),D0
	BEQ.S	mt_Tremolo2
	MOVE.B	n_tremolocmd(A6),D2
	AND.B	#$0F,D0
	BEQ.S	mt_treskip
	AND.B	#$F0,D2
	OR.B	D0,D2
mt_treskip
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$F0,D0
	BEQ.S	mt_treskip2
	AND.B	#$0F,D2
	OR.B	D0,D2
mt_treskip2
	MOVE.B	D2,n_tremolocmd(A6)
mt_Tremolo2
	MOVE.B	n_tremolopos(A6),D0
	LEA	mt_VibratoTable(PC),A4
	LSR.W	#2,D0
	AND.W	#$001F,D0
	MOVEQ	#0,D2
	MOVE.B	n_wavecontrol(A6),D2
	LSR.B	#4,D2
	AND.B	#$03,D2
	BEQ.S	mt_tre_sine
	LSL.B	#3,D0
	CMP.B	#1,D2
	BEQ.S	mt_tre_rampdown
	MOVE.B	#255,D2
	BRA.S	mt_tre_set
mt_tre_rampdown
	TST.B	n_vibratopos(A6)
	BPL.S	mt_tre_rampdown2
	MOVE.B	#255,D2
	SUB.B	D0,D2
	BRA.S	mt_tre_set
mt_tre_rampdown2
	MOVE.B	D0,D2
	BRA.S	mt_tre_set
mt_tre_sine
	MOVE.B	(A4,D0.W),D2
mt_tre_set
	MOVE.B	n_tremolocmd(A6),D0
	AND.W	#15,D0
	MULU	D0,D2
	LSR.W	#6,D2
	MOVEQ	#0,D0
	MOVE.B	n_volume(A6),D0
	TST.B	n_tremolopos(A6)
	BMI.S	mt_TremoloNeg
	ADD.W	D2,D0
	BRA.S	mt_Tremolo3
mt_TremoloNeg
	SUB.W	D2,D0
mt_Tremolo3
	BPL.S	mt_TremoloSkip
	CLR.W	D0
mt_TremoloSkip
	CMP.W	#$40,D0
	BLS.S	mt_TremoloOk
	MOVE.W	#$40,D0
mt_TremoloOk
;	MOVE.W	D0,8(A5)
	move.w	d0,pe_Volume(a5)
	addq.w	#1,pe_NewVolume(a5)
	MOVE.B	n_tremolocmd(A6),D0
	LSR.W	#2,D0
	AND.W	#$003C,D0
	ADD.B	D0,n_tremolopos(A6)
	RTS

mt_SampleOffset
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	BEQ.S	mt_sononew
	MOVE.B	D0,n_sampleoffset(A6)
mt_sononew
	MOVE.B	n_sampleoffset(A6),D0
	LSL.W	#7,D0
	CMP.W	n_length(A6),D0
	BGE.S	mt_sofskip
	SUB.W	D0,n_length(A6)
	LSL.W	#1,D0
	ADD.L	D0,n_start(A6)
	RTS
mt_sofskip
	MOVE.W	#$0001,n_length(A6)
	RTS

mt_VolumeSlide
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	LSR.B	#4,D0
	TST.B	D0
	BEQ.S	mt_VolSlideDown
mt_VolSlideUp
	ADD.B	D0,n_volume(A6)
	CMP.B	#$40,n_volume(A6)
	BMI.S	mt_vsuskip
	MOVE.B	#$40,n_volume(A6)
mt_vsuskip
	MOVE.B	n_volume(A6),D0
;	MOVE.W	D0,8(A5)
	move.w	d0,pe_Volume(a5)
	addq.w	#1,pe_NewVolume(a5)
	RTS

mt_VolSlideDown
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
mt_VolSlideDown2
	SUB.B	D0,n_volume(A6)
	BPL.S	mt_vsdskip
	CLR.B	n_volume(A6)
mt_vsdskip
	MOVE.B	n_volume(A6),D0
;	MOVE.W	D0,8(A5)
	move.w	d0,pe_Volume(a5)
	addq.w	#1,pe_NewVolume(a5)
	RTS

mt_PositionJump
	MOVE.B	n_cmdlo(A6),D0
	SUBQ.B	#1,D0
	MOVE.B	D0,ptd_SongPos(a2)
mt_pj2	CLR.B	ptd_PBreakPos(a2)
	ST 	ptd_PosJumpFlag(a2)
	RTS

mt_VolumeChange
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	CMP.B	#$40,D0
	BLS.S	mt_VolumeOk
	MOVEQ	#$40,D0
mt_VolumeOk
	MOVE.B	D0,n_volume(A6)
;	MOVE.W	D0,8(A5)
	move.w	d0,pe_Volume(a5)
	addq.w	#1,pe_NewVolume(a5)
	RTS

mt_PatternBreak
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	MOVE.L	D0,D2
	LSR.B	#4,D0
	MULU	#10,D0
	AND.B	#$0F,D2
	ADD.B	D2,D0
	CMP.B	#63,D0
	BHI.S	mt_pj2
	MOVE.B	D0,ptd_PBreakPos(a2)
	ST	ptd_PosJumpFlag(a2)
	RTS

mt_SetSpeed
	MOVEQ	#0,D0
	MOVE.B	3(A6),D0
	BEQ	mt_end
	CMP.B	#32,D0
	BHS	SetTempo
	CLR.B	ptd_counter(a2)
	MOVE.B	D0,ptd_speed(a2)
	RTS

mt_CheckMoreEfx
	BSR	mt_UpdateFunk
	MOVE.B	2(A6),D0
	AND.B	#$0F,D0
	CMP.B	#$9,D0
	BEQ	mt_SampleOffset
	CMP.B	#$B,D0
	BEQ	mt_PositionJump
	CMP.B	#$D,D0
	BEQ.S	mt_PatternBreak
	CMP.B	#$E,D0
	BEQ.S	mt_E_Commands
	CMP.B	#$F,D0
	BEQ.S	mt_SetSpeed
	CMP.B	#$C,D0
	BEQ	mt_VolumeChange
	BRA	mt_PerNop

mt_E_Commands
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$F0,D0
	LSR.B	#4,D0
	BEQ.S	mt_FilterOnOff
	CMP.B	#1,D0
	BEQ	mt_FinePortaUp
	CMP.B	#2,D0
	BEQ	mt_FinePortaDown
	CMP.B	#3,D0
	BEQ.S	mt_SetGlissControl
	CMP.B	#4,D0
	BEQ	mt_SetVibratoControl
	CMP.B	#5,D0
	BEQ	mt_SetFineTune
	CMP.B	#6,D0
	BEQ	mt_JumpLoop
	CMP.B	#7,D0
	BEQ	mt_SetTremoloControl
	CMP.B	#9,D0
	BEQ	mt_RetrigNote
	CMP.B	#$A,D0
	BEQ	mt_VolumeFineUp
	CMP.B	#$B,D0
	BEQ	mt_VolumeFineDown
	CMP.B	#$C,D0
	BEQ	mt_NoteCut
	CMP.B	#$D,D0
	BEQ	mt_NoteDelay
	CMP.B	#$E,D0
	BEQ	mt_PatternDelay
	CMP.B	#$F,D0
	BEQ	mt_FunkIt
	RTS

mt_FilterOnOff
;	MOVE.B	n_cmdlo(A6),D0
;	AND.B	#1,D0
;	ASL.B	#1,D0
;	AND.B	#$FD,$BFE001
;	OR.B	D0,$BFE001
	RTS	

mt_SetGlissControl
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
	AND.B	#$F0,n_glissfunk(A6)
	OR.B	D0,n_glissfunk(A6)
	RTS

mt_SetVibratoControl
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
	AND.B	#$F0,n_wavecontrol(A6)
	OR.B	D0,n_wavecontrol(A6)
	RTS

mt_SetFineTune
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
	MOVE.B	D0,n_finetune(A6)
	RTS

mt_JumpLoop
	TST.B	ptd_counter(a2)
	BNE	mt_Return
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
	BEQ.S	mt_SetLoop
	TST.B	n_loopcount(A6)
	BEQ.S	mt_jumpcnt
	SUBQ.B	#1,n_loopcount(A6)
	BEQ	mt_Return
mt_jmploop
	MOVE.B	n_pattpos(A6),ptd_PBreakPos(a2)
	ST	ptd_PBreakFlag(a2)
	RTS

mt_jumpcnt
	MOVE.B	D0,n_loopcount(A6)
	BRA.S	mt_jmploop

mt_SetLoop
	MOVE.W	ptd_PatternPos(a2),D0
	LSR.W	#4,D0
	MOVE.B	D0,n_pattpos(A6)
	RTS

mt_SetTremoloControl
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
	LSL.B	#4,D0
	AND.B	#$0F,n_wavecontrol(A6)
	OR.B	D0,n_wavecontrol(A6)
	RTS

mt_RetrigNote
	MOVE.L	D1,-(SP)
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
	BEQ.S	mt_rtnend
	MOVEQ	#0,D1
	MOVE.B	ptd_counter(a2),D1
	BNE.S	mt_rtnskp
	MOVE.W	(A6),D1
	AND.W	#$0FFF,D1
	BNE.S	mt_rtnend
	MOVEQ	#0,D1
	MOVE.B	ptd_counter(a2),D1
mt_rtnskp
	DIVU	D0,D1
	SWAP	D1
	TST.W	D1
	BNE.S	mt_rtnend
mt_DoRetrig
;	MOVE.W	n_dmabit(A6),$DFF096	; Channel DMA off
;	MOVE.L	n_start(A6),(A5)	; Set sampledata pointer
;	MOVE.W	n_length(A6),4(A5)	; Set length
	move.l	n_start(A6),pe_Offset(a5)
	move.w	n_length(A6),pe_Length(a5)
	addq.w	#1,pe_NewSample(a5)

;	MOVE.W	#DMAWait,D0
;mt_rtnloop1
;	DBRA	D0,mt_rtnloop1
;	MOVE.W	n_dmabit(A6),D0
;	BSET	#15,D0
;	MOVE.W	D0,$DFF096
;	MOVE.W	#DMAWait,D0
;mt_rtnloop2
;	DBRA	D0,mt_rtnloop2
;
;	MOVE.L	n_loopstart(A6),(A5)
;	MOVE.L	n_replen(A6),4(A5)
	move.l	n_loopstart(A6),pe_LoopOffset(a5)
	move.w	n_replen(A6),pe_LoopLength(a5)
	addq.w	#1,pe_NewLoopSample(a5)
mt_rtnend
	MOVE.L	(SP)+,D1
	RTS

mt_VolumeFineUp
	TST.B	ptd_counter(a2)
	BNE	mt_Return
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$F,D0
	BRA	mt_VolSlideUp

mt_VolumeFineDown
	TST.B	ptd_counter(a2)
	BNE	mt_Return
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
	BRA	mt_VolSlideDown2

mt_NoteCut
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
	CMP.B	ptd_counter(a2),D0
	BNE	mt_Return
	CLR.B	n_volume(A6)
;	MOVE.W	#0,8(A5)
	clr.w	pe_Volume(a5)
	addq.w	#1,pe_NewVolume(a5)
	RTS

mt_NoteDelay
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
	CMP.B	ptd_counter(a2),D0
	BNE	mt_Return
	MOVE.W	(A6),D0
	BEQ	mt_Return
	MOVE.L	D1,-(SP)
	BRA	mt_DoRetrig

mt_PatternDelay
	TST.B	ptd_counter(a2)
	BNE	mt_Return
	MOVEQ	#0,D0
	MOVE.B	n_cmdlo(A6),D0
	AND.B	#$0F,D0
	TST.B	ptd_PattDelTime2(a2)
	BNE	mt_Return
	ADDQ.B	#1,D0
	MOVE.B	D0,ptd_PattDelTime(a2)
	RTS

mt_FunkIt
;	TST.B	mt_counter
;	BNE	mt_Return
;	MOVE.B	n_cmdlo(A6),D0
;	AND.B	#$0F,D0
;	LSL.B	#4,D0
;	AND.B	#$0F,n_glissfunk(A6)
;	OR.B	D0,n_glissfunk(A6)
;	TST.B	D0
;	BEQ	mt_Return
mt_UpdateFunk
;	MOVEM.L	A0/D1,-(SP)
;	MOVEQ	#0,D0
;	MOVE.B	n_glissfunk(A6),D0
;	LSR.B	#4,D0
;	BEQ.S	mt_funkend
;	LEA	mt_FunkTable(PC),A0
;	MOVE.B	(A0,D0.W),D0
;	ADD.B	D0,n_funkoffset(A6)
;	BTST	#7,n_funkoffset(A6)
;	BEQ.S	mt_funkend
;	CLR.B	n_funkoffset(A6)
;
;	MOVE.L	n_loopstart(A6),D0
;	MOVEQ	#0,D1
;	MOVE.W	n_replen(A6),D1
;	ADD.L	D1,D0
;	ADD.L	D1,D0
;	MOVE.L	n_wavestart(A6),A0
;	ADDQ.L	#1,A0
;	CMP.L	D0,A0
;	BLO.S	mt_funkok
;	MOVE.L	n_loopstart(A6),A0
;mt_funkok
;	MOVE.L	A0,n_wavestart(A6)
;	MOVEQ	#-1,D0
;	SUB.B	(A0),D0
;	MOVE.B	D0,(A0)
;mt_funkend
;	MOVEM.L	(SP)+,A0/D1
	RTS


;mt_FunkTable dc.b 0,5,6,7,8,10,11,13,16,19,22,26,32,43,64,128

mt_VibratoTable	
	dc.b   0, 24, 49, 74, 97,120,141,161
	dc.b 180,197,212,224,235,244,250,253
	dc.b 255,253,250,244,235,224,212,197
	dc.b 180,161,141,120, 97, 74, 49, 24

mt_PeriodTable
; Tuning 0, Normal
	dc.w	856,808,762,720,678,640,604,570,538,508,480,453
	dc.w	428,404,381,360,339,320,302,285,269,254,240,226
	dc.w	214,202,190,180,170,160,151,143,135,127,120,113
; Tuning 1
	dc.w	850,802,757,715,674,637,601,567,535,505,477,450
	dc.w	425,401,379,357,337,318,300,284,268,253,239,225
	dc.w	213,201,189,179,169,159,150,142,134,126,119,113
; Tuning 2
	dc.w	844,796,752,709,670,632,597,563,532,502,474,447
	dc.w	422,398,376,355,335,316,298,282,266,251,237,224
	dc.w	211,199,188,177,167,158,149,141,133,125,118,112
; Tuning 3
	dc.w	838,791,746,704,665,628,592,559,528,498,470,444
	dc.w	419,395,373,352,332,314,296,280,264,249,235,222
	dc.w	209,198,187,176,166,157,148,140,132,125,118,111
; Tuning 4
	dc.w	832,785,741,699,660,623,588,555,524,495,467,441
	dc.w	416,392,370,350,330,312,294,278,262,247,233,220
	dc.w	208,196,185,175,165,156,147,139,131,124,117,110
; Tuning 5
	dc.w	826,779,736,694,655,619,584,551,520,491,463,437
	dc.w	413,390,368,347,328,309,292,276,260,245,232,219
	dc.w	206,195,184,174,164,155,146,138,130,123,116,109
; Tuning 6
	dc.w	820,774,730,689,651,614,580,547,516,487,460,434
	dc.w	410,387,365,345,325,307,290,274,258,244,230,217
	dc.w	205,193,183,172,163,154,145,137,129,122,115,109
; Tuning 7
	dc.w	814,768,725,684,646,610,575,543,513,484,457,431
	dc.w	407,384,363,342,323,305,288,272,256,242,228,216
	dc.w	204,192,181,171,161,152,144,136,128,121,114,108
; Tuning -8
	dc.w	907,856,808,762,720,678,640,604,570,538,508,480
	dc.w	453,428,404,381,360,339,320,302,285,269,254,240
	dc.w	226,214,202,190,180,170,160,151,143,135,127,120
; Tuning -7
	dc.w	900,850,802,757,715,675,636,601,567,535,505,477
	dc.w	450,425,401,379,357,337,318,300,284,268,253,238
	dc.w	225,212,200,189,179,169,159,150,142,134,126,119
; Tuning -6
	dc.w	894,844,796,752,709,670,632,597,563,532,502,474
	dc.w	447,422,398,376,355,335,316,298,282,266,251,237
	dc.w	223,211,199,188,177,167,158,149,141,133,125,118
; Tuning -5
	dc.w	887,838,791,746,704,665,628,592,559,528,498,470
	dc.w	444,419,395,373,352,332,314,296,280,264,249,235
	dc.w	222,209,198,187,176,166,157,148,140,132,125,118
; Tuning -4
	dc.w	881,832,785,741,699,660,623,588,555,524,494,467
	dc.w	441,416,392,370,350,330,312,294,278,262,247,233
	dc.w	220,208,196,185,175,165,156,147,139,131,123,117
; Tuning -3
	dc.w	875,826,779,736,694,655,619,584,551,520,491,463
	dc.w	437,413,390,368,347,328,309,292,276,260,245,232
	dc.w	219,206,195,184,174,164,155,146,138,130,123,116
; Tuning -2
	dc.w	868,820,774,730,689,651,614,580,547,516,487,460
	dc.w	434,410,387,365,345,325,307,290,274,258,244,230
	dc.w	217,205,193,183,172,163,154,145,137,129,122,115
; Tuning -1
	dc.w	862,814,768,725,684,646,610,575,543,513,484,457
	dc.w	431,407,384,363,342,323,305,288,272,256,242,228
	dc.w	216,203,192,181,171,161,152,144,136,128,121,114

;/* End of File */
