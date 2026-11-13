	IFND LIBRARIES_AHISUB_I
LIBRARIES_AHISUB_I	SET	1

**
**	$VER: ahi_sub.i 6.0 (02.02.2005)
**	:ts=8 (TAB SIZE: 8)
**
**	ahi/[driver].audio definitions
**
**	(C) Copyright 1994-2005 Martin Blom
**	All Rights Reserved.
**
**

*------------------------------------------------------------------------*


	IFND EXEC_TYPES_I
	INCLUDE 'exec/types.i'
	ENDC

	IFND UTILITY_TAGITEM_I
	INCLUDE 'utility/tagitem.i'
	ENDC

	IFND DEVICES_AHI_I
	INCLUDE 'devices/ahi.i'
	ENDC

	IFND LIBRARIES_IFFPARSE_I
	INCLUDE 'libraries/iffparse.i'
	ENDC

*------------------------------------------------------------------------*

*** STRUCTUES

; AHIAudioCtrlDrv
	STRUCTURE AHIAudioCtrlDrv,0
	STRUCT	ahiac_AudioCtrl,AHIAudioCtrl_SIZEOF
	ULONG	ahiac_Flags			; See below for definition
	APTR	ahiac_SoundFunc			; AHIA_SoundFunc (Hook)
	APTR	ahiac_PlayerFunc		; AHIA_PlayerFunc (Hook)
	Fixed	ahiac_PlayerFreq		; AHIA_PlayerFreq
	Fixed	ahiac_MinPlayerFreq		; AHIA_MinPlayerFreq
	Fixed	ahiac_MaxPlayerFreq		; AHIA_MaxPlayerFreq
	ULONG	ahiac_MixFreq			; AHIA_MixFreq
	UWORD	ahiac_Channels			; AHIA_Channels
	UWORD	ahiac_Sounds			; AHIA_Sounds

	ULONG	ahiac_DriverData		; Unused. Store whatever you want here.

	APTR	ahiac_MixerFunc			; Mixing routine Hook
	APTR	ahiac_SamplerFunc		; Sampler routine Hook
	ULONG	ahiac_Obsolete
	ULONG	ahiac_BuffSamples		; Samples to mix this pass.
	ULONG	ahiac_MinBuffSamples		; Min. samples to mix each pass.
	ULONG	ahiac_MaxBuffSamples		; Max. samples to mix each pass.
	ULONG	ahiac_BuffSize			; Buffer size ahiac_MixerFunc needs.
	ULONG	ahiac_BuffType			; Buffer format (V2)
	FPTR	ahiac_PreTimer			; Call before mixing (V4)
	FPTR	ahiac_PostTimer			; Call after mixing (V4)
	ULONG	ahiac_AntiClickSamples		; AntiClick samples (V6)
	APTR	ahiac_PreTimerFunc		; A Hook wrapper for ahiac_PreTimer (V6)
	APTR	ahiac_PostTimerFunc		; A Hook wrapper for ahiac_PostTimer (V6)

; The rest is PRIVATE! Hands off! They may change any time.
;	[lots of private stuff]
	LABEL	AHIAudioCtrlDrv_SIZEOF		; Do not use!

*** TAGS

AHIDB_UserBase	EQU AHI_TagBase+500		; Use for driver specific tags


***DEFS

 ; AHIsub_AllocAudio() return flags
	BITDEF	AHIS,ERROR,0
	BITDEF	AHIS,MIXING,1
	BITDEF	AHIS,TIMING,2
	BITDEF	AHIS,KNOWSTEREO,3
	BITDEF	AHIS,KNOWHIFI,4
	BITDEF	AHIS,CANRECORD,5
	BITDEF	AHIS,CANPOSTPROCESS,6
	BITDEF	AHIS,KNOWMULTICHANNEL,7

 ; AHIsub_Start() and AHIsub_Stop() flags
	BITDEF	AHIS,PLAY,0
	BITDEF	AHIS,RECORD,1

 ; ahiac_Flags
	BITDEF	AHIAC,VOL,0
	BITDEF	AHIAC,PAN,1
	BITDEF	AHIAC,STEREO,2
	BITDEF	AHIAC,HIFI,3
	BITDEF	AHIAC,PINGPONG,4
	BITDEF	AHIAC,RECORD,5
	BITDEF	AHIAC,MULTTAB,6			; Private!
	BITDEF	AHIAC,MULTICHANNEL,7

 ; AHIsub_Set#? and AHIsub_(Un)LoadSound return code
AHIS_UNKNOWN	EQU	~0

 ; IFF chunk names for the audio mode file
ID_AHIM		EQU	'AHIM'			; AHI Modes
ID_AUDN		EQU	'AUDN'			; AUDio driver Name
ID_AUDD		EQU	'AUDD'			; AUDio driver Data
ID_AUDM		EQU	'AUDM'			; AUDio Mode

	ENDC ; LIBRARIES_AHISUB_I
