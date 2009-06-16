*
* CAMD Driver for Poseidon USB camdusbmidi.class
*
* Copyright 2006 Chris Hodges
*
************************************************************************
* Date	      | Change
*-----------------------------------------------------------------------
* 04-Feb-2006 : Initial
************************************************************************

	;output	DEVS:midi/poseidonusb

	include	"AmigaLVOs.s"
	include	"Macros.lnk"
	
	include "exec/types.i"
	include "exec/execbase.i"           ; for FlushDevice()
	include "exec/macros.i"
	include	"exec/ports.i"
	include	"utility/hooks.i"
	include "midi/camddevices.i"

_LVOusbCAMDOpenPort	equ	-108
_LVOusbCAMDClosePort	equ	-114

Version  equ 1
Revision equ 2
Ports	 equ 16

DEBUG_DETAIL	set	0

   STRUCTURE CAMDAdapter,0
	APTR	ca_ActivateFunc
	BOOL	ca_IsOpen
	STRUCT	ca_CAMDRXFunc,h_SIZEOF
	STRUCT	ca_CAMDTXFunc,IS_SIZE
	ULONG	ca_PortNum
	APTR	ca_TXFunc
	APTR	ca_RXFunc
	APTR	ca_UserData
	APTR	ca_TXBuffer
	ULONG	ca_TXBufSize
	ULONG	ca_TXWritePos
	ULONG	ca_TXReadPos
	APTR	ca_MsgPort
	LABEL	CAMDAdapter_SIZE

****************************************************************
*
*   Standard MIDI Device driver header
*
****************************************************************

; code at start of file in case anyone tries to execute us as a program

FalseStart
	moveq	#-1,d0
	rts

MDD ; struct MidiDeviceData
	dc.l	MDD_Magic	; mdd_Magic
	dc.l	Name		; mdd_Name
	dc.l	IDString	; mdd_IDString
	dc.w	Version 	; mdd_Version
	dc.w	Revision	; mdd_Revision
	dc.l	Init		; mdd_Init
	dc.l	Expunge 	; mdd_Expunge
	dc.l	OpenPort	; mdd_OpenPort
	dc.l	ClosePort	; mdd_ClosePort
	dc.b	Ports		; mdd_NPorts
	dc.b	0		; mdd_Flags

	;        123456789012
Name	dc.b    'poseidonusb',0
	dc.b	'3456789012345678901',0			; 32 bytes

IDString    
	dc.b    '$VER: Poseidon USB camdusbmidi.class driver 1.1 (21-Feb-06)',0
	dc.b	'Copyright 2006 Chris Hodges',0
	even

****************************************************************
*
*   MidiDeviceData Functions
*
****************************************************************

****************************************************************
*
*   Init
*
*   FUNCTION
*	Gets called by CAMD after being LoadSeg'ed.
*
*   INPUTS
*	None
*
*   RESULTS
*	TRUE if successful, FALSE on failure.
*
****************************************************************

Init
	PUTMSG	10,<"Init!">
	PUSHM	a6/a0/a1/d1
	move.l	4.w,a6
	move.l	a6,SysBase
	lea	.classname(pc),a1
	moveq.l	#0,d0
	CALL	OpenLibrary
	move.l	d0,USBClsBase
	bne.s	.good
	lea	.classname2(pc),a1
	moveq.l	#0,d0
	CALL	OpenLibrary
	move.l	d0,USBClsBase
.good	POPM
	rts
.classname2
	dc.b	'USB/'
.classname
	dc.b	'camdusbmidi.class',0

	even

****************************************************************
*
*   Expunge
*
*   FUNCTION
*	Gets called by CAMD immediately before being
*	UnLoadSeg'ed.
*
*   INPUTS
*	None
*
*   RESULTS
*	None
*
****************************************************************

Expunge
	PUTMSG	10,<"Expunge!">
	PUSHM	a6/a1/d0/d1
	move.l	SysBase(pc),a6
	move.l	USBClsBase(pc),a1
	CALL	CloseLibrary
	clr.l	USBClsBase
	POPM
	rts


****************************************************************
*
*   SendToCAMD
*
* Called by CallHookA() from camdusbmidi.class with Buffer address in a2
* and size in a1.
*
****************************************************************

SendToCAMD
	PUTMSG	10,<"SendToCAMD %08lx, buf=%08lx">,a0,a2
	PUSHM	a2-a4/d2
	moveq.l	#0,d2
	move.l	a2,a3 ; object
	move.l	h_Data(a0),a0
	move.b	(a3)+,d2
	PUTMSG	10,<"size=%08ld">,d2
	move.l	ca_UserData(a0),a2
	move.l	ca_RXFunc(a0),a4
.loop	moveq.l	#0,d0
	move.b	(a3)+,d0
	jsr	(a4)
	subq.l	#1,d2
	bgt.s	.loop
	POPM
	PUTMSG	10,<"done">
	rts

****************************************************************
*
*   GetFromCAMD
*
* Called by Cause() from ActivateXmit. 
*
****************************************************************

GetFromCAMD
	PUTMSG	10,<"GetFromCAMD %08lx">,a1
	PUSHM	a2-a6/d2/d3
	move.l	a1,a3
	move.l	ca_UserData(a3),a2
	move.l	ca_TXBuffer(a3),a5
	move.l	ca_TXFunc(a3),a4
	move.l	ca_TXBufSize(a3),d3
	move.l	ca_TXWritePos(a3),d2
	subq.l	#1,d3
.loop	jsr	(a4)
	PUTMSG	10,<"Byte... %lx %ld">,d0,d1
	move.b	d0,(a5,d2.l)
	addq.l	#1,d2
	and.l	d3,d2
	cmp.l	ca_TXReadPos(a3),d2
	beq.s	.oops			; this will lose data!
	move.l	d2,ca_TXWritePos(a3)
	tst.b	d1
	beq.s	.loop
.oops	
	move.l	ca_MsgPort(a3),d0	; now inform usb driver
	beq.s	.noport
	move.l	d0,a0
	move.l	MP_SIGTASK(a0),a1
	moveq.l	#1,d0
	moveq.l	#0,d1
	move.b	MP_SIGBIT(a0),d1
	lsl.l	d1,d0
	move.l	SysBase(pc),a6
	CALL	Signal
	
.noport	POPM
	PUTMSG	10,<"Done">
	rts

****************************************************************
*
*   OpenPort
*
*   FUNCTION
*	Open a MIDI port.
*
*   INPUTS
*	D0.b - Port number (should always be 0 for this driver)
*	A0 - Xmit function
*	A1 - Recv function
*	A2 - Data
*
*   RESULT
*	D0 - pointer to MidiPortData structure.
*
****************************************************************

OpenPort
	PUTMSG	10,<"OpenPort %ld, Xmit=%08lx,Recv=%08lx,Data=%08lx">,d0,a0,a1,a2
	PUSHM	a6/a0-a3/d1/d2
	move.l	USBClsBase(pc),a6
	moveq.l	#0,d2
	move.b	d0,d2
	move.l	d2,d0
	lea	Name(pc),a3
	CALL	usbCAMDOpenPort
	lea	CAMDPortBases(pc),a0
	move.l	d0,(a0,d2.l*4)
	beq.s	.toobad
	move.l	d0,a0
	lea	ActivateXmit0(pc,d2.l*4),a1
	move.l	a1,ca_ActivateFunc(a0)

	move.l	d0,ca_CAMDRXFunc+h_Data(a0)
	lea	SendToCAMD(pc),a1
	move.l	a1,ca_CAMDRXFunc+h_Entry(a0)

	move.b	#NT_INTERRUPT,ca_CAMDTXFunc+LN_TYPE(a0)
	clr.b	ca_CAMDTXFunc+LN_PRI(a0)
	move.l	d0,ca_CAMDTXFunc+IS_DATA(a0)
	lea	GetFromCAMD(pc),a1
	move.l	a1,ca_CAMDTXFunc+IS_CODE(a0)
	st	ca_IsOpen(a0)
.toobad	PUTMSG	10,<"Result=%08lx">,d0
	POPM
	rts


****************************************************************
*
*   ClosePort
*
*   FUNCTION
*	Close a MIDI port.
*
*   INPUTS
*	D0.b - Port number (always 0 for this driver).
*       A1 - USBID
*
*   RESULT
*	None
*
****************************************************************

ClosePort
	PUSHM	a6/a0/a1/d1
	move.l	USBClsBase(pc),a6
	and.l	#$ff,d0
	lea	Name(pc),a1
	CALL	usbCAMDClosePort
	POPM
	rts

	IFNE	DEBUG_DETAIL
kprintf	PUSHM	a2-a3/a6
	lea	.putchr(pc),a2
	move.l	4.w,a6
	move.l	a6,a3
	CALL	RawDoFmt
	POPM
	rts
.putchr	dc.w	$CD4B,$4EAE,$FDFC,$CD4B,$4E75
	ENDC

****************************************************************
*
*   ActivateXmit
*
* Called by CAMD with Midi Data in a0 (or a2?)
* BUG: Actually, the Midi Data is not in any register. This sucks
*
****************************************************************

ActivateXmit0
	moveq.l	#0,d0
	bra.s	ActivateXmit
	moveq.l	#1,d0
	bra.s	ActivateXmit
	moveq.l	#2,d0
	bra.s	ActivateXmit
	moveq.l	#3,d0
	bra.s	ActivateXmit
	moveq.l	#4,d0
	bra.s	ActivateXmit
	moveq.l	#5,d0
	bra.s	ActivateXmit
	moveq.l	#6,d0
	bra.s	ActivateXmit
	moveq.l	#7,d0
	bra.s	ActivateXmit
	moveq.l	#8,d0
	bra.s	ActivateXmit
	moveq.l	#9,d0
	bra.s	ActivateXmit
	moveq.l	#10,d0
	bra.s	ActivateXmit
	moveq.l	#11,d0
	bra.s	ActivateXmit
	moveq.l	#12,d0
	bra.s	ActivateXmit
	moveq.l	#13,d0
	bra.s	ActivateXmit
	moveq.l	#14,d0
	bra.s	ActivateXmit
	moveq.l	#15,d0
;	bra.s	ActivateXmit

ActivateXmit
	PUTMSG	10,<"ActivateXmit Port %ld">,d0
	PUSHM	a6
	move.l	SysBase(pc),a6
	move.l	CAMDPortBases(pc,d0.l*4),a0
	lea	ca_CAMDTXFunc(a0),a1
	CALL	Cause
	POPM
	rts

USBClsBase	
	ds.l	1
SysBase	ds.l	1

CAMDPortBases
	ds.l	16

	END

