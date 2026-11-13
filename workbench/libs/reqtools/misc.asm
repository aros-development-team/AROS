
	SECTION "dofmt",CODE

	INCLUDE "exec/types.i"
	INCLUDE "exec/funcdef.i"
	INCLUDE "exec/exec_lib.i"
	INCLUDE "lvo/utility_lib.i"
	INCLUDE "intuition/intuitionbase.i"
;	INCLUDE "lvo/intuition_lib.i"
	INCLUDE "lvo/graphics_lib.i"

	INCLUDE "utility/hooks.i"

	INCLUDE "libraries/gadtools.i"

	INCLUDE "intuition/intuition.i"
	INCLUDE "lvo/intuition_lib.i"
	INCLUDE "intuition/classes.i"
	INCLUDE "intuition/classusr.i"
	INCLUDE "intuition/imageclass.i"

	INCLUDE "libraries/reqtools.i"
	INCLUDE "lvo/reqtools_lib.i"

	XDEF _DofmtCount
	XDEF _Dofmt
	XDEF _DofmtArgs
	XDEF _FillBarTable
	XDEF _FillNewLineTable
	XDEF _LoopReqHandler
	XDEF _CallHook
	XDEF _StrCat
	XDEF _MakeColVal
	XDEF @SpreadColors
	XDEF @DoWaitPointer
	XDEF @DoLockWindow
	XDEF @SetWinTitleFlash

	XREF _ReqToolsBase
	XREF _IntuitionBase
	XREF _GfxBase
	XREF _UtilityBase

	XREF __UCXD33

* void __regargs SetWinTitleFlash (struct Window *win, char *str)
*                                  A0                  A1
@SetWinTitleFlash:
	movem.l	a2/a6,-(a7)
	move.l	_IntuitionBase(a4),a6
	movem.l	a0/a1,-(a7)
	move.l	wd_WScreen(a0),a0
	jsr	_LVODisplayBeep(a6)
	movem.l	(a7)+,a0/a1
	moveq	#-1,d0
	move.l	d0,a2
	jsr	_LVOSetWindowTitles(a6)
	movem.l	(a7)+,a2/a6
	rts

* void __regargs DoWaitPointer (struct Window *win, int doit, int setpointer)
*                               A0                  D0        D1
@DoWaitPointer:
	tst.l	d0
	beq.b	exitwaitpointer

	move.l	a0,d0
	beq.b	exitwaitpointer

	move.l	a6,-(a7)
	tst.l	d1
	beq.b	clearpointer

setpointer:
	move.l	_ReqToolsBase(a4),a6
	jsr	_LVOrtSetWaitPointer(a6)
	bra.b	endwaitpointer

clearpointer:
	move.l	_IntuitionBase(a4),a6
	jsr	_LVOClearPointer(a6)
endwaitpointer:
	move.l	(a7)+,a6
exitwaitpointer:
	rts

* APTR __regargs DoLockWindow (struct Window *win, int doit, APTR lock, int lockit)
*                              A0                  D0        A1         D1
@DoLockWindow:
	tst.l	d0
	beq.b	exitlockwin

	move.l	a0,d0
	beq.b	exitlockwin

	move.l	a6,-(a7)
	move.l	_ReqToolsBase(a4),a6
	tst.l	d1
	beq.b	unlockwin

	jsr	_LVOrtLockWindow(a6)
	bra.b	endlockwin

unlockwin:
	jsr	_LVOrtUnlockWindow(a6)
endlockwin:
	move.l	(a7)+,a6
exitlockwin:
	rts

* void __regargs SpreadColors (GlobData *glob, int from, int to, long *rgb2)
*                              A0              D0        D1      A1

cols		equ		12
vp		equ		(cols+3*4)
redbits		equ		(vp+4)
greenbits	equ		(redbits+4)
bluebits	equ		(greenbits+4)

* _LVOSetRGB32	equ	-$354

_MakeColVal:
	move.l	d1,-(a7)
	moveq	#32,d1
	sub.l	d4,d1
	lsl.l	d1,d0
	move.l	d0,d1			; shift color value completely to the left
.nextshift:
	lsr.l	d4,d1
	beq.b	endmakecol

	or.l	d1,d0			; shift right and or
	bra.b	.nextshift

endmakecol:
	move.l	(a7)+,d1
	rts

@SpreadColors:
	movem.l	d2-d7/a2-a6,-(a7)
	lea	-24(a7),a7
	lea	12(a7),a2		; a2 -> rgb[3]
	move.l	a7,a3			; a3 -> rgbstep[3]
	moveq	#1,d4			; d4 = colstep = 1
	move.l	a0,a5
	move.l	d0,d5
	move.l	d1,d6
	move.l	d6,d2
	sub.l	d5,d2			; d2 = steps
	beq	nospread

	bpl.b	posstep

	moveq	#-1,d4
	neg.l	d2
posstep:
	moveq	#2,d3
spreadloop:
	lsl.l	#2,d3
	move.l	cols(a5,d3.w),d7
	move.l	0(a1,d3.w),d0
	sub.l	d7,d0
	swap	d0
	clr.w	d0
	move.l	d2,d1
	move.l	_UtilityBase(a4),a6
	jsr	_LVOSDivMod32(a6)	; ((rgb2[i] - rgb[i]) << 16) / steps
	move.l	d0,0(a3,d3.w)
	swap	d7
	move.l	d7,0(a2,d3.w)		; rgb[i] = glob->cols[i] << 16
	lsr.l	#2,d3
	dbf	d3,spreadloop
	move.l	#$8000,d7		; d7 = .5 (16 bits fixed point)
spreadloop2:
	move.l	vp(a5),a0
	move.l	(a2),d1
	add.l	d7,d1
	swap	d1
	move.l	4(a2),d2
	add.l	d7,d2
	swap	d2
	move.l	8(a2),d3
	add.l	d7,d3
	swap	d3
	move.l	_GfxBase(a4),a6
	moveq	#39,d0
	cmp.w	LIB_VERSION(a6),d0
	bls.s	.okkick39

	move.l	d5,d0
	jsr	_LVOSetRGB4(a6)
	bra.b	.cont

.okkick39:
	move.l	d4,-(a7)
	move.l	redbits(a5),d4
	move.l	d1,d0
	bsr	_MakeColVal
	move.l	d0,d1
	move.l	greenbits(a5),d4
	move.l	d2,d0
	bsr	_MakeColVal
	move.l	d0,d2
	move.l	bluebits(a5),d4
	move.l	d3,d0
	bsr	_MakeColVal
	move.l	d0,d3
	move.l	vp(a5),a0
	move.l	d5,d0
	jsr	_LVOSetRGB32(a6)
	move.l	(a7)+,d4
.cont:
	move.l	(a3),d0
	add.l	d0,(a2)
	move.l	4(a3),d0
	add.l	d0,4(a2)
	move.l	8(a3),d0
	add.l	d0,8(a2)
	add.l	d4,d5
	cmp.l	d5,d6
	bne.b	spreadloop2
nospread:
	lea	24(a7),a7
	movem.l	(a7)+,d2-d7/a2-a6
	rts

;_LVORawDoFmt	equ	-$20a

* if you want to count the newlines and the number of chars call DoFmt with
* buff == NULL and with ptr pointing to an array of 2 longs. array[0] will
* hold the number of lines, array[1] the number of chars.
* Initialize array[1] and array[0] to 1!

* void DofmtCount (char *fmt, APTR args, ULONG *ptr, int mode);
*                  A0         A1         A3          D0

* void Dofmt (char *buff, char *fmt, APTR args);
*             A3          A0         A1

* void __stdargs DofmtArgs (char *buff, char *fmt,...);
*

_DofmtArgs:
	movem.l	a2/a3/a6,-(a7)
	move.l	4+12(a7),a3
	move.l	8+12(a7),a0
	lea	12+12(a7),a1
	bra.b	dofmtargs

_DofmtCount:
	movem.l a2/a3/a6,-(a7)
	moveq	#1,d1
	move.l d1,(a3)+
	move.l d1,(a3)
	tst.l	d0
	bne.b	seperatorbar

	lea	CountNewLinesAndChars(PC),a2
	bra.b	dofmt

seperatorbar:
	lea	CountBarsAndChars(PC),a2
	bra.b	dofmt

_Dofmt:
	movem.l	a2/a3/a6,-(a7)
dofmtargs:
	lea	PutCharInBuff(PC),a2
dofmt:
	move.l	($4).w,a6
	jsr	_LVORawDoFmt(a6)
	movem.l	(a7)+,a2/a3/a6
	rts

PutCharInBuff:
	move.b	d0,(a3)+
	rts

CountBarsAndChars:
	cmp.b	#'|',d0
	bne.b	nobar

	bra.b	isbar

CountNewLinesAndChars:
	cmp.b	#10,d0
	bne.b	nonewline

isbar:
	addq.l #1,-4(a3)
nonewline:
nobar:
	addq.l #1,(a3)
	rts

* void FillBarTable (char **table, char *buff)
*                    A1            A0

* void FillNewLineTable (char **table, char *buff)
*                        A1            A0

_FillBarTable:
	move.b	#'|',d1
	bra.b	filltable
_FillNewLineTable:
	move.b	#$0a,d1
filltable:
	move.l	a0,(a1)+
loopnl:
	move.b	(a0)+,d0
	beq.b	done

	cmp.b	d1,d0
	bne.b	loopnl

	clr.b	-1(a0)
	move.l	a0,(a1)+
	bra.b	loopnl
done:
	rts

* ULONG __asm LoopReqHandler (register __a1 struct rtHandlerInfo *)
* D0                                        A1

	XREF	rtReqHandlerA

_LoopReqHandler:
	move.l	a2,-(a7)
loophandler:
	moveq	#0,d0
	tst.l	rthi_DoNotWait(a1)
	bne.b	nowait

	movem.l	a1/a6,-(a7)
	move.l	rthi_WaitMask(a1),d0
	move.l	($4).w,a6
	jsr	_LVOWait(a6)
	movem.l	(a7)+,a1/a6
	; d0 holds sigs
nowait:
	sub.l	a0,a0
	sub.l	a2,a2
	move.l	a1,-(a7)
	jsr	rtReqHandlerA(PC)
	move.l	(a7)+,a1
	cmp.l	#CALL_HANDLER,d0
	beq.b	loophandler

	move.l	(a7)+,a2
	rts

* ULONG __stdargs CallHook (struct Hook *, APTR,...);

_CallHook:
	movem.l	a2/a3,-(a7)
	move.l	4+8(a7),a0
	move.l	8+8(a7),a2
	lea	12+8(a7),a1
	move.l	h_Entry(a0),a3
	jsr	(a3)
	movem.l	(a7)+,a2/a3
	rts

* void StrCat (UBYTE *, UBYTE *);
*		a0	a1
_StrCat:
	tst.b	(a0)+
	bne.b	_StrCat

	subq.l	#1,a0
catloop:
	move.b	(a1)+,(a0)+
	bne.b	catloop
	rts

	IFD	DICE_HACK

	XREF	_IntuitionBase

	XREF	_MyGetString
	XDEF	_GetString

* ULONG __saveds GetString (
*	register __a1 UBYTE *stringbuff,		/* str in case of rtEZRequestA */
*	register __d0 int maxlen,			/* args in case of rtEZRequestA */
*	register __a2 char *title,			/* gadfmt in case of rtEZRequestA */
*	register __d1 ULONG checksum,
*	register __d2 ULONG *value,
*	register __d3 int mode,
*	register __d4 struct rtReqInfo *reqinfo,
*	register __a0 struct TagItem *taglist)
*

_GetString:
	move.l	a0,-(sp)
	movem.l	d1-d4,-(sp)
	move.l	a2,-(sp)
	move.l	d0,-(sp)
	move.l	a1,-(sp)
	bsr	_MyGetString
	lea	8*4(sp),sp
	rts

	ENDC

	END
