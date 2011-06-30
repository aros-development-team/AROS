; loader.asm


		INCLUDE	"exec/types.i"
		INCLUDE	"exec/exec.i"
		INCLUDE	"libraries/dosextens.i"
		INCLUDE	"libraries/amiga._LVO.i"


		STRUCTURE	STACKDATA,0
		APTR	Packet
		LONG	ReturnVal
		APTR	DOSBase
		BPTR	Segment
		BYTE	STACKDATA_SIZE


LINKEXE:	MACRO
		MOVEA.L	_AbsExecBase,A6
		JSR	_LVO\1(A6)
		ENDM

LINKDOS:	MACRO
		MOVEA.L	DOSBase(SP),A6
		JSR	_LVO\1(A6)
		ENDM


_AbsExecBase	EQU	4


;----------------------------------------------------------------------------

StartModule:	DC.L	(EndModule-StartModule)/4	; for BCPL linking


EntryPoint:	SUBA.L	#STACKDATA_SIZE,SP

		LSL.L	#2,D1			; convert to byte pointer
		MOVE.L	D1,Packet(SP)

		CLR.L	ReturnVal(SP)		; no error - for now


OpenDOS:	LEA	DOSName(PC),A1
		CLR.L	D0
		LINKEXE	OpenLibrary
		MOVE.L	D0,DOSBase(SP)
		BNE	LoadCode

		MOVE.L	#ERROR_INVALID_RESIDENT_LIBRARY,ReturnVal(SP)
		BRA	Return
		

LoadCode:	LEA	HandlerName(PC),A1
		MOVE.L	A1,D1
		LINKDOS	LoadSeg
		MOVE.L	D0,Segment(SP)
		BNE	CallHandler

		MOVE.L	#ERROR_OBJECT_NOT_FOUND,ReturnVal(SP)
		BRA	CloseDOS


CallHandler:	LEA	SPsave(PC),A1
		MOVE.L	SP,(A1)			; save current SP

		MOVE.L	Segment(SP),D0		; BPTR to segment
		LSL.L	#2,D0
		MOVEA.L	D0,A0			; byte pointer to segment
		MOVE.L	Packet(SP),D0		; packet address
		MOVE.L	 D0,-(SP)		; push (not sure if safe above)

; --- Now, call the loaded handler code.
; --- It is sent the byte address of the startup packet passed to this code.

		JSR	4(A0)			; call first code in segment

		MOVEA.L	SPsave(PC),SP		; restore SP


UnloadCode:	MOVE.L	Segment(SP),D1
		LINKDOS	UnLoadSeg

CloseDOS:	MOVE.L	DOSBase(SP),A1
		LINKEXE	CloseLibrary

Return:		MOVE.L	ReturnVal(SP),D0	; retrieve return value
		ADDA.L	#STACKDATA_SIZE,SP
		RTS


SPsave:		DC.L	0

DOSName:	DOSNAME

HandlerName:	DC.B	'L:'
ProcessName:	DC.B	'pipe-handler',0


;	trailing definitions for BCPL linking

		CNOP	0,4			; align to lonword boundary

		DC.L	0			; End Marker
		DC.L	1			; Global 1
		DC.L	EntryPoint-StartModule	; Offset
		DC.L	1			; Highest Global Used

EndModule:	END
