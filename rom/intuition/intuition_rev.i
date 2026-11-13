VERSION	EQU	50
REVISION	EQU	46
DATE	MACRO
	dc.b	'13.6.2003'
	ENDM
VERS	MACRO
	dc.b	'intuition.library 50.46'
	ENDM
VSTRING	MACRO
	dc.b	'intuition.library 50.46 (13.6.2003) ',13,10,0
	ENDM
VERSTAG	MACRO
	dc.b	0,'$VER: intuition.library 50.46 (13.6.2003) ',0
	ENDM
