********************
* ReqTools library *
********************

	SECTION "text",CODE

	INCLUDE	"exec/types.i"
	INCLUDE	"exec/funcdef.i"
	INCLUDE	"lvo/exec_lib.i"
	INCLUDE	"exec/nodes.i"
	INCLUDE	"exec/lists.i"
	INCLUDE	"exec/alerts.i"
	INCLUDE	"exec/libraries.i"
	INCLUDE	"exec/memory.i"
	INCLUDE	"dos/dos.i"
	INCLUDE	"lvo/dos_lib.i"
	INCLUDE	"dos/var.i"
	INCLUDE	"graphics/gfxbase.i"
	INCLUDE	"intuition/intuition.i"
	INCLUDE	"intuition/classes.i"
	INCLUDE	"intuition/classusr.i"
	INCLUDE	"intuition/screens.i"
	INCLUDE	"lvo/intuition_lib.i"
	INCLUDE	"utility/hooks.i"

	INCLUDE	"libraries/reqtools.i"
	INCLUDE	"lvo/reqtools_lib.i"

	INCLUDE	"boopsigads.i"

	XREF _myBoopsiDispatch
	XREF _FreeReqToolsFonts
	XREF DataTable
	XREF __BSSLEN

	XDEF Init

Init:
	dc.l	ReqToolsBase_SIZE
	dc.l	FuncTable
	dc.l	DataTable
	dc.l	InitRoutine

dosName:
	dc.b "dos.library",0
intName:
	dc.b "intuition.library",0
gfxName:
	dc.b "graphics.library",0
gadName:
	dc.b "gadtools.library",0
utilName:
	dc.b "utility.library",0
layersName:
	dc.b "layers.library",0
localeName:
	dc.b "locale.library",0
consoleName:
	dc.b "console.device",0
imageclass:
	dc.b "imageclass",0
varname:
	dc.b "ReqTools.prefs",0
	ds.l	0
;	dc.l __BSSLEN			; we need this line for Blink to generate BSS!

	EVEN

*****************************************************************************

; D1 must hold default size, A1 is advanced, D1 is preserved
InitReqDefaults:
	move.l	d1,(a0)+		; rtrd_Size
	moveq	#REQPOS_TOPLEFTSCR,d0
	move.l	d0,(a0)+		; rtrd_ReqPos
InitReqDefs_NoSizeNoReqPos:
	moveq	#25,d0
	move.w	d0,(a0)+		; rtrd_LeftOffset
	moveq	#18,d0
	move.w	d0,(a0)+		; rtrd_TopOffset
	moveq	#6,d0
	move.w	d0,(a0)+		; rtrd_MinEntries
	moveq	#10,d0
	move.w	d0,(a0)+		; rtrd_MaxEntries
	rts

	; Initialize library
	; d0 = library pointer, a0 = seglist, a6 = sysbase
	; -> d0 = non zero if lib must be linked in the system list
InitRoutine:
	movem.l	d2/a2/a4-a6,-(a7)	; MUST BE SAME AS EXPUNGELIB !!!!!
	move.l	d0,a5
	lea	_ReqToolsBase,a4
	move.l	d0,(a4)			; our base to be used in C routines
	move.l	a6,_SysBase-_ReqToolsBase(a4)
	move.l	a0,rt_SegList(a5)

	move.w	#ReqToolsBase_SIZE,d0	; set everything private to 0
	sub.w	#rt_RealOpenCnt+1,d0
	lea	rt_RealOpenCnt(a5),a0
clear:
	clr.b	(a0)+
	dbf	d0,clear

	; Fill in preference size
	move.l	#RTPREFS_SIZE,rt_ReqToolsPrefs+rtpr_PrefsSize(a5)

	; Initialize prefs semaphore
	lea	rt_ReqToolsPrefs+rtpr_PrefsSemaphore(a5),a0
	jsr	_LVOInitSemaphore(a6)

	; Set default preferences
	lea	rt_ReqToolsPrefs+rtpr_ReqDefaults(a5),a0
	moveq	#75,d1
	bsr.b	InitReqDefaults		; RTPREF_FILEREQ
	move.w	#10,-4(a0)		; set min entries to 10
	move.w	#50,-2(a0)		; set max entries to 50
	moveq	#65,d1
	bsr.b	InitReqDefaults		; RTPREF_FONTREQ
	bsr.b	InitReqDefaults		; RTPREF_PALETTEREQ, size ignored
	bsr.b	InitReqDefaults		; RTPREF_SCREENMODEREQ
	bsr.b	InitReqDefaults		; RTPREF_VOLUMEREQ
	addq.l	#8,a0
	bsr.b	InitReqDefs_NoSizeNoReqPos ; RTPREF_OTHERREQ

	lea	intName(PC),a1
	jsr	_LVOOldOpenLibrary(a6)
	move.l	d0,_IntuitionBase-_ReqToolsBase(a4)
	move.l	d0,rt_IntuitionBase(a5)
	move.l	d0,d2

	move.l	d0,a0
	moveq	#36,d0
	cmp.w	LIB_VERSION(a0),d0	; is version >= 37
	bcs.b	okkick37

	lea	kickalertstr(PC),a0
	move.l	d2,a6
	moveq	#0,d0
	moveq	#19,d1
	jsr	_LVODisplayAlert(a6)
	moveq	#0,d2
	bra	errorinitlib

okkick37:
	move.l	a0,a6
	lea	imageclass(PC),a1
	sub.l	a0,a0
	sub.l	a2,a2
	moveq	#lod_SIZEOF,d0
	moveq	#0,d1
	jsr	_LVOMakeClass(a6)
	moveq	#0,d2
	move.l	d0,_ButtonImgClass-_ReqToolsBase(a4)
	beq	errorinitlib		; A6 MUST hold IntuitionBase!

	move.l	d0,a1
	move.l	#_myBoopsiDispatch,cl_Dispatcher+h_Entry(a1)
	move.l	$4.w,a6

	lea	gadName(PC),a1
	jsr	_LVOOldOpenLibrary(a6)
	move.l	d0,_GadToolsBase-_ReqToolsBase(a4)
	move.l	d0,rt_GadToolsBase(a5)

	lea	dosName(PC),a1
	jsr	_LVOOldOpenLibrary(a6)
	move.l	d0,_DOSBase-_ReqToolsBase(a4)
	move.l	d0,rt_DOSBase(a5)

	; Get current ReqTools preferences (D0 = DOSBase)
	movem.l	d2-d4/a6,-(a7)
	move.l	d0,a6
	lea	varname(PC),a0
	move.l	a0,d1
	lea	rt_ReqToolsPrefs+rtpr_Flags(a5),a0
	move.l	a0,d2
	move.l	rt_ReqToolsPrefs+rtpr_PrefsSize(a5),d3
	move.l	#GVF_BINARY_VAR!GVF_GLOBAL_ONLY!LV_VAR,d4
	jsr	_LVOGetVar(a6)
	movem.l	(a7)+,d2-d4/a6

	lea	gfxName(PC),a1
	jsr	_LVOOldOpenLibrary(a6)
	move.l	d0,_GfxBase-_ReqToolsBase(a4)
	move.l	d0,rt_GfxBase(a5)
	move.l	d0,a0

	lea	utilName(PC),a1
	jsr	_LVOOldOpenLibrary(a6)
	move.l	d0,_UtilityBase-_ReqToolsBase(a4)
	move.l	d0,rt_UtilityBase(a5)

	lea	layersName(PC),a1
	jsr	_LVOOldOpenLibrary(a6)
	move.l	d0,_LayersBase-_ReqToolsBase(a4)

	lea	localeName(PC),a1
	jsr	_LVOOldOpenLibrary(a6)
	move.l	d0,_LocaleBase-_ReqToolsBase(a4)

	lea	consoleName(PC),a0
	lea	iorequest-_ReqToolsBase(a4),a1
	moveq	#-1,d0
	moveq	#0,d1
	jsr	_LVOOpenDevice(a6)
	move.l	iorequest-_ReqToolsBase+IO_DEVICE(a4),d0
	move.l	d0,_ConsoleDevice-_ReqToolsBase(a4)

	move.l	a5,d0
	movem.l	(a7)+,d2/a2/a4-a6
	rts

kickalertstr:
	dc.b 0,96,11
	dc.b "This version of reqtools.library needs Kickstart 2.0 V37+",0,0

	cnop 0,2

	; Open library
	; d0 = version, a6 = ptr to lib
	; -> d0 = ptr to lib
Open:
;	addq.w	#1,LIB_OPENCNT(a6)
	addq.w	#1,rt_RealOpenCnt(a6)
;	bclr.b	#LIBB_DELEXP,rt_RTFlags(a6)	; Prevent delayed expunges
	move.l	a6,d0
	rts

	; Close library
	; a6 = ptr to lib
	; -> d0 = return seglist if lib is completely closed and delayed expunge
Close:
	moveq	#0,d0
;	subq.w	#1,LIB_OPENCNT(a6)
	subq.w	#1,rt_RealOpenCnt(a6)
;	bne.s	EndClose

;	btst	#LIBB_DELEXP,rt_RTFlags(a6)
;	bne.s	Expunge
EndClose:
	rts

	; Expunge the library
	; a6 = ptr to lib
	; -> d0 = seglist if library is no longer open
Expunge:
	movem.l	d2/a2/a4-a6,-(a7)	; MUST BE SAME AS INITLIB !!!!!
	move.l	a6,a5
	move.l	$4.w,a6

	tst.w	rt_AvailFontsLock(a5)
	bne.b	noavailfonts

	jsr	_FreeReqToolsFonts(PC)
noavailfonts:

;	tst.w	LIB_OPENCNT(a5)
	tst.w	rt_RealOpenCnt(a5)
	beq.b	NoOneOpen

;	bset.b	#LIBB_DELEXP,rt_RTFlags(a5) ; still open, set delayed expunge flag
	moveq	#0,d0
	bra.b	ExpungeEnd

NoOneOpen:
	move.l	rt_SegList(a5),d2
	move.l	a5,a1
	jsr	_LVORemove(a6)		; remove lib
	lea	iorequest,a1
	jsr	_LVOCloseDevice(a6)	; close console.device
	move.l	rt_DOSBase(a5),a1
	jsr	_LVOCloseLibrary(a6)	; close DOS
	move.l	rt_GadToolsBase(a5),a1
	jsr	_LVOCloseLibrary(a6)	; close GadTools
	move.l	rt_GfxBase(a5),a1
	jsr	_LVOCloseLibrary(a6)	; close Gfx
	move.l	rt_UtilityBase(a5),a1
	jsr	_LVOCloseLibrary(a6)	; close Utility
	move.l	_LayersBase,a1
	jsr	_LVOCloseLibrary(a6)
	move.l	_LocaleBase,a1
	jsr	_LVOCloseLibrary(a6)
	move.l	rt_IntuitionBase(a5),a6
	move.l	_ButtonImgClass,a0
	jsr	_LVOFreeClass(a6)	; free image class
errorinitlib:
	move.l	a6,a1
	move.l	($4).w,a6
	jsr	_LVOCloseLibrary(a6)	; close Intuition

	move.l	a5,a1
	moveq	#0,d0
	move.w	LIB_NEGSIZE(a5),d0
	sub.l	d0,a1
	add.w	LIB_POSSIZE(a5),d0
	jsr	_LVOFreeMem(a6)		; free our mem
	move.l	d2,d0			; seglist
ExpungeEnd:
	movem.l	(a7)+,d2/a2/a4-a6
	rts

	; Do nothing function
Null:
	moveq		#0,d0
	rts

******************************************************************************
*
* ReqTools Functions
*
******************************************************************************

	XREF _AllocRequestA
	XREF _FreeRequest
	XREF _FreeReqBuffer
	XREF _FileRequestA
	XREF _ChangeReqAttrA
	XREF _FreeFileList
	XREF _PaletteRequestA
	XREF _GetVScreenSize
	XREF _CloseWindowSafely

FuncTable:
	dc.l Open
	dc.l Close
	dc.l Expunge
	dc.l Null

	dc.l _AllocRequestA
	dc.l _FreeRequest
	dc.l _FreeReqBuffer
	dc.l _ChangeReqAttrA
	dc.l _FileRequestA
	dc.l _FreeFileList
	dc.l rtEZRequestA
	dc.l rtGetStringA
	dc.l rtGetLongA
	dc.l rtInternalGetPasswordA	; private
	dc.l rtInternalEnterPasswordA	; private
	dc.l _FileRequestA 		; = _FontRequestA!
	dc.l _PaletteRequestA
	dc.l rtReqHandlerA
	dc.l rtSetWaitPointer
	dc.l _GetVScreenSize
	dc.l rtSetReqPosition
	dc.l rtSpread
	dc.l rtScreenToFrontSafely
	dc.l _FileRequestA		; rtScreenModeRequestA
	dc.l _CloseWindowSafely
	dc.l rtLockWindow
	dc.l rtUnLockWindow
	; private funcs for preferences
	dc.l rtLockPrefs
	dc.l rtUnlockPrefs
	dc.l -1

*----------------------------------------------------------------------------*
* -- PRIVATE --
* struct ReqToolsPrefs *rtLockPrefs (void);
* d0

rtLockPrefs:
	move.l	a6,-(a7)
	lea	rt_ReqToolsPrefs+rtpr_PrefsSemaphore(a6),a0
	move.l	$4.w,a6
	jsr	_LVOObtainSemaphore(a6)
	move.l	(a7)+,a6
	lea	rt_ReqToolsPrefs(a6),a0
	move.l	a0,d0
	rts

*----------------------------------------------------------------------------*
* -- PRIVATE --
* void rtUnlockPrefs (void);

rtUnlockPrefs:
	move.l	a6,-(a7)
	lea	rt_ReqToolsPrefs+rtpr_PrefsSemaphore(a6),a0
	move.l	$4.w,a6
	jsr	_LVOReleaseSemaphore(a6)
	move.l	(a7)+,a6
	rts

	XREF _GetString

CHECK_PASSWORD	equ	0
ENTER_PASSWORD	equ	1
ENTER_STRING	equ	2
ENTER_NUMBER	equ	3
IS_EZREQUEST	equ	4

*----------------------------------------------------------------------------*
* -- PRIVATE --
* BOOL rtInternalEnterPasswordA (UBYTE *, char *(*)(), struct TagItem *);
* d0                             a1       d2           a0

rtInternalEnterPasswordA:
	movem.l	d2-d4/a2,-(a7)
	moveq	#ENTER_PASSWORD,d3
	bra.b	getpass

*----------------------------------------------------------------------------*
* -- PRIVATE --
* BOOL rtInternalGetPasswordA (UBYTE *, ULONG, char *(*)(), struct TagItem *);
* d0                           a1       d1:16  d2           a0

rtInternalGetPasswordA:
	movem.l	d2-d4/a2,-(a7)
	moveq	#CHECK_PASSWORD,d3
	clr.b	(a1)
getpass:
	moveq	#16,d0
	lea	PwTitle(PC),a2
	andi.l	#$ffff,d1
	bra.b	GetStr_PopD2toD4_A2_RTS

PwTitle:
	dc.b "Password",0
	cnop 0,2

*----------------------------------------------------------------------------*
* BOOL rtGetStringA
*              (UBYTE *, ULONG, char *, struct rtReqInfo *, struct TagItem *);
* d0            a1       d0     a2      a3                  a0

	XREF	_CheckStackCallFunc

rtGetStringA:
	movem.l	d2-d4/a2,-(a7)
	moveq	#ENTER_STRING,d3
GetStr_PopD2toD4_A2_RTS:
	move.l	a3,d4

	pea	_GetString(PC)
	jsr	_CheckStackCallFunc(PC)

	movem.l	(a7)+,d2-d4/a2
	rts

*----------------------------------------------------------------------------*
* ULONG rtEZRequestA
*                (char *, char *, struct rtReqInfo *, APTR, struct TagItem *);
* d0              a1      a2      a3                  a4    a0

rtEZRequestA:
	movem.l	d2-d4/a2,-(a7)
	move.l	a4,d0			; args
	moveq	#IS_EZREQUEST,d3
	bra.b	GetStr_PopD2toD4_A2_RTS

*----------------------------------------------------------------------------*
* BOOL rtGetLongA (ULONG *, char *, struct rtReqInfo *, struct TagItem *);
* d0               a1       a2      a3                  a0

rtGetLongA:
	movem.l	d2-d4/a2,-(a7)
	move.l	a1,d2
	moveq	#ENTER_NUMBER,d3
	bra.b	GetStr_PopD2toD4_A2_RTS

*----------------------------------------------------------------------------*
* ULONG rtReqHandlerA (APTR, ULONG *, struct TagItem *);
* d0                   a1    a2       a0

	XDEF	rtReqHandlerA

rtReqHandlerA:
	move.l	(a1),-(a7)				; first longword holds real function address!
	rts

*----------------------------------------------------------------------------*
* void rtSetWaitPointer (struct Window *);
*                        a0

	XREF		_waitpointer

rtSetWaitPointer:
	movem.l	d2/d3/a6,-(a7)
	move.l	$4.w,a1
	moveq		#38,d0
	cmp.w		LIB_VERSION(a1),d0			; is version >= 39
	bcs.b		.iskick39
	lea		_waitpointer,a1
	moveq		#16,d0
	moveq		#16,d1
	moveq		#-6,d2
	moveq		#0,d3
	move.l	rt_IntuitionBase(a6),a6
	jsr		_LVOSetPointer(a6)
	bra.b		.endsetwaitpointer
.iskick39:
	lea		setpointertaglist(PC),a1
	move.l	rt_IntuitionBase(a6),a6
	jsr		_LVOSetWindowPointerA(a6)
.endsetwaitpointer:
	movem.l	(a7)+,d2/d3/a6
	rts

setpointertaglist:
	dc.l WA_BusyPointer,1,TAG_DONE

*----------------------------------------------------------------------------*
* APTR rtLockWindow (struct Window *);
*                    a0

    STRUCTURE	rtWindowLock,0
	* requester structure (MUST BE FIRST ITEM!!!)
	STRUCT	rtwl_Requester,rq_SIZEOF
	APTR	rtwl_Magic
	ULONG	rtwl_RequesterPtr
	ULONG	rtwl_LockCount
	ULONG	rtwl_ReqInstalled
	* same as in Window structure (same order as well!)
	APTR	rtwl_Pointer
	BYTE	rtwl_PtrHeight
	BYTE	rtwl_PtrWidth
	BYTE	rtwl_XOffset
	BYTE	rtwl_YOffset
	WORD	rtwl_MinWidth
	WORD	rtwl_MinHeight
	WORD	rtwl_MaxWidth
	WORD	rtwl_MaxHeight
	LABEL	rtwl_SIZEOF

LOCKWINMAGIC		equ		'rtLW'

rtLockWindow:
	movem.l	d2/d3/a0/a2/a6,-(a7)	; DON'T CHANGE THIS WITHOUT THINKING!!

	* First see if we already locked this window.
	move.l	wd_FirstRequest(a0),a1
.nextreq:
	move.l	a1,d0
	beq.b	.noreq

	move.l	rtwl_Magic(a1),d0
	cmp.l	#LOCKWINMAGIC,d0
	bne.b	.nomagic

	cmp.l	rtwl_RequesterPtr(a1),a1
	beq.b	.increasecount

.nomagic:
	move.l	(a1),a1
	bra.b	.nextreq

	* If the window was already locked increase lock-count

.increasecount:
	addq.l	#1,rtwl_LockCount(a1)
	move.l	a1,d0			; return window lock
	bra	endlockwin

	* Lock the window
.noreq:
	move.l	#rtwl_SIZEOF,d0
	move.l	#MEMF_CLEAR,d1
	move.l	a6,-(a7)
	move.l	$4.w,a6
	jsr	_LVOAllocVec(a6)
	move.l	(a7)+,a6
	tst.l	d0
	beq.b	endlockwin

	move.l	d0,a2
	move.l	d0,rtwl_RequesterPtr(a2)
	move.l	#LOCKWINMAGIC,rtwl_Magic(a2)

	move.l	rt_IntuitionBase(a6),a6

	move.l	8(a7),a0		; A0 on stack + 8 = window ptr
	lea	rtwl_MinWidth(a2),a1
	move.l	wd_MinWidth(a0),(a1)+	; remember wd_MinWidth and wd_MinHeight
	move.l	wd_MaxWidth(a0),(a1)+	; remember wd_MaxWidth and wd_MaxHeight
	moveq	#0,d0
	move.w	wd_Width(a0),d0
	moveq	#0,d1
	move.w	wd_Height(a0),d1
	move.l	d0,d2
	move.l	d1,d3
	jsr	_LVOWindowLimits(a6)	; set window limits to current size

	move.l	a2,a0
	jsr	_LVOInitRequester(a6)

	move.l	8(a7),a1		; A0 on stack + 8 = window ptr
	move.l	a2,a0
	jsr	_LVORequest(a6)
	move.l	d0,rtwl_ReqInstalled(a2)

	move.l	8(a7),a0
	move.l	16(a7),a6
	lea	rtwl_Pointer(a2),a1
	move.l	wd_Pointer(a0),(a1)+	; remember wd_Pointer
	move.l	wd_PtrHeight(a0),(a1)+	; remember wd_PtrHeight, wd_PtrWidth,...
	jsr	_LVOrtSetWaitPointer(a6)

	move.l	a2,d0
endlockwin:
	movem.l	(a7)+,d2/d3/a0/a2/a6
	rts

*----------------------------------------------------------------------------*
* ULONG rtUnLockWindow (struct Window *, APTR lock);
*                       a0               a1

rtUnLockWindow:
	movem.l	d2/d3/a0/a1/a6,-(a7)
	move.l	a1,d0
	beq	endunlockwin

	move.l	rtwl_LockCount(a1),d0
	beq.b	.unlockwin

	subq.l	#1,rtwl_LockCount(a1)
	bra.b	endunlockwin

.unlockwin:
	* restore old window pointer
	move.l	rt_IntuitionBase(a6),a6
	tst.l	rtwl_Pointer(a1)
	beq.b	.clrptr

	moveq	#0,d0
	move.b	rtwl_PtrHeight(a1),d0
	moveq	#0,d1
	move.b	rtwl_PtrWidth(a1),d1
	move.b	rtwl_XOffset(a1),d2
	ext.w	d2
	ext.l	d2
	move.b	rtwl_YOffset(a1),d3
	ext.w	d3
	ext.l	d3
	move.l	rtwl_Pointer(a1),a1
	jsr	_LVOSetPointer(a6)
	bra.b	endreq
.clrptr:
	jsr	_LVOClearPointer(a6)

endreq:
	movem.l	8(a7),a0/a1
	tst.l	rtwl_ReqInstalled(a1)
	beq.b	noreqinstalled

	exg.l	a0,a1
	jsr	_LVOEndRequest(a6)
	movem.l	8(a7),a0/a1
noreqinstalled:

	* restore wd_MinWidth, wd_MinHeight, wd_MaxWidth and wd_MaxHeight
	moveq	#0,d0
	move.w	rtwl_MinWidth(a1),d0
	moveq	#0,d1
	move.w	rtwl_MinHeight(a1),d1
	moveq	#0,d2
	move.w	rtwl_MaxWidth(a1),d2
	moveq	#0,d3
	move.w	rtwl_MaxHeight(a1),d3
	jsr	_LVOWindowLimits(a6)	; reset window limits
	movem.l	8(a7),a0/a1

	* free lock
	move.l	$4.w,a6
	jsr	_LVOFreeVec(a6)

endunlockwin:
	movem.l	(a7)+,d2/d3/a0/a1/a6
	rts

*----------------------------------------------------------------------------*
* void rtSpread (ULONG *, ULONG *, ULONG,  ULONG, ULONG, ULONG);
*               (pos,     width,   length, x1,    x2,    num);
* D0             A0       A1       D0      D1     D2     D3

rtSpread:
	movem.l	d2/d3/d4/d5,-(a7)
	move.l	d1,d5
	swap	d5
	clr.w	d5			; gadpos (D5) = x1 << 16
	move.l	d3,d4			; num-- (D4)
	subq.l	#1,d4
	move.l	d2,d3
	sub.l	d1,d3
	sub.l	d0,d3
	divs	d4,d3
	swap	d3
	clr.w	d3			; gadgap (D3) = ((x2-x1-totwidth) / num) << 16
	moveq	#0,d0			; i (D0) = 0
loopspread:
	cmp.w	d0,d4
	bcs.s	endspread
	bne.s	notnum

	move.l	d2,d1			; pos[i] = x2 - width[i]
	sub.l	(a1),d1
	bra.b	addpos
notnum:
	move.l	d5,d1			; pos[i] = gadpos >> 16
	clr.w	d1
	swap	d1
addpos:
	move.l	d1,(a0)+
	move.l	(a1)+,d1		; gadpos += (width[i] << 16) + gadgap
	swap	d1
	clr.w	d1
	add.l	d3,d1
	add.l	d1,d5
	addq.l	#1,d0
	bra.b	loopspread

endspread:
	movem.l	(a7)+,d2/d3/d4/d5
	rts

*----------------------------------------------------------------------------*
* void rtScreenToFrontSafely (struct Screen *);
*                             a0

rtScreenToFrontSafely:
	move.l	a6,-(a7)
	move.l	rt_IntuitionBase(a6),a1
	move.l	($4).w,a6
	jsr	_LVOForbid(a6)		; a1 will be preserved
	move.l	a1,a6
	move.l	ib_FirstScreen(a6),a1
loopscr:
	move.l	a1,d0
	beq.b	nofoundscr

	cmp.l	a1,a0
	beq.b	foundscr

	move.l	(a1),a1			; == move.l sc_NextScreen(a1),a1
	bra.b	loopscr
foundscr:
	jsr	_LVOScreenToFront(a6)
nofoundscr:
	move.l	($4).w,a6
	jsr	_LVOPermit(a6)
	move.l	(a7)+,a6
	rts

*----------------------------------------------------------------------------*
* void rtSetReqPosition
*               (ULONG, struct NewWindow *, struct Screen *, struct Window *);
*                d0     a0                  a1               a2

;void __asm SetReqPosition (
;	register __d0 int reqpos,
;	register __a0 struct NewWindow *nw,
;	register __a1 struct Screen *scr,
;	register __a2 struct Window *win)
;{
;	int mx, my, val, leftedge, topedge;
;	int scrwidth, scrheight;
;	int width, height, left, top;
;
;	rtGetVScreenSize (scr, &scrwidth, &scrheight);
;	leftedge = -scr->LeftEdge;
;	if (leftedge < 0) leftedge = 0;
;	topedge = -scr->TopEdge;
;	if (topedge < 0) topedge = 0;
;
;	left = leftedge; top = topedge;
;	width = scrwidth; height = scrheight;
;	switch (reqpos) {
;		case REQPOS_DEFAULT:
;			nw->LeftEdge = 25;
;			nw->TopEdge = 18;
;			goto topleftscr;
;		case REQPOS_POINTER:
;			mx = scr->MouseX; my = scr->MouseY;
;			break;
;		case REQPOS_CENTERWIN:
;			if (win) {
;				left = win->LeftEdge; top = win->TopEdge;
;				width = win->Width; height = win->Height;
;				}
;		case REQPOS_CENTERSCR:
;			mx = (width - nw->Width) / 2 + left;
;			my = (height - nw->Height) / 2 + top;
;			break;
;		case REQPOS_TOPLEFTWIN:
;			if (win) {
;				left = win->LeftEdge;
;				top = win->TopEdge;
;				}
;		case REQPOS_TOPLEFTSCR:
;topleftscr:
;			mx = left; my = top;
;			break;
;		}
;
;	/* keep window completely visible */
;	mx += nw->LeftEdge; my += nw->TopEdge;
;	val = leftedge + scrwidth - nw->Width;
;	if (mx < leftedge) mx = leftedge;
;	else if (mx > val) mx = val;
;	val = topedge + scrheight - nw->Height;
;	if (my < topedge) my = topedge;
;	else if (my > val) my = val;
;
;	nw->LeftEdge = mx; nw->TopEdge = my;
;}

REQPOS_DEFAULT		equ	$FFFF

rtSetReqPosition:
	movem.l	d2-d7/a2-a5,-(a7)
	move.l	a0,a4			; a4 = newwindow
	move.l	a1,a3			; a3 = screen
	move.l	a2,a5			; a5 = win
	move.l	a1,a0
	subq.w	#4,a7
	move.l	a7,a1
	subq.w	#4,a7
	move.l	a7,a2
	move.l	d0,-(a7)
	jsr	_LVOrtGetVScreenSize(a6)
	move.l	(a7)+,d0
	move.l	(a7)+,d7		; d7 = scrheight
	move.l	(a7)+,d6		; d6 = scrwidth
	moveq	#0,d1
	move.w	sc_LeftEdge(a3),d1
	ext.l	d1
	neg.l	d1			; d1 = leftedge
	bpl.b	posleft

	moveq	#0,d1
posleft:
	moveq	#0,d2
	move.w	sc_TopEdge(a3),d2
	ext.l	d2
	neg.l	d2			; d2 = topedge
	bpl.b	postop

	moveq	#0,d2
postop:
	move.l	d1,d3			; d3 = left
	move.l	d2,d4			; d4 = top
	move.l	d6,a1			; a1 = width
	move.l	d7,a2			; a2 = height
	; switch (reqpos) {
	cmp.l	#REQPOS_DEFAULT,d0	; is d0 == REQPOS_DEFAULT
	bne.b	notdefault

	; case REQPOS_DEFAULT:
	moveq	#25,d0
	move.w	d0,(a4)			; == move.w d5,nw_LeftEdge(a4)
	moveq	#18,d0
	move.w	d0,nw_TopEdge(a4)
	bra.b	tlscr

notdefault:
	tst.l	d0
	bne.b	notpointer

	; case REQPOS_POINTER:
	move.w	sc_MouseX(a3),d3	; mx = scr->MouseX
	move.w	sc_MouseY(a3),d4	; my = scr->MouseY
	bra.b	endswitch

notpointer:
	subq.l	#1,d0
	bne.b	notcentwin

	; case REQPOS_CENTERWIN:
	move.l	a5,d0
	beq.b	centscr

	move.w	wd_LeftEdge(a5),d3	; left = win->LeftEdge
	move.w	wd_TopEdge(a5),d4	; top = win->TopEdge
	move.w	wd_Width(a5),a1		; width = win->Width
	move.w	wd_Height(a5),a2	; height = win->Height
	bra.b	centscr

notcentwin:
	subq.l	#1,d0
	bne.b	notcentscr

	; case REQPOS_CENTERSCR:
centscr:
	move.l	a1,d0
	moveq	#0,d5
	move.w	nw_Width(a4),d5
	sub.l	d5,d0
	lsr.l	#1,d0
	add.l	d0,d3			; mx = (width - nw->Width) / 2 + left
	move.l	a2,d0
	move.w	nw_Height(a4),d5
	sub.l	d5,d0
	lsr.l	#1,d0
	add.l	d0,d4			; my = (height - nw->Height) / 2 + top
	bra.b	endswitch

notcentscr:
	subq.l	#1,d0
	bne.b	nottlwin

	; case REQPOS_TOPLEFTWIN:
	move.l	a5,d0
	beq.b	tlscr

	move.w	wd_LeftEdge(a5),d3	; left = win->LeftEdge
	move.w	wd_TopEdge(a5),d4	; top = win->TopEdge
*	bra.b	tlscr

nottlwin:
*	subq.l	#1,d0
*	bne.b	endswitch

	; case REQPOS_TOPLEFTSCR:
tlscr:
*	move.l	d3,d3			; mx = left
*	move.l	d4,d4			; my = top
endswitch:
	; } /* switch */

	add.w	(a4),d3			; == add.w nw_LeftEdge(a4),d3
	cmp.w	d1,d3
	bgt.s	okleft

	move.w	d1,d3
okleft:
	add.w	d6,d1
	sub.w	nw_Width(a4),d1		; leftedge + scrwidth - nw->Width
	cmp.w	d1,d3
	blt.s	okright

	move.w	d1,d3
okright:

	add.w	nw_TopEdge(a4),d4
	cmp.w	d2,d4
	bgt.s	oktop

	move.w	d2,d4
oktop:
	add.w	d7,d2
	sub.w	nw_Height(a4),d2	; leftedge + scrwidth - nw->Width
	cmp.w	d2,d4
	blt.s	okbottom

	move.w	d2,d4
okbottom:

	move.w	d3,(a4)			; == move.w d3,nw_LeftEdge(a4)
	move.w	d4,nw_TopEdge(a4)

	movem.l	(a7)+,d2-d7/a2-a5
	rts


	SECTION __MERGED,BSS

	XDEF _ReqToolsBase
	XDEF _IntuitionBase
	XDEF _GadToolsBase
	XDEF _DOSBase
	XDEF _GfxBase
	XDEF _UtilityBase
	XDEF _LayersBase
	XDEF _LocaleBase
	XDEF _ConsoleDevice
	XDEF _ButtonImgClass
	XDEF _SysBase

_ReqToolsBase:		ds.b 4
_SysBase:		ds.b 4
_IntuitionBase:		ds.b 4
_GadToolsBase:		ds.b 4
_DOSBase:		ds.b 4
_GfxBase:		ds.b 4
_UtilityBase:		ds.b 4
_LayersBase:		ds.b 4
_LocaleBase:		ds.b 4
_ConsoleDevice:		ds.b 4
_ButtonImgClass:	ds.b 4
iorequest:		ds.b IOSTD_SIZE

	END
