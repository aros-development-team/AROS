
	SECTION "text",CODE

	INCLUDE "exec/types.i"
	INCLUDE "exec/funcdef.i"
	INCLUDE "exec/exec_lib.i"
	INCLUDE "intuition/intuitionbase.i"
	INCLUDE "graphics/rastport.i"
	INCLUDE "graphics/text.i"
	INCLUDE "graphics/gfxbase.i"
	INCLUDE "lvo/graphics_lib.i"

	INCLUDE "lvo/utility_lib.i"
	INCLUDE "utility/hooks.i"

	INCLUDE "libraries/gadtools.i"

	INCLUDE "intuition/intuition.i"
	INCLUDE "lvo/intuition_lib.i"
	INCLUDE "intuition/classes.i"
	INCLUDE "intuition/classusr.i"
	INCLUDE "intuition/imageclass.i"

	INCLUDE "libraries/reqtools.i"
	INCLUDE "lvo/reqtools_lib.i"

	INCLUDE	"boopsigads.i"

	XDEF _myBoopsiDispatch
	XDEF @ShortDelay
	XDEF _NewObject
	XDEF @KeyFromStr

	XREF _LinkerDB
	XREF _GfxBase
	XREF _GadToolsBase
	XREF _UtilityBase
	XREF _IntuitionBase

_LVODrawBevelBox			equ		-$78

* int __regargs CatStrLen (char *string);
* D0                       A0
;@CatStrLen:
;	moveq		#'_',d1
;	moveq		#-1,d0
;.nextchar:
;	addq.l	#1,d0
;.nextcharnoadd:
;	tst.b		(a0)
;	beq.b		.endlen
;	cmp.b		(a0)+,d1
;	bne.b		.nextchar
;	moveq		#0,d1
;	bra.b		.nextcharnoadd
;.endlen:
;	rts

* char __regargs KeyFromStr (char *string, char underchar);
* D0                         A0            D0
@KeyFromStr:
	tst.b		(a0)
	beq.b		.nounderscore
	cmp.b		(a0)+,d0
	bne.b		@KeyFromStr
	move.b	(a0),d0
	move.l	a6,-(a7)
	move.l	_UtilityBase(a4),a6
	jsr		_LVOToUpper(a6)
	move.l	(a7)+,a6
	rts
.nounderscore:
	moveq		#0,d0
	rts

@ShortDelay:
	movem.l	d2/a6,-(a7)
	move.l	_GfxBase(a4),a6
	moveq		#5,d2
waitloop:
	jsr		_LVOWaitTOF(a6)
	dbf		d2,waitloop
	movem.l	(a7)+,d2/a6
	rts

;_LVONewObjectA	EQU	-636

_NewObject:
	movem.l	a2/a6,-(a7)
	movem.l	8+4(a7),a0/a1
	lea		8+12(a7),a2
	move.l	_IntuitionBase(a4),a6
	jsr		_LVONewObjectA(a6)
	movem.l	(a7)+,a2/a6
	rts

* myBoopsiDispatch (Class *cl, struct Image *im, struct impDraw *msg)
*                   A0         A2                A1
_myBoopsiDispatch:
	movem.l	d1-d7/a0-a6,-(a7)
	lea	_LinkerDB,a4
	move.w	cl_InstOffset(a0),d7
	move.l	_GfxBase(a4),a6

	cmp.l		#IM_DRAW,(a1)
	beq		im_draw

	* This is DoSuperMethod for prepackaged "message" packets
	move.l	a1,-(a7)
	move.l	a2,d0
	beq.s		cmnullreturn
	move.l	a0,d0
	beq.s		cmnullreturn
	move.l	h_SIZEOF+4(a0),a0			; substitute superclass
	; --- performs call to hook in A0 and restores a2
cminvoke:
	pea.l		cmreturn(pc)
	move.l	h_Entry(a0),-(a7)
	rts
cmnullreturn:
	moveq		#0,d0
cmreturn:
	move.l	(a7)+,a1

	cmp.l		#OM_NEW,(a1)
	bne		enddispatch

****** OM_NEW ******

om_new:
	tst.l		d0								; error ?
	beq		enddispatch
	move.l	d0,a2							; DSM returned image pointer

	* Copy over InitData structure
	move.l	ig_ImageData(a2),a0
	lea		lod_IData(a2,d7.w),a1
	moveq		#(idata_SIZEOF/4)-1,d0
copyidata:
	move.l	(a0)+,(a1)+
	dbra		d0,copyidata

	move.l	lod_IData+idata_Label(a2,d7.w),a1
	move.l	lod_IData+idata_TextAttr(a2,d7.w),a0
	lea		lod_IData+idata_Underscore+3(a2,d7.w),a3
	bsr		_myTextLength
	move.w	d0,ig_Width(a2)

	* Calculate text position if we have a gadget
	move.l	lod_IData+idata_Gadget(a2,d7.w),a0
	move.l	a0,d0
	beq.b		nogad3

	* im->LeftEdge = (gad->Width - im->Width) / 2;
	move.w	gg_Width(a0),d0
	sub.w		ig_Width(a2),d0
	asr.w		#1,d0
	move.w	d0,ig_LeftEdge(a2)

	* im->TopEdge = (gad->Height - im->Height) / 2;
	move.w	gg_Height(a0),d0
	sub.w		ig_Height(a2),d0
	asr.w		#1,d0
	move.w	d0,ig_TopEdge(a2)
nogad3:

	move.l	a2,ig_ImageData(a2)
	move.l	#BUTTON_MAGIC_LONGWORD,ig_Width(a2)

	move.l	a2,d0
	bra		enddispatch

****** IM_DRAW ******

im_draw:
	move.l	a1,a5
	move.l	impd_RPort(a5),a1			; rp

	move.l	a1,-(a7)						; push rastport

	* Set drawmode to JAM1
	moveq		#RP_JAM1,d0
	jsr		_LVOSetDrMd(a6)

	* Open and set text font (or set default font)
	move.l	lod_IData+idata_TextAttr(a2,d7.w),a0
	moveq		#0,d5
	move.b	ta_Style(a0),d5
	jsr		_LVOOpenFont(a6)
	move.l	d0,d6							; d6 = font
	bne.b		okfont
	move.l	gb_DefaultFont(a6),d0
okfont:
	move.l	d0,a0
	move.l	(a7),a1
	jsr		_LVOSetFont(a6)

	* Set style of font (bold, underlined,...)
	move.l	d5,d0
	move.l	d5,d1
	move.l	(a7),a1
	jsr		_LVOSetSoftStyle(a6)
	move.l	(a7)+,a1						; pos rastport

	move.w	impd_OffsetY(a5),d1		; x offset
	move.w	impd_OffsetX(a5),d0		; y offset for image only
	move.l	impd_DrInfo(a5),a0
	move.l	dri_Pens(a0),a0			; a0 points to pens
	move.l	a0,d2
	bne.b		pensok
	lea		defaultpens(PC),a0
pensok:

	* is image connected to a gadget ?
	move.l	lod_IData+idata_Gadget(a2,d7.w),a3	; gadget
	move.l	a3,d2
	beq.b		nogad1

	* Get size of gadget container
	move.w	gg_TopEdge(a3),d1
	move.w	gg_LeftEdge(a3),d0
	moveq		#0,d3
	move.w	gg_Height(a3),d3			; height
	moveq		#0,d2
	move.w	gg_Width(a3),d2			; width

	move.l	impd_State(a5),d4			; d4 = impd_State

	movem.l	d0-d3/a1,-(a7)				; push gadget coords + rastport
												; will be popped when bevelbox is drawn!

	* draw rectangle in BACKGROUNDPEN or FILLPEN
	movem.l	d0-d3/a0-a1,-(a7)
	add.w		#FILLPEN*2,a0
	subq.l	#IDS_SELECTED,d4			; d4 = !selected
	beq.b		selected
	addq.l	#(BACKGROUNDPEN-FILLPEN)*2,a0
selected:
	move.w	(a0),d0
	jsr		_LVOSetAPen(a6)			; set pen to FILLPEN
	movem.l	(a7),d0-d3/a0-a1
	add.l		d0,d2
	subq.l	#1,d2							; xmax
	add.l		d1,d3
	subq.l	#1,d3							; ymax
	jsr		_LVORectFill(a6)			; draw background rectangle
	movem.l	(a7),d0-d3/a0-a1

	* calculate gadget text pen
	moveq		#TEXTPEN,d0
	tst.l		d4
	bne.b		notsel2
	moveq		#FILLTEXTPEN,d0
notsel2:
	move.b	d0,ig_PlanePick(a2)
	movem.l	(a7)+,d0-d3/a0-a1
nogad1:

	* now we have d0 = xpos, d1 = ypos, a1 = rp, a0 = pens
	*				  a5 = msg, a2 = image

	move.l	lod_IData+idata_Label(a2,d7.w),a3
	add.w		ig_LeftEdge(a2),d0
	add.w		ig_TopEdge(a2),d1

	* set front pen
	movem.l	d0-d1/a1,-(a7)
	moveq		#0,d0
	move.b	ig_PlanePick(a2),d0
	add.l		d0,d0
	add.l		d0,a0
	move.w	(a0),d0
	jsr		_LVOSetAPen(a6)
	movem.l	(a7),d0-d1/a1

	* move to text position
	add.w		rp_TxBaseline(a1),d1
	jsr		_LVOMove(a6)
	movem.l	(a7),d0-d1/a1

	* draw first part of text (entire text if no underscore)
	move.l	a3,a0
	move.w	lod_UnderOff(a2,d7.w),d5
	beq.b		notextbeforeunderscore
	move.w	d5,d0
	jsr		_LVOText(a6)
notextbeforeunderscore:
	movem.l	(a7)+,d0-d1/a1

	* is there an underscore in text ?
	tst.b		ig_PlaneOnOff(a2)
	beq.b		nounderscore

	* First draw rest of text
	movem.l	d0-d1/a1,-(a7)
	add.w		d5,a3
	addq.w	#1,a3
	move.w	rp_cp_x(a1),d5
	move.w	lod_RestLen(a2,d7.w),d0
	move.l	a3,a0
	jsr		_LVOText(a6)
	movem.l	(a7)+,d0-d1/a1

	* Draw underscore
	move.w	d5,d0
	add.w		lod_UnderY(a2,d7.w),d1
	movem.l	d0-d1/a1,-(a7)
	jsr		_LVOMove(a6)
	movem.l	(a7)+,d0-d1/a1
	add.w		lod_UnderWidth(a2,d7.w),d0
	subq.w	#1,d0
	jsr		_LVODraw(a6)
nounderscore:

	tst.l		lod_IData+idata_Gadget(a2,d7.w)	; gadget
	beq.b		nogad2

	* draw bevelbox around gadget container (recessed if selected)
	movem.l	(a7)+,d0-d3/a0				; pop gadget coords + rastport (in A0!)
	move.l	a7,a3
	clr.l		-(a7)							; TAG_DONE
	tst.l		d4
	bne.b		notrecessed
	subq.w	#4,a7
	pea		GTBB_Recessed				; recessed box
notrecessed:
	move.l	lod_IData+idata_VisualInfo(a2,d7.w),-(a7)	; visinfo
	pea		GT_VisualInfo
	move.l	a7,a1							; tag array
	move.l	_GadToolsBase(a4),a6
	jsr		_LVODrawBevelBox(a6)
	move.l	a3,a7
nogad2:

	* close font we opened
	tst.l		d6								; font opened ?
	beq.b		nofont
	move.l	d6,a1
	move.l	_GfxBase(a4),a6
	jsr		_LVOCloseFont(a6)
nofont:

enddispatch:
	movem.l	(a7)+,d1-d7/a0-a6
	rts


	XDEF	_myTextLength

* pixellen = myTextLength (char *str, struct TextAttr *attr,
*                                              UBYTE *underscore, ULONG do_lod);
* D0                       A1         A0       A3                 D7
* (if D7 is non-zero => A2 must be image pointer!)

_myTextLength:
	movem.l	d2-d5/d7/a2/a5/a6,-(a7)
	sub.w		#rp_SIZEOF,a7
	move.l	a7,a5							; a5 points to RastPort
	move.l	_GfxBase(a4),a6

	move.l	a1,-(a7)

	move.l	a0,-(a7)

	* Initialize the RastPort
	move.l	a5,a1
	jsr		_LVOInitRastPort(a6)

	move.l	(a7)+,a0

	* Open and set text font (if supplied)
	jsr		_LVOOpenFont(a6)
	move.l	d0,d3							; d3 = font
	bne.b		okfont2
	move.l	gb_DefaultFont(a6),d0
okfont2:
	move.l	d0,a0
	move.l	a5,a1
	jsr		_LVOSetFont(a6)

	moveq		#0,d0
	move.l	(a7)+,d4						; d4 is label
	beq.b		no_label

	move.l	d4,a0
	moveq		#-1,d5
strlen:
	addq.l	#1,d5
	tst.b		(a0)+
	bne.b		strlen						; d5 is strlen(label)

	* Image and object data variables
	tst.l		d7
	beq.b		no_lod
	clr.b		ig_PlaneOnOff(a2)
	move.w	d5,lod_UnderOff(a2,d7.w)
	move.w	rp_TxHeight(a5),ig_Height(a2)
no_lod:

	* Set the image width and height
	move.l	d5,d0
	move.l	d4,a0
	move.l	a5,a1
	jsr		_LVOTextLength(a6)

	move.l	d0,-(a7)

	* Search for '_' in string
	move.b	(a3),d1
	moveq		#-1,d0
	move.l	d4,a0
strloop:
	tst.b		(a0)
	beq.b		nounderscore2
	addq.l	#1,d0
	cmp.b		(a0)+,d1
	bne.b		strloop

	tst.l		d7
	beq.b		no_lod2

	move.b	(a0),ig_PlaneOnOff(a2)		; store code of underscored key
	move.w	d0,lod_UnderOff(a2,d7.w)	; store offset of underscore
	sub.w		d0,d5
	subq.w	#1,d5
	move.w	d5,lod_RestLen(a2,d7.w)		; store len of remaining string
	move.w	rp_TxBaseline(a5),d0
	addq.w	#2,d0
	move.w	d0,lod_UnderY(a2,d7.w)		; Y position of underscore

	move.l	a5,a1
	moveq		#1,d0
	jsr		_LVOTextLength(a6)
	move.w	d0,lod_UnderWidth(a2,d7.w)
no_lod2:

	move.l	a5,a1
	moveq		#1,d0
	move.l	a3,a0
	jsr		_LVOTextLength(a6)
	sub.l		d0,(a7)							; subtract from pixel length
nounderscore2:

	tst.l		d3
	beq.b		nofont2
	move.l	d3,a1
	jsr		_LVOCloseFont(a6)
nofont2:

	move.l	(a7)+,d0

no_label:
	add.w		#rp_SIZEOF,a7				; remove rastport from stack
	movem.l	(a7)+,d2-d5/d7/a2/a5/a6
	rts

defaultpens:
	dc.w		1,0,1,2,1,3,1,0,2

	END
