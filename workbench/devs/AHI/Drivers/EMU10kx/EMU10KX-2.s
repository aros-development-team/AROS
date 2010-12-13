

	.MACRO	FORM_START name
CHUNKCNT	.ASSIGNA 0
	.ascii	"FORM"
	.long	FORMEND-FORMSTART
FORMSTART:
	.ascii	"\name"
	.ENDM

	.MACRO	FORM_END name
FORMEND:
	.balign	2
	.ENDM

	.MACRO	CHUNK_START name
	.ascii	"\name"
	.long	CHUNKEND\&CHUNKCNT-CHUNKSTART\&CHUNKCNT
CHUNKSTART\&CHUNKCNT:
	.ENDM

	.MACRO	CHUNK_END
CHUNKEND\&CHUNKCNT:
CHUNKCNT	.ASSIGNA \&CHUNKCNT+1
	.balign	2
	.ENDM

AHIDB_AudioID	.EQU	2147483648+100
AHIDB_Volume	.EQU	2147483648+103
AHIDB_Panning	.EQU	2147483648+104
AHIDB_Stereo	.EQU	2147483648+105
AHIDB_HiFi	.EQU	2147483648+106
AHIDB_MultTable	.EQU	2147483648+108
AHIDB_Name	.EQU	2147483648+32768+109

TAG_DONE	.EQU	0
	
TRUE		.EQU	1
FALSE		.EQU	0
	
		
	FORM_START	AHIM
	
	CHUNK_START	AUDN
	.asciz		"emu10kx"
	CHUNK_END
	
	CHUNK_START	AUDM
1:	
	.long		AHIDB_AudioID,	0x001e1001
	.long		AHIDB_Volume,	TRUE
	.long		AHIDB_Panning,	FALSE
	.long		AHIDB_Stereo,	FALSE
	.long		AHIDB_HiFi,	TRUE
	.long		AHIDB_MultTable,FALSE
	.long		AHIDB_Name,	2f-1b
	.long		TAG_DONE
2:
	.asciz		"EMU10kx-2:HiFi 16 bit mono"
	CHUNK_END

	CHUNK_START	AUDM
1:	
	.long		AHIDB_AudioID,	0x001e1002
	.long		AHIDB_Volume,	TRUE
	.long		AHIDB_Panning,	TRUE
	.long		AHIDB_Stereo,	TRUE
	.long		AHIDB_HiFi,	TRUE
	.long		AHIDB_MultTable,FALSE
	.long		AHIDB_Name,	2f-1b
	.long		TAG_DONE
2:
	.asciz		"EMU10kx-2:HiFi 16 bit stereo++"
	CHUNK_END
		
	CHUNK_START	AUDM
1:	
	.long		AHIDB_AudioID,	0x001e1003
	.long		AHIDB_Volume,	TRUE
	.long		AHIDB_Panning,	FALSE
	.long		AHIDB_Stereo,	FALSE
	.long		AHIDB_HiFi,	FALSE
	.long		AHIDB_MultTable,FALSE
	.long		AHIDB_Name,	2f-1b
	.long		TAG_DONE
2:
	.asciz		"EMU10kx-2:16 bit mono"
	CHUNK_END

	CHUNK_START	AUDM
1:	
	.long		AHIDB_AudioID,	0x001e1004
	.long		AHIDB_Volume,	TRUE
	.long		AHIDB_Panning,	TRUE
	.long		AHIDB_Stereo,	TRUE
	.long		AHIDB_HiFi,	FALSE
	.long		AHIDB_MultTable,FALSE
	.long		AHIDB_Name,	2f-1b
	.long		TAG_DONE
2:
	.asciz		"EMU10kx-2:16 bit stereo++"
	CHUNK_END
		
	FORM_END
	.end

	