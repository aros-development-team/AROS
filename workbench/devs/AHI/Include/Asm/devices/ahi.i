	IFND DEVICES_AHI_I
DEVICES_AHI_I		SET	1

**
**	$VER: ahi.i 6.0 (02.02.2005)
**	:ts=8 (TAB SIZE: 8)
**
**	ahi.device definitions
**
**	(C) Copyright 1994-2005 Martin Blom
**	All Rights Reserved.
**
**

*------------------------------------------------------------------------*


	IFND EXEC_TYPES_I
	INCLUDE 'exec/types.i'
	ENDC

	IFND EXEC_IO_I
	INCLUDE 'exec/io.i'
	ENDC

	IFND UTILITY_TAGITEM_I
	INCLUDE 'utility/tagitem.i'
	ENDC

*------------------------------------------------------------------------*


*** MACROS

Fixed	MACRO					; A fixed-point value, 16 bits
\1	    EQU     SOFFSET			; to the left of the point and
SOFFSET     SET     SOFFSET+4			; 16 bits to the right
	    ENDM

AHINAME	MACRO
	dc.b	"ahi.device",0
	ENDM

 ; A handy macro to open the device for use as a library.
 ; Usage:  OPENAHI <version>
 ; On exit: d0=_AHIBase or NULL for error.

OPENAHI	MACRO
	movem.l	d1/a0-a1/a6,-(sp)
	lea	AHI_PORT(pc),a0
	clr.l	(a0)+
	clr.l	(a0)+
	moveq	#0,d0
	move.l	d0,(a0)+

	move.l	4.w,a6
	jsr	_LVOCreateMsgPort(a6)
	lea	AHI_PORT(pc),a0
	move.l	d0,(a0)
	beq.b	AHI_ERROR
	move.l	d0,a0
	moveq	#AHIRequest_SIZEOF,d0
	jsr	_LVOCreateIORequest(a6)
	lea	AHI_IOR(pc),a0
	move.l	d0,(a0)
	beq.b	AHI_ERROR
	lea	AHI_NAME(pc),a0
	move.l	d0,a1
	move.w	#\1,ahir_Version(a1)
	move.l	#AHI_NO_UNIT,d0
	moveq	#0,d1
	jsr	_LVOOpenDevice(a6)
	lea	AHI_DEVICE(pc),a0
	move.l	d0,(a0)
	bne.b	AHI_ERROR

	move.l	AHI_IOR(PC),a0
	move.l	IO_DEVICE(a0),d0
	bra.b	AHI_EXIT

AHI_PORT:	dc.l	0
AHI_IOR:	dc.l	0
AHI_DEVICE:	dc.l	0
AHI_NAME:	AHINAME
	even
AHI_ERROR:
	moveq	#0,D0
AHI_EXIT:
	movem.l	(sp)+,d1/a0-a1/a6
	ENDM

 ; Here is the macro for closing the device after it has been
 ; opened with the OPENAHI macro. Call CLOSEAHI even if OPENAHI
 ; failed!

CLOSEAHI MACRO
	movem.l	d0-d1/a0-a1/a6,-(sp)
	move.l	4.w,a6
	lea	AHI_DEVICE(pc),a1
	tst.l	(a1)
	bne.b	.ahi_nodevice
	subq.l	#1,(a1)
	move.l	AHI_IOR(pc),a1
	jsr	_LVOCloseDevice(a6)
	lea	AHI_IOR(pc),a1
	move.l	(a1),a0
	clr.l	(a1)
	jsr	_LVODeleteIORequest(a6)
.ahi_nodevice
	lea	AHI_PORT(pc),a1
	move.l	(a1),a0
	clr.l	(a1)
	jsr	_LVODeleteMsgPort(a6)
	movem.l	(sp)+,d0-d1/a0-a1/a6
	ENDM
 
*** STRUCTURES

 ; AHIAudioCtrl
	STRUCTURE AHIAudioCtrl,0
	APTR	ahiac_UserData
	; Lots of private data follows!
	LABEL	AHIAudioCtrl_SIZEOF		; Do not use!

 ; AHISoundMessage
 	STRUCTURE AHISoundMessage,0
 	UWORD	ahism_Channel
 	LABEL	AHISoundMessage_SIZEOF

 ; AHIRecordMessage
	STRUCTURE AHIRecordMessage,0
	ULONG	ahirm_Type			; Format of buffer (object)
	APTR	ahirm_Buffer			; Pointer to the sample array
	ULONG	ahirm_Length			; Number of sample frames in buffer
	LABEL	AHIRecordMessage_SIZEOF

 ; AHISampleInfo
	STRUCTURE AHISampleInfo,0
	ULONG	ahisi_Type			; Format of samples
	APTR	ahisi_Address			; Address to array of samples
	ULONG	ahisi_Length			; Number of samples in array
	LABEL	AHISampleInfo_SIZEOF

 ; AHIAudioModeRequester
	STRUCTURE AHIAudioModeRequester,0
	ULONG	ahiam_AudioID;			; Selected audio mode
	ULONG	ahiam_MixFreq;			; Selected mixing/sampling frequency
	
	WORD	ahiam_LeftEdge;			; Coordinates of requester on exit
	WORD	ahiam_TopEdge;
	WORD	ahiam_Width;
	WORD	ahiam_Height;

	BOOL	ahiam_InfoOpened;		; Info window opened on exit
	WORD	ahiam_InfoLeftEdge;		; Last coordinates of Info window
	WORD	ahiam_InfoTopEdge;
	WORD	ahiam_InfoWidth;
	WORD	ahiam_InfoHeight;

	UWORD	ahiam_ObsoleteUserData0;
	UWORD	ahiam_ObsoleteUserData1;
	UWORD	ahiam_Pad;
	APTR	ahiam_UserData;			; You can store your own data here (V6)

	; Lots of private data follows!
	LABEL	AHIAudioModeRequester_SIZEOF

ahie_Effect	EQU	0

 ; AHIEffMasterVolume
	STRUCTURE AHIEffMasterVolume,0
	ULONG	ahiemv_Effect			; Set to AHIET_MASTERVOLUME
	Fixed	ahiemv_Volume			; See autodocs for range!
	LABEL	AHIEffMasterVolume_SIZEOF

 ; AHIEffOutputBuffer
	STRUCTURE AHIEffOutputBuffer,0
	ULONG	ahieob_Effect			; Set to AHIET_OUTPUTBUFFER
	APTR	ahieob_Func
 ; These fields are filled by AHI
	ULONG	ahieob_Type			; Format of buffer
	APTR	ahieob_Buffer			; Pointer to the sample array
	ULONG	ahieob_Length			; Number of sample frames in buffer
	LABEL	AHIEffOutputBuffer_SIZEOF

 ; AHIEffDSPMask (V4)
	STRUCTURE AHIEffDSPMask,0
	ULONG	ahiedm_Effect			; Set to AHIET_DSPMASK
	UWORD	ahiedm_Channels			; Number of elements in array
	LABEL	ahiedm_Mask			; Here follows the UBYTE array
	LABEL	AHIEffDSPMask_SIZEOF

AHIEDM_WET	EQU	0
AHIEDM_DRY	EQU	1

 ; AHIEffDSPEcho (V4)
	STRUCTURE AHIEffDSPEcho,0
	ULONG	ahiede_Effect			; Set to AHIET_DSPECHO
	ULONG	ahiede_Delay			; In samples
	Fixed	ahiede_Feedback
	Fixed	ahiede_Mix
	Fixed	ahiede_Cross
	LABEL	AHIEffDSPEcho_SIZEOF

 ; AHIEffChannelInfo (V4)
	STRUCTURE AHIEffChannelInfo,0
	ULONG	ahieci_Effect			; Set to AHIET_CHANNELINFO
	APTR	ahieci_Func
	UWORD	ahieci_Channels
	UWORD	ahieci_Pad
 ; The rest is filled by AHI
	LABEL	ahieci_Offset			; The ULONG array follows
 	LABEL	AHIEffChannelInfo_SIZEOF

*** TAGS

AHI_TagBase		EQU TAG_USER
AHI_TagBaseR		EQU AHI_TagBase|$8000

 ; AHI_AllocAudioA tags
AHIA_AudioID		EQU AHI_TagBase+1	; Desired audio mode
AHIA_MixFreq		EQU AHI_TagBase+2	; Suggested mixing frequency
AHIA_Channels		EQU AHI_TagBase+3	; Suggested number of channels
AHIA_Sounds		EQU AHI_TagBase+4	; Number of sounds to use
AHIA_SoundFunc		EQU AHI_TagBase+5	; End-of-Sound Hook
AHIA_PlayerFunc		EQU AHI_TagBase+6	; Player Hook
AHIA_PlayerFreq		EQU AHI_TagBase+7	; Frequency for player Hook (Fixed)
AHIA_MinPlayerFreq	EQU AHI_TagBase+8	; Minimum Frequency for player Hook
AHIA_MaxPlayerFreq	EQU AHI_TagBase+9	; Maximum Frequency for player Hook
AHIA_RecordFunc		EQU AHI_TagBase+10	; Sample recording Hook
AHIA_UserData		EQU AHI_TagBase+11	; What to put in ahiac_UserData
AHIA_AntiClickSamples	EQU AHI_TagBase+13	; # of samples to smooth (V6)

  ; AHI_PlayA tags (V4)
AHIP_BeginChannel 	EQU AHI_TagBase+40	; All command tags should be...
AHIP_EndChannel		EQU AHI_TagBase+41	; ... enclosed by these tags.
AHIP_Freq		EQU AHI_TagBase+50
AHIP_Vol		EQU AHI_TagBase+51
AHIP_Pan		EQU AHI_TagBase+52
AHIP_Sound		EQU AHI_TagBase+53
AHIP_Offset		EQU AHI_TagBase+54
AHIP_Length		EQU AHI_TagBase+55
AHIP_LoopFreq		EQU AHI_TagBase+60
AHIP_LoopVol		EQU AHI_TagBase+61
AHIP_LoopPan		EQU AHI_TagBase+62
AHIP_LoopSound		EQU AHI_TagBase+63
AHIP_LoopOffset		EQU AHI_TagBase+64
AHIP_LoopLength		EQU AHI_TagBase+65

 ; AHI_ControlAudioA tags
AHIC_Play		EQU AHI_TagBase+80	; Boolean
AHIC_Record		EQU AHI_TagBase+81	; Boolean
AHIC_MonitorVolume	EQU AHI_TagBase+82
AHIC_MonitorVolume_Query EQU AHI_TagBase+83	; ti_Data is pointer to Fixed (LONG)
AHIC_MixFreq_Query	EQU AHI_TagBase+84	; ti_Data is pointer to ULONG
* --- New for V2, they will be ignored by V1 ---
AHIC_InputGain		EQU AHI_TagBase+85
AHIC_InputGain_Query	EQU AHI_TagBase+86	; ti_Data is pointer to Fixed (LONG)
AHIC_OutputVolume	EQU AHI_TagBase+87
AHIC_OutputVolume_Query	EQU AHI_TagBase+88	; ti_Data is pointer to Fixed (LONG)
AHIC_Input		EQU AHI_TagBase+89
AHIC_Input_Query	EQU AHI_TagBase+90	; ti_Data is pointer to ULONG
AHIC_Output		EQU AHI_TagBase+91
AHIC_Output_Query	EQU AHI_TagBase+92	; ti_Data is pointer to ULONG

 ; AHI_GetAudioAttrsA tags
AHIDB_AudioID		EQU AHI_TagBase+100
AHIDB_Driver		EQU AHI_TagBaseR+101	; Pointer to name of driver
AHIDB_Flags		EQU AHI_TagBase+102	; Private!
AHIDB_Volume		EQU AHI_TagBase+103	; Boolean
AHIDB_Panning		EQU AHI_TagBase+104	; Boolean
AHIDB_Stereo		EQU AHI_TagBase+105	; Boolean
AHIDB_HiFi		EQU AHI_TagBase+106	; Boolean
AHIDB_PingPong		EQU AHI_TagBase+107	; Boolean
AHIDB_MultTable		EQU AHI_TagBase+108	; Private!
AHIDB_Name		EQU AHI_TagBaseR+109	; Pointer to name of this mode
AHIDB_Bits		EQU AHI_TagBase+110	; Output bits
AHIDB_MaxChannels	EQU AHI_TagBase+111	; Max supported channels
AHIDB_MinMixFreq	EQU AHI_TagBase+112	; Min mixing freq. supported
AHIDB_MaxMixFreq	EQU AHI_TagBase+113	; Max mixing freq. supported
AHIDB_Record		EQU AHI_TagBase+114	; Boolean
AHIDB_Frequencies	EQU AHI_TagBase+115
AHIDB_FrequencyArg	EQU AHI_TagBase+116	; ti_Data is index number!
AHIDB_Frequency		EQU AHI_TagBase+117
AHIDB_Author		EQU AHI_TagBase+118	; Pointer to driver author name
AHIDB_Copyright		EQU AHI_TagBase+119	; Pointer to driver copyright notice
AHIDB_Version		EQU AHI_TagBase+120	; Pointer to driver version string
AHIDB_Annotation	EQU AHI_TagBase+121	; Pointer to driver annotation text
AHIDB_IndexArg		EQU AHI_TagBase+123	; ti_Data is frequency!
AHIDB_BufferLen		EQU AHI_TagBase+122	; Specifies the string buffer size
AHIDB_Index		EQU AHI_TagBase+124
AHIDB_Realtime		EQU AHI_TagBase+125	; Boolean
AHIDB_MaxPlaySamples	EQU AHI_TagBase+126	; It's sample *frames*
AHIDB_MaxRecordSamples	EQU AHI_TagBase+127	; It's sample *frames*
AHIDB_MixFreq	   	EQU AHI_TagBase+128
AHIDB_FullDuplex	EQU AHI_TagBase+129	; Boolean
* --- New for V2, they will be ignored by V1 ---
AHIDB_MinMonitorVolume	EQU AHI_TagBase+130
AHIDB_MaxMonitorVolume	EQU AHI_TagBase+131
AHIDB_MinInputGain	EQU AHI_TagBase+132
AHIDB_MaxInputGain	EQU AHI_TagBase+133
AHIDB_MinOutputVolume	EQU AHI_TagBase+134
AHIDB_MaxOutputVolume	EQU AHI_TagBase+135
AHIDB_Inputs		EQU AHI_TagBase+136
AHIDB_InputArg		EQU AHI_TagBase+137	* ti_Data is input index
AHIDB_Input		EQU AHI_TagBase+138
AHIDB_Outputs		EQU AHI_TagBase+139
AHIDB_OutputArg		EQU AHI_TagBase+140	* ti_Data is input index
AHIDB_Output		EQU AHI_TagBase+141
* --- New for V4, they will be ignored by V2 and earlier ---
AHIDB_Data		EQU AHI_TagBaseR+142	; Private!
AHIDB_DriverBaseName	EQU AHI_TagBaseR+143	; Private!
* --- New for V6, they will be ignored by V4 and earlier ---
AHIDB_MultiChannel	EQU AHI_TagBase+144	; Boolean

 ; AHI_BestAudioIDA tags
* --- New for V4, they will be ignored by V2 and earlier ---
AHIB_Dizzy		EQU (AHI_TagBase+190)

 ; AHI_AudioRequestA tags
   ; Window control
AHIR_Window		EQU AHI_TagBase+200	; Parent window
AHIR_Screen		EQU AHI_TagBase+201	; Screen to open on if no window
AHIR_PubScreenName	EQU AHI_TagBase+202	; Name of public screen
AHIR_PrivateIDCMP	EQU AHI_TagBase+203	; Allocate private IDCMP?
AHIR_IntuiMsgFunc	EQU AHI_TagBase+204	; Function to handle IntuiMessages
AHIR_SleepWindow	EQU AHI_TagBase+205	; Block input in AHIR_Window?
AHIR_ObsoleteUserData	EQU AHI_TagBase+206	; V4 UserData
AHIR_UserData		EQU AHI_TagBase+207	; What to put in ahiam_UserData (V6)
   ; Text display
AHIR_TextAttr		EQU AHI_TagBase+220	; Text font to use for gadget text
AHIR_Locale		EQU AHI_TagBase+221	; Locale to use for text
AHIR_TitleText		EQU AHI_TagBase+222	; Title of requester
AHIR_PositiveText	EQU AHI_TagBase+223	; Positive gadget text
AHIR_NegativeText	EQU AHI_TagBase+224	; Negative gadget text
   ; Initial settings
AHIR_InitialLeftEdge	EQU AHI_TagBase+240	; Initial requester coordinates
AHIR_InitialTopEdge	EQU AHI_TagBase+241
AHIR_InitialWidth 	EQU AHI_TagBase+242	; Initial requester dimensions
AHIR_InitialHeight	EQU AHI_TagBase+243
AHIR_InitialAudioID	EQU AHI_TagBase+244	; Initial audio mode id
AHIR_InitialMixFreq	EQU AHI_TagBase+245	; Initial mixing/sampling frequency
AHIR_InitialInfoOpened	EQU AHI_TagBase+246	; Info window initially opened?
AHIR_InitialInfoLeftEdge EQU AHI_TagBase+247	; Initial Info window coords.
AHIR_InitialInfoTopEdge	EQU AHI_TagBase+248
AHIR_InitialInfoWidth	EQU AHI_TagBase+249	; Not used!
AHIR_InitialInfoHeight	EQU AHI_TagBase+250	; Not used!
   ; Options
AHIR_DoMixFreq		EQU AHI_TagBase+260	; Allow selection of mixing frequency?
AHIR_DoDefaultMode	EQU AHI_TagBase+261	; Allow selection of default mode? (V4)
   ; Filtering
AHIR_FilterTags		EQU AHI_TagBase+270	; Pointer to filter taglist
AHIR_FilterFunc		EQU AHI_TagBase+271	; Function to filter mode id's


*** DEFS

AHI_INVALID_ID		EQU ~0			; Invalid Audio ID
AHI_DEFAULT_ID		EQU $00000000		; Only for AHI_AllocAudioA()!
AHI_LOOPBACK_ID		EQU $00000001		; Special sample render ID
AHI_DEFAULT_FREQ	EQU 0			; Only for AHI_AllocAudioA()!
AHI_MIXFREQ		EQU ~0			; Special frequency for AHI_SetFreq()
AHI_NOSOUND		EQU ~0			; Turns a channel off

 ; Set#? Flags
	BITDEF	AHIS,IMM,0			; Trigger action immediately
	BITDEF	AHIS,NODELAY,1			; Don't wait for zero-crossing 

AHISF_NONE		EQU 0			; No flags (V6)


 ; Effect types
AHIET_CANCEL		EQU 1<<31		; OR with effect to disable
AHIET_MASTERVOLUME	EQU 1
AHIET_OUTPUTBUFFER	EQU 2
* --- New for V4 ---
AHIET_DSPMASK		EQU 3
AHIET_DSPECHO		EQU 4
AHIET_CHANNELINFO	EQU 5

 ; Sound types
AHIST_NOTYPE		EQU ~0			; Private
AHIST_SAMPLE		EQU 0			; 8 or 16 bit sample
AHIST_DYNAMICSAMPLE	EQU 1			; Dynamic sample
AHIST_INPUT		EQU 1<<29		; The input from your sampler
AHIST_BW		EQU 1<<30		; Private

 ; Sample types
; Note that only AHIST_M8S, AHIST_S8S, AHIST_M16S and AHIST_S16S
; (plus AHIST_M32S, AHIST_S32S and AHIST_L7_1 in V6)
; are supported by AHI_LoadSound().
AHIST_M8S		EQU 0			; Mono, 8 bit signed (BYTE)
AHIST_M16S		EQU 1			; Mono, 16 bit signed (WORD)
AHIST_S8S		EQU 2			; Stereo, 8 bit signed (2×BYTE)
AHIST_S16S		EQU 3			; Stereo, 16 bit signed (2×WORD)
AHIST_M32S		EQU 8			; Mono, 32 bit signed (LONG)
AHIST_S32S		EQU 10			; Stereo, 32 bit signed (2×LONG)

AHIST_M8U		EQU 4			; OBSOLETE!

AHIST_L7_1		EQU $00c3000a		; 7.1, 32 bit signed (8×LONG)

 ; Error codes
AHIE_OK			EQU 0			; No error
AHIE_NOMEM		EQU 1			; Out of memory
AHIE_BADSOUNDTYPE	EQU 2			; Unknown sound type
AHIE_BADSAMPLETYPE	EQU 3			; Unknown/unsupported sample type
AHIE_ABORTED		EQU 4			; User-triggered abortion
AHIE_UNKNOWN		EQU 5			; Error, but unknown
AHIE_HALFDUPLEX		EQU 6			; CMD_WRITE/CMD_READ failure



*- DEVICE INTERFACE DEFINITIONS FOLLOWS ------------------------------------*

 ; Device units

AHI_DEFAULT_UNIT	EQU 0
AHI_NO_UNIT		EQU 255


 ; The preference file

ID_AHIU 		EQU "AHIU"
ID_AHIG 		EQU "AHIG"

	STRUCTURE AHIUnitPrefs,0
	UBYTE	ahiup_Unit
        UBYTE	ahiup_Obsolete				; Was ahiup_ScaleMode
        UWORD	ahiup_Channels
        ULONG	ahiup_AudioMode
        ULONG	ahiup_Frequency
        Fixed	ahiup_MonitorVolume
        Fixed	ahiup_InputGain
        Fixed	ahiup_OutputVolume
        ULONG	ahiup_Input
        ULONG	ahiup_Output
	LABEL	AHIUnitPrefs_SIZEOF


	STRUCTURE AHIGlobalPrefs,0
	UWORD	ahigp_DebugLevel			; Range: 0-3 (for None, Low,
							; High and All
	BOOL	ahigp_DisableSurround
	BOOL	ahigp_DisableEcho
	BOOL	ahigp_FastEcho
	Fixed	ahigp_MaxCPU
	BOOL	ahigp_ClipMasterVolume
	UWORD	ahigp_Pad;
	Fixed	ahigp_AntiClickTime;			; In seconds (V6)
        UWORD	ahigp_ScaleMode				; See below (V6)
	LABEL	AHIGlobalPrefs_SIZEOF

 ; Debug levels

AHI_DEBUG_NONE		EQU (0)
AHI_DEBUG_LOW		EQU (1)
AHI_DEBUG_HIGH		EQU (2)
AHI_DEBUG_ALL		EQU (3)

 ; Scale modes

AHI_SCALE_FIXED_SAFE	EQU (0)			; x=y*1/max(ch)
AHI_SCALE_DYNAMIC_SAFE	EQU (1)			; x=y*1/ch
AHI_SCALE_FIXED_0_DB	EQU (2)			; x=y
AHI_SCALE_FIXED_3_DB	EQU (3)			; x=y*1/sqrt(2)
AHI_SCALE_FIXED_6_DB	EQU (4)			; x=y*1/2

 ; AHIRequest

	STRUCTURE AHIRequest,0
	STRUCT	ahir_Std,IOSTD_SIZE		; Standard IO request
	UWORD	ahir_Version			; Needed version
* --- New for V4, they will be ignored by V2 and earlier ---
	UWORD	ahir_Pad1
	ULONG	ahir_Private1			; Hands off!
	ULONG	ahir_Private2			; Hands off!
	ULONG	ahir_Type			; Sample format
	ULONG	ahir_Frequency			; Sample/Record frequency
	Fixed	ahir_Volume			; Sample volume
	Fixed	ahir_Position			; Stereo position
	APTR	ahir_Link			; For double buffering
	LABEL	AHIRequest_SIZEOF

 ; Flags for OpenDevice()

	BITDEF	AHID,NOMODESCAN,0

	ENDC ; DEVICES_AHI_I
