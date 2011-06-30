; prelude.asm


		XREF	_handler


StartModule:	DC.L	(EndModule-StartModule+3)/4	; for BCPL linking

EntryPoint:	LEA	SPsave(PC),A0
		MOVE.L	SP,(A0)			; save initial SP

		LSL.L	#2,D1			; convert to byte pointer
		MOVE.L	D1,-(SP)		; startup packet pointer

; --- Now, call the loaded handler code.
; --- It is sent the byte address of the startup packet, to which
; --- it should reply.

		JSR	_handler		; call handler code

		MOVEA.L	SPsave(PC),SP		; restore SP
		RTS


SPsave:		DC.L	0


;	trailing definitions for BCPL linking

		CNOP	0,4			; align to lonword boundary

		DC.L	0			; End Marker
		DC.L	1			; Global 1
		DC.L	EntryPoint-StartModule	; Offset
		DC.L	1			; Highest Global Used

EndModule:	END
