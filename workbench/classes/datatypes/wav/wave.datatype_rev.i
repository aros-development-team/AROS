VERSION		EQU	50
REVISION	EQU	3

DATE	MACRO
		dc.b '20.11.2007'
		ENDM

VERS	MACRO
		dc.b 'wave.datatype 50.3'
		ENDM

VSTRING	MACRO
		dc.b 'wave.datatype 50.3 (20.11.2007)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: wave.datatype 50.3 (20.11.2007)',0
		ENDM
