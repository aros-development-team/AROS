
	.MACRO	LONG num
.ifdef	LITTLE_ENDIAN
	.byte	((\num)>>24)&255
	.byte	((\num)>>16)&255
	.byte	((\num)>> 8)&255
	.byte	((\num)>> 0)&255
.else
	.long	\num
.endif
	.ENDM

	.MACRO	LONG2 num1,num2
.ifdef	LITTLE_ENDIAN
	.byte	((\num1)>>24)&255
	.byte	((\num1)>>16)&255
	.byte	((\num1)>> 8)&255
	.byte	((\num1)>> 0)&255
	
	.byte	((\num2)>>24)&255
	.byte	((\num2)>>16)&255
	.byte	((\num2)>> 8)&255
	.byte	((\num2)>> 0)&255
.else
	.long	\num1, \num2
.endif
	.ENDM

	.MACRO	FORM_START name
	.ascii	"FORM"
	LONG	FORMEND-FORMSTART
FORMSTART:
	.ascii	"\name"
	.ENDM

	.MACRO	FORM_END name
FORMEND:
	.balign	2,0
	.ENDM

	.MACRO	CHUNK_START name
	.ascii	"\name"
	LONG	101f-100f
100:
	.ENDM

	.MACRO	CHUNK_END
101:
	.balign	2,0
	.ENDM

.SET AHI_TagBase,     2147483648
.SET AHI_TagBaseR,    AHI_TagBase+32768

.SET AHIDB_AudioID,   AHI_TagBase+100
.SET AHIDB_Volume,    AHI_TagBase+103
.SET AHIDB_Panning,   AHI_TagBase+104
.SET AHIDB_Stereo,    AHI_TagBase+105
.SET AHIDB_HiFi,      AHI_TagBase+106
.SET AHIDB_MultTable, AHI_TagBase+108
.SET AHIDB_Name,      AHI_TagBaseR+109
.SET AHIDB_MultiChannel, AHI_TagBase+144
.SET AHIDB_PingPong,  AHI_TagBase+107

.SET AHIDB_UserBase,  AHI_TagBase+500

.SET TAG_DONE, 	      0
	
.SET TRUE, 	      1
.SET FALSE, 	      0
