
	FORM_START	AHIM
	
	CHUNK_START	AUDN
	.asciz		"cmi8738"
	CHUNK_END
	
	CHUNK_START	AUDM
1:	
	.long		AHIDB_AudioID,	0x00351001
	.long		AHIDB_Volume,	TRUE
	.long		AHIDB_Panning,	FALSE
	.long		AHIDB_Stereo,	FALSE
	.long		AHIDB_HiFi,	TRUE
	.long		AHIDB_MultTable,FALSE
	.long		AHIDB_Name,	2f-1b
	.long		TAG_DONE
2:
	.asciz		"CMI8738:HiFi 16 bit mono"
	CHUNK_END

	CHUNK_START	AUDM
1:	
	.long		AHIDB_AudioID,	0x00351002
	.long		AHIDB_Volume,	TRUE
	.long		AHIDB_Panning,	TRUE
	.long		AHIDB_Stereo,	TRUE
	.long		AHIDB_HiFi,	TRUE
	.long		AHIDB_MultTable,FALSE
	.long		AHIDB_Name,	2f-1b
	.long		TAG_DONE
2:
	.asciz		"CMI8738:HiFi 16 bit stereo++"
	CHUNK_END
		
	CHUNK_START	AUDM
1:	
	.long		AHIDB_AudioID,	0x00351003
	.long		AHIDB_Volume,	TRUE
	.long		AHIDB_Panning,	FALSE
	.long		AHIDB_Stereo,	FALSE
	.long		AHIDB_HiFi,	FALSE
	.long		AHIDB_MultTable,FALSE
	.long		AHIDB_Name,	2f-1b
	.long		TAG_DONE
2:
	.asciz		"CMI8738:16 bit mono"
	CHUNK_END

	CHUNK_START	AUDM
1:	
	.long		AHIDB_AudioID,	0x00351004
	.long		AHIDB_Volume,	TRUE
	.long		AHIDB_Panning,	TRUE
	.long		AHIDB_Stereo,	TRUE
	.long		AHIDB_HiFi,	FALSE
	.long		AHIDB_MultTable,FALSE
	.long		AHIDB_Name,	2f-1b
	.long		TAG_DONE
2:
	.asciz		"CMI8738:16 bit stereo++"
	CHUNK_END
		
	FORM_END

	.balign	4,0
	.END
	
