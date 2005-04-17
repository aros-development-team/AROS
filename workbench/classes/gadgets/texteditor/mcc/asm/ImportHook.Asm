

	IncDir	Includes:

	Include	LVO/DOS_lib.i
	Include	LVO/Exec_lib.i
	Include	Exec/Memory.I
	Include	Exec/Types.I


	STRUCTURE	HookMsg,0
		APTR	HM_Data
		APTR	HM_LineNode
		APTR	HM_PoolHandle
		LONG	HM_ImportWrap
		LABEL	SizeOf_HookMsg

	STRUCTURE	LineNode,0
		APTR	LN_Contents
		LONG	LN_Length
		APTR	LN_Styles
		APTR	LN_Colors
		BOOL	LN_Color
		WORD	LN_Flow
		WORD	LN_Separator

	UNDERLINE=1
	BOLD=2
	ITALIC=4
	COLOR=8

; a0 = char *text
; a1 = struct LineNode *
; a2 = void *poolhandle
; d5 = int   highlightcolor

	XRef	_DOSBase
	XDef	_ImPlainHook
_ImPlainHook:	dc.l	0,0,PlainParse

PlainParse:	move.l	HM_Data(a1),a0
	move.l	HM_PoolHandle(a1),a2
	move.l	HM_ImportWrap(a1),d5
	move.l	HM_LineNode(a1),a1

	lea.l	LineBuffer(pc),a4
	lea.l	JumpTablePlain(pc),a5
	moveq	#0,d0
	moveq	#0,d1
	moveq	#0,d2

.SearchForCode	cmp.b	#27,(a0)
	bne.w	MIMEParse|.ReUse
	cmp.b	#"l",1(a0)
	bne.b	.notleft
	move.w	#0,LN_Flow(a1)
	addq.w	#2,a0
	bra.b	.SearchForCode
.notleft	cmp.b	#"c",1(a0)
	bne.b	.notcenter
	move.w	#1,LN_Flow(a1)
	addq.w	#2,a0
	bra.b	.SearchForCode
.notcenter	cmp.b	#"r",1(a0)
	bne.b	.notright
	move.w	#2,LN_Flow(a1)
	addq.w	#2,a0
	bra.b	.SearchForCode
.notright	cmp.b	#"[",1(a0)
	bne.w	MIMEParse|.ReUse
	cmp.b	#"s",2(a0)
	bne.w	MIMEParse|.ReUse
	cmp.b	#":",3(a0)
	bne.w	MIMEParse|.ReUse
	addq.w	#4,a0

	movem.l	d0-d2/a4-a5,-(sp)

	lea.l	(a0),a5
	lea.l	(a1),a4

	move.l	a5,d1
	move.l	#SepValue,d2
	move.l	_DOSBase,a6
	jsr	_LVOStrToLong(a6)
	tst.l	d0
	bmi.b	.none
	add.l	d0,a5
	move.w	SepValue+2,LN_Separator(a4)
.none	addq.w	#1,a5

	lea.l	(a5),a0
	lea.l	(a4),a1

	movem.l	(sp)+,d0-d2/a4-a5
	bra.w	.SearchForCode

SepValue:	dc.l	0
JumpTablePlain:	dc.l	Ignore, Tab, Normal, Normal, EndOfFile, EndOfLine, Normal, ESC, Normal

	XDef	_ImEMailHook
_ImEMailHook:	dc.l	0,0,EMailParse

EMailParse:	move.l	HM_Data(a1),a0
	move.l	HM_PoolHandle(a1),a2
	move.l	HM_ImportWrap(a1),d5
	move.l	HM_LineNode(a1),a1

	lea.l	LineBuffer(pc),a4
	lea.l	JumpTableEMail(pc),a5
	moveq	#0,d0
	moveq	#0,d1
	moveq	#0,d2

	bra.w	MIMEParse|.EmailReUse

	XDef	_ImMIMEQuoteHook
_ImMIMEQuoteHook:	dc.l	0,0,MIMEQuoteParse

JumpTableEMail:	dc.l	Ignore, Tab, Normal, Normal, EndOfFile, EndOfLine, Style, ESC, Quote

MIMEQuoteParse:	move.l	HM_Data(a1),a0
	move.l	HM_PoolHandle(a1),a2
	move.l	HM_ImportWrap(a1),d5
	move.l	HM_LineNode(a1),a1

	lea.l	LineBuffer(pc),a4
	lea.l	JumpTableMIME(pc),a5
	moveq	#0,d0
	moveq	#2,d1
	moveq	#0,d2
	move.w	#"> ",(a4)+	; Quote each line
	move.w	#1,LN_Color(a1)	; Highlight this line
	bra.w	MIMEParse|.ReUse

	XDef	_ImMIMEHook
_ImMIMEHook:	dc.l	0,0,MIMEParse

MIMEParse:	move.l	HM_Data(a1),a0
	move.l	HM_PoolHandle(a1),a2
	move.l	HM_ImportWrap(a1),d5
	move.l	HM_LineNode(a1),a1

	lea.l	LineBuffer(pc),a4
	lea.l	JumpTableMIME(pc),a5
	moveq	#0,d0
	moveq	#0,d1
	moveq	#0,d2

.EmailReUse	lea.l	(a0),a3
	moveq	#2,d3

	clr.w	LN_Flow(a1)
	cmp.b	#"<",(a3)+
	bne.b	.SearchQuote
	cmp.b	#"t",(a3)
	bne.b	.TestBar
	or.w	#16,d3
	addq.w	#1,a3
.TestBar	cmp.b	#"s",(a3)+
	bne.b	.SearchQuote
	cmp.b	#"b",(a3)+
	bne.b	.SearchQuote
	cmp.b	#">",(a3)+
	bne.b	.SearchQuote
	lea.l	(a3),a0
	move.w	d3,LN_Separator(a1)	; Create separator
	move.w	#1,LN_Flow(a1)
	bra.b	.ReUse

.SearchQuote	clr.w	IndentLevel
	lea.l	(a0),a3

	move.b	(a3),d2
	bsr.w	ChkAlphaNum
	tst.w	d3
	bne.b	.PreFindQuote

	tst.w	CurrentHCol
	beq.b	.PreFindQuote
	move.w	#1,LN_Color(a1)
	bra.w	.ReUse

.PreFindQuote	move.w	#0,CurrentHCol
	move.w	#0,LN_Color(a1)

;	moveq	#4,d3
;.FindQuote	cmp.b	#">",(a3)
;	beq.b	.Highlight
;	cmp.b	#10,(a3)+
;	dbeq	d3,.FindQuote
;.End	move.w	#0,CurrentHCol
;	bra.b	.ReUse
;.Highlight	move.w	#1,CurrentHCol
;	move.w	#1,LN_Color(a1)	; Highlight this line

.ReUse	lea.l	ParseMap(pc),a3
	move.l	#StyleBuffer,StyleBufferPnt
	move.l	#ColorBuffer,ColorBufferPnt

	clr.w	CurrentStyle

	cmp.l	#1024,d5
	blo.b	.loop
	move.l	#1023,d5
.fine	cmp.l	#4,d5
	bhi.b	.loop
	moveq	#5,d5

.loop	move.b	(a0)+,d0
	move.b	(a3,d0.w),d2
	move.l	(a5,d2.w),a6
	jsr	(a6)
	cmp.w	d5,d1		; WrapBorder
	blo.b	.loop

	movem.l	d1/a0/a4,-(sp)
	addq.w	#1,a4

.findWord	cmp.b	#" ",-(a4)
	beq.b	.foundNewWord
	subq.w	#1,a0
	subq.w	#1,d1
	cmp.w	#1,d1
	bhi.b	.findWord
	movem.l	(sp)+,d1/a0/a4
	bra.b	.skip
.foundNewWord	addq.w	#1,d1
	addq.w	#1,a0
	addq.w	#1,a4
	lea.l	12(sp),sp
.skip	jsr	EndOfLine

JumpTableMIME:	dc.l	Ignore, Tab, Normal, MIME, EndOfFile, EndOfLine, Style, ESC, Quote

Tab:	moveq	#3,d7
.Space	move.b	#" ",(a4)+
	dbra	d7,.Space
	addq.w	#4,d1
Ignore:	rts
Normal:	move.b	d0,(a4)+
	addq	#1,d1
	rts

Style:	cmp.b	#"/",d0
	blo.b	.Bold
	bgt.b	.Underline
.Italic	move.w	#ITALIC,d7
	bra.b	.Done
.Bold	cmp.w	#"#",d0
	bne.b	.IsBold

	move.w	#COLOR,d7
	tst.w	Color
	beq.b	.StyleStart

	clr.w	Color
	move.l	ColorBufferPnt(pc),a6
	move.w	#1,(a6)
	add.w	d1,(a6)+
	clr.w	(a6)+

	tst.w	CurrentHCol
	beq	.equal
	btst.b	#0,IndentLevel+1
	bne	.equal
	move.w	#1,(a6)
	add.w	d1,(a6)+
	move.w	#8,(a6)+

.equal	move.l	a6,ColorBufferPnt
	rts

.IsBold	move.w	#BOLD,d7
	bra.b	.Done
.Underline	move.w	#UNDERLINE,d7

.Done	move.w	CurrentStyle,d6
	and.w	d7,d6
	beq.b	.StyleStart

	not.w	d7
	and.w	d7,CurrentStyle

	move.l	StyleBufferPnt(pc),a6
	move.w	#1,(a6)
	add.w	d1,(a6)+
	move.w	d7,(a6)+
	move.l	a6,StyleBufferPnt
	rts

.StyleStart	cmp.b	(a0),d0
	beq.w	Normal
	tst.w	d1
	beq.b	.SkipThisCheck
	cmp.b	-2(a0),d0
	beq.w	Normal

	move.b	-2(a0),d2
	bsr.w	ChkAlphaNum	; Preceding char must
	tst.b	d3		; not be alpha-numeric!
	beq.w	Normal

.SkipThisCheck	move.l	a0,-(sp)
	moveq	#-1,d3
.findEnder	move.b	(a0)+,d2
	cmp.b	d2,d0
	beq.b	.ItsGood

	tst.b	d3
	beq.b	.FoundAlpha
	bsr.w	ChkAlphaNum

.FoundAlpha	cmp.b	#10,d2
	bne.b	.findEnder
	cmp.b	#"=",-1(a0)
	beq.b	.findEnder
	move.l	(sp)+,a0
	bra.w	Normal

.ItsGood	move.l	(sp)+,a0
	tst.b	d3
	bne.w	Normal

	cmp.w	#COLOR,d7
	bne.b	.Style

	st	Color
	move.l	ColorBufferPnt(pc),a6
	move.w	#1,(a6)
	add.w	d1,(a6)+
	move.w	#7,(a6)+
	move.l	a6,ColorBufferPnt
	rts

.Style	or.w	d7,CurrentStyle
	move.l	StyleBufferPnt(pc),a6
	move.w	#1,(a6)
	add.w	d1,(a6)+
	move.w	d7,(a6)+
	move.l	a6,StyleBufferPnt
	rts

MIME:	move.b	(a0)+,d3
	cmp.b	#10,d3
	beq.b	.Skip
	sub.b	#"0",d3
	cmp.b	#10,d3
	blo.b	.number1
	sub.b	#"A"-"0"-10,d3
	bmi.b	.Skip
.number1	lsl.b	#4,d3
	move.b	(a0)+,d4
	sub.b	#"0",d4
	cmp.b	#10,d4
	blo.b	.number2
	sub.b	#"A"-"0"-10,d4
	bmi.b	.Skip
.number2	or.b	d4,d3
	move.b	d3,(a4)+
	addq.w	#1,d1
.Skip	rts

EndOfFile:	move.l	#.ClearA0,(sp)
	bsr.b	EndOfLine
.ClearA0:	moveq	#0,d0
	rts

EndOfLine:	move.b	#10,(a4)+
	clr.b	(a4)+
	bsr.w	ESC|.ResetStyle
	addq.w	#2,d1
	move.l	d1,LN_Length(a1)

	movem.l	d0-d1/a0-a1/a4,-(sp)

	lea.l	(a1),a4
	lea.l	(a2),a0
	move.l	d1,d0
	add.w	#12,d0
	move.l	$4.w,a6
	jsr	_LVOAllocPooled(a6)
	tst.l	d0
	beq.w	.NoStyles

	move.l	#LineBuffer,a0
	move.l	d0,a1
	move.l	LN_Length(a4),d0
	move.l	d0,(a1)
	add.l	#12,(a1)+
	move.l	a1,LN_Contents(a4)
	addq.l	#8,d0
	jsr	_LVOCopyMem(a6)
	subq.l	#1,LN_Length(a4)
;----------------------------------------------------------
	cmp.l	#ColorBuffer,ColorBufferPnt
	beq.b	.NoColors

	move.l	ColorBufferPnt,a0
	move.w	#-1,(a0)+
	move.l	a0,d0
	sub.l	#ColorBuffer,d0
	addq.l	#8,d0
	move.l	d0,-(sp)

	lea.l	(a2),a0
	move.l	$4.w,a6
	jsr	_LVOAllocPooled(a6)
	tst.l	d0
	beq.b	.NoStyles

	move.l	d0,a1
	move.l	(sp)+,d0
	move.l	d0,(a1)+
	move.l	a1,LN_Colors(a4)

	subq.l	#4,d0
	lea.l	ColorBuffer,a0
	jsr	_LVOCopyMem(a6)

.NoColors	cmp.l	#StyleBuffer,StyleBufferPnt
	beq.b	.NoStyles

	move.l	StyleBufferPnt,a0
	move.w	#-1,(a0)+
	move.l	a0,d0
	sub.l	#StyleBuffer,d0
	addq.l	#8,d0
	move.l	d0,-(sp)

	lea.l	(a2),a0
	move.l	$4.w,a6
	jsr	_LVOAllocPooled(a6)
	tst.l	d0
	beq.b	.NoStyles

	move.l	d0,a1
	move.l	(sp)+,d0
	move.l	d0,(a1)+
	move.l	a1,LN_Styles(a4)

	subq.l	#4,d0
	lea.l	StyleBuffer,a0
	jsr	_LVOCopyMem(a6)

.NoStyles	movem.l	(sp)+,d0-d1/a0-a1/a4
	addq.w	#4,sp
	move.l	a0,d0
	rts

ESC:	move.b	(a0)+,d0
	cmp.b	#"h",d0
	bne.b	.CheckColor
	move.w	#1,LN_Color(a1)
	bra.w	.End

.CheckColor	cmp.b	#"p",d0
	bne.b	.CheckStyle
	cmp.b	#"[",(a0)+
	bne.w	.End

	movem.l	d0-d2/a4-a5,-(sp)

	lea.l	(a0),a5
	lea.l	(a1),a4

	move.l	a5,d1
	move.l	#SepValue,d2
	move.l	_DOSBase,a6
	jsr	_LVOStrToLong(a6)
	tst.l	d0
	bmi.b	.none
	add.l	d0,a5
	move.l	SepValue,d7

.none	addq.w	#1,a5

	lea.l	(a5),a0
	lea.l	(a4),a1

	movem.l	(sp)+,d0-d2/a4-a5

	move.l	ColorBufferPnt(pc),a6
	move.w	#1,(a6)
	add.w	d1,(a6)+
	move.w	d7,(a6)+
	move.l	a6,ColorBufferPnt
	rts

.CheckStyle	move.w	#ITALIC,d7
	cmp.b	#"i",d0
	beq.b	.Done
	move.w	#BOLD,d7
	cmp.b	#"b",d0
	beq.b	.Done
	move.w	#UNDERLINE,d7
	cmp.b	#"u",d0
	beq.b	.Done
	cmp.b	#"n",d0
	bne.b	.End

.ResetStyle	move.w	CurrentStyle(pc),d6
	move.w	d6,d7
	and.w	#BOLD,d7
	beq.b	.SkipBold
	not.w	d7
	bsr.b	.Done
.SkipBold	move.w	d6,d7
	and.w	#ITALIC,d7
	beq.b	.SkipItalic
	not.w	d7
	bsr.b	.Done
.SkipItalic	move.w	d6,d7
	and.w	#UNDERLINE,d7
	beq.b	.StylesReset
	not.w	d7
	bsr.b	.Done
.StylesReset	clr.w	CurrentStyle
	rts

.Done	or.w	d7,CurrentStyle
	move.l	StyleBufferPnt(pc),a6
	move.w	#1,(a6)
	add.w	d1,(a6)+
	move.w	d7,(a6)+
	move.l	a6,StyleBufferPnt

.End	rts

Quote:	cmp.w	#8,d1
	bhi	.noquote

	cmp.w	#1,IndentLevel
	blo	.skip

	move.l	ColorBufferPnt(pc),a6
	move.w	#1,(a6)
	add.w	d1,(a6)+
	clr.w	(a6)+
	btst.b	#0,IndentLevel+1
	beq	.equal
	move.w	#8,-2(a6)
.equal	move.l	a6,ColorBufferPnt

.skip	move.w	#1,CurrentHCol
	move.w	#1,LN_Color(a1)	; Highlight this line
	addq.w	#1,IndentLevel

.noquote	move.b	d0,(a4)+
	addq	#1,d1
	rts

ChkAlphaNum:	moveq	#0,d3
	cmp.b	#"0",d2
	blo.b	.Neg
	cmp.b	#"9",d2
	bls.b	.Pos
	cmp.b	#"A",d2
	blo.b	.Neg
	cmp.b	#"Z",d2
	bls.b	.Pos
	cmp.b	#"a",d2
	blo.b	.Neg
	cmp.b	#"z",d2
	bls.b	.Pos
	cmp.b	#"À",d2
	blo.b	.Neg
	cmp.b	#215,d2
	beq.b	.Neg
	cmp.b	#247,d2
	bne.b	.Pos
.Neg	st	d3
.Pos	rts

; 0 = Not pastable
; 4 = TAB
; 8 = Normalchar
; 12 = MIME char
; 16 = End of file
; 20 = End of line
; 24 = Email style (*, _ & /)
; 28 = ESC (introducing style)
; 32 = Quote level (>)

ParseMap:	dc.b	16	;       0
	dcb.b	8,0	;     1-8
	dc.b	4	;   <TAB>   (09)
	dc.b	20	;    <LF>   (10)
	dcb.b	2,0	;   11-12
	dc.b	0	;    <CR>   (13)
	dcb.b	13,0	;   14-26
	dc.b	28	;   <ESC>   (27)
	dcb.b	4,0	;   28-31
	dcb.b	3,8	; " "-"""
	dc.b	24	;     "#"   (35)
	dcb.b	6,8	; "$"-")"
	dc.b	24	;     "*"   (42)
	dcb.b	4,8	; "+"-"."
	dc.b	24	;     "/"   (47)
	dcb.b	13,8	; "0"-"<"
	dc.b	12	;     "="   (61)
	dc.b	32	;     ">"   (62)
	dcb.b	32,8	; ">"-"^"
	dc.b	24	;     "_"   (95)
	dcb.b	31,8	; "`"-"~"
	dcb.b	33,0	; 127-159
	dcb.b	96,8	; " "-"ÿ"
	dc.w	-1

IndentLevel:	dc.w	0
Color:	dc.w	0
StyleBufferPnt:	dc.l	0
ColorBufferPnt:	dc.l	0
LineBuffer:	dcb.b	1030
StyleBuffer:	dcb.w	500
ColorBuffer:	dcb.w	500
CurrentStyle:	dc.w	0
CurrentHCol:	dc.w	0

  "½      H                              #L