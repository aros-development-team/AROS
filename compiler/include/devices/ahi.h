#ifndef DEVICES_AHI_H
#define DEVICES_AHI_H

/*
**	$VER: ahi.h 6.0 (02.02.2005)
**
**	ahi.device definitions
**
**	(C) Copyright 1994-2005 Martin Blom
**	All Rights Reserved.
**
** (TAB SIZE: 8)
*/

/*****************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_IO_H
#include <exec/io.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

/*****************************************************************************/

#ifndef EIGHTSVX_H 				/* Do not define Fixed twice */

typedef LONG	Fixed;				/* A fixed-point value, 16 bits
						   to the left of the point and
						   16 bits to the right */
#endif
typedef Fixed	sposition;

/*** STRUCTURES */

 /* AHIAudioCtrl */
struct AHIAudioCtrl
{
	APTR	ahiac_UserData;
	/* Lots of private data follows! */
};

 /* AHISoundMessage */
struct AHISoundMessage
{
 	UWORD	ahism_Channel;
};

 /* AHIRecordMessage */
struct AHIRecordMessage
{
	ULONG	ahirm_Type;			/* Format of buffer (object) */
	APTR	ahirm_Buffer;			/* Pointer to the sample array */
	ULONG	ahirm_Length;			/* Number of sample frames in buffer */
};

 /* AHISampleInfo */
struct AHISampleInfo
{
	ULONG	ahisi_Type;			/* Format of samples */
	APTR	ahisi_Address;			/* Address to array of samples */
	ULONG	ahisi_Length;			/* Number of samples in array */
};


 /* AHIAudioModeRequester */
struct AHIAudioModeRequester
{
	ULONG	ahiam_AudioID;			/* Selected audio mode */
	ULONG	ahiam_MixFreq;			/* Selected mixing/sampling frequency */
	
	WORD	ahiam_LeftEdge;			/* Coordinates of requester on exit */
	WORD	ahiam_TopEdge;
	WORD	ahiam_Width;
	WORD	ahiam_Height;

	BOOL	ahiam_InfoOpened;		/* Info window opened on exit? */
	WORD	ahiam_InfoLeftEdge;		/* Last coordinates of Info window */
	WORD	ahiam_InfoTopEdge;
	WORD	ahiam_InfoWidth;
	WORD	ahiam_InfoHeight;

	UWORD	ahiam_ObsoleteUserData[2];
	UWORD	ahiam_Pad;
	APTR	ahiam_UserData;			/* You can store your own data here (V6) */
	/* Lots of private data follows! */
};

 /* AHIEffMasterVolume */
struct AHIEffMasterVolume
{
	ULONG	ahie_Effect;			/* Set to AHIET_MASTERVOLUME */
	Fixed	ahiemv_Volume;			/* See autodocs for range! */
};

 /* AHIEffOutputBuffer */
struct AHIEffOutputBuffer
{
	ULONG		 ahie_Effect;		/* Set to AHIET_OUTPUTBUFFER */
	struct Hook	*ahieob_Func;
 /* These fields are filled by AHI */
	ULONG		 ahieob_Type;		/* Format of buffer */
	APTR		 ahieob_Buffer;		/* Pointer to the sample array */
	ULONG		 ahieob_Length;		/* Number of sample frames in buffer */
};

 /* AHIEffDSPMask (V4) */
struct AHIEffDSPMask
{
	ULONG	ahie_Effect;			/* Set to AHIET_DSPMASK */
	UWORD	ahiedm_Channels;		/* Number of elements in array */
	UBYTE	ahiedm_Mask[0];			/* Here follows the array */
};

#define AHIEDM_WET		(0)
#define AHIEDM_DRY		(1)

 /* AHIEffDSPEcho (V4) */
struct AHIEffDSPEcho
{
	ULONG	ahie_Effect;			/* Set to AHIET_DSPECHO */
	ULONG	ahiede_Delay;			/* In samples */
	Fixed	ahiede_Feedback;
	Fixed	ahiede_Mix;
	Fixed	ahiede_Cross;
};

#define AHIDSPEcho AHIEffDSPEcho		/* Fix for error in V4 includes	*/

 /* AHIEffChannelInfo (V4) */

struct AHIEffChannelInfo
{
	ULONG		 ahie_Effect;		/* Set to AHIET_CHANNELINFO */
	struct Hook	*ahieci_Func;
	UWORD		 ahieci_Channels;
	UWORD		 ahieci_Pad;
 /* The rest is filled by AHI */
 	ULONG		 ahieci_Offset[0];	/* The array follows */
};

/*** TAGS */

#define AHI_TagBase		(TAG_USER)
#define AHI_TagBaseR		(AHI_TagBase|0x8000)

 /* AHI_AllocAudioA tags */
#define AHIA_AudioID		(AHI_TagBase+1)		/* Desired audio mode */
#define AHIA_MixFreq		(AHI_TagBase+2)		/* Suggested mixing frequency */
#define AHIA_Channels		(AHI_TagBase+3)		/* Suggested number of channels */
#define AHIA_Sounds		(AHI_TagBase+4)		/* Number of sounds to use */
#define AHIA_SoundFunc		(AHI_TagBase+5)		/* End-of-Sound Hook */
#define AHIA_PlayerFunc		(AHI_TagBase+6)		/* Player Hook */
#define AHIA_PlayerFreq		(AHI_TagBase+7)		/* Frequency for player Hook (Fixed)*/
#define AHIA_MinPlayerFreq	(AHI_TagBase+8)		/* Minimum Frequency for player Hook */
#define AHIA_MaxPlayerFreq	(AHI_TagBase+9)		/* Maximum Frequency for player Hook */
#define AHIA_RecordFunc		(AHI_TagBase+10)	/* Sample recording Hook */
#define AHIA_UserData		(AHI_TagBase+11)	/* What to put in ahiac_UserData */
#define AHIA_AntiClickSamples	(AHI_TagBase+13)	/* # of samples to smooth (V6)	*/

  /* AHI_PlayA tags (V4) */
#define AHIP_BeginChannel	(AHI_TagBase+40)	/* All command tags should be... */
#define AHIP_EndChannel		(AHI_TagBase+41)	/* ... enclosed by these tags. */
#define AHIP_Freq		(AHI_TagBase+50)
#define AHIP_Vol		(AHI_TagBase+51)
#define AHIP_Pan		(AHI_TagBase+52)
#define AHIP_Sound		(AHI_TagBase+53)
#define AHIP_Offset		(AHI_TagBase+54)
#define AHIP_Length		(AHI_TagBase+55)
#define AHIP_LoopFreq		(AHI_TagBase+60)
#define AHIP_LoopVol		(AHI_TagBase+61)
#define AHIP_LoopPan		(AHI_TagBase+62)
#define AHIP_LoopSound		(AHI_TagBase+63)
#define AHIP_LoopOffset		(AHI_TagBase+64)
#define AHIP_LoopLength		(AHI_TagBase+65)

 /* AHI_ControlAudioA tags */
#define AHIC_Play		(AHI_TagBase+80)	/* Boolean */
#define AHIC_Record		(AHI_TagBase+81)	/* Boolean */
#define AHIC_MonitorVolume	(AHI_TagBase+82)
#define AHIC_MonitorVolume_Query (AHI_TagBase+83)	/* ti_Data is pointer to Fixed (LONG) */
#define AHIC_MixFreq_Query	(AHI_TagBase+84)	/* ti_Data is pointer to ULONG */
/* --- New for V2, they will be ignored by V1 --- */
#define AHIC_InputGain		(AHI_TagBase+85)
#define AHIC_InputGain_Query	(AHI_TagBase+86)	/* ti_Data is pointer to Fixed (LONG) */
#define AHIC_OutputVolume	(AHI_TagBase+87)
#define AHIC_OutputVolume_Query	(AHI_TagBase+88)	/* ti_Data is pointer to Fixed (LONG) */
#define AHIC_Input		(AHI_TagBase+89)
#define AHIC_Input_Query	(AHI_TagBase+90)	/* ti_Data is pointer to ULONG */
#define AHIC_Output		(AHI_TagBase+91)
#define AHIC_Output_Query	(AHI_TagBase+92)	/* ti_Data is pointer to ULONG */

 /* AHI_GetAudioAttrsA tags */
#define AHIDB_AudioID		(AHI_TagBase+100)
#define AHIDB_Driver		(AHI_TagBaseR+101)	/* Pointer to name of driver */
#define AHIDB_Flags		(AHI_TagBase+102)	/* Private! */
#define AHIDB_Volume		(AHI_TagBase+103)	/* Boolean */
#define AHIDB_Panning		(AHI_TagBase+104)	/* Boolean */
#define AHIDB_Stereo		(AHI_TagBase+105)	/* Boolean */
#define AHIDB_HiFi		(AHI_TagBase+106)	/* Boolean */
#define AHIDB_PingPong		(AHI_TagBase+107)	/* Boolean */
#define AHIDB_MultTable		(AHI_TagBase+108)	/* Private! */
#define AHIDB_Name		(AHI_TagBaseR+109)	/* Pointer to name of this mode */
#define AHIDB_Bits		(AHI_TagBase+110)	/* Output bits */
#define AHIDB_MaxChannels	(AHI_TagBase+111)	/* Max supported channels */
#define AHIDB_MinMixFreq	(AHI_TagBase+112)	/* Min mixing freq. supported */
#define AHIDB_MaxMixFreq	(AHI_TagBase+113)	/* Max mixing freq. supported */
#define AHIDB_Record		(AHI_TagBase+114)	/* Boolean */
#define AHIDB_Frequencies	(AHI_TagBase+115)
#define AHIDB_FrequencyArg	(AHI_TagBase+116)	/* ti_Data is frequency index */
#define AHIDB_Frequency		(AHI_TagBase+117)
#define AHIDB_Author		(AHI_TagBase+118)	/* Pointer to driver author name */
#define AHIDB_Copyright		(AHI_TagBase+119)	/* Pointer to driver copyright notice */
#define AHIDB_Version		(AHI_TagBase+120)	/* Pointer to driver version string */
#define AHIDB_Annotation	(AHI_TagBase+121)	/* Pointer to driver annotation text */
#define AHIDB_BufferLen		(AHI_TagBase+122)	/* Specifies the string buffer size */
#define AHIDB_IndexArg		(AHI_TagBase+123)	/* ti_Data is frequency! */
#define AHIDB_Index		(AHI_TagBase+124)
#define AHIDB_Realtime		(AHI_TagBase+125)	/* Boolean */
#define AHIDB_MaxPlaySamples	(AHI_TagBase+126)	/* It's sample *frames* */
#define AHIDB_MaxRecordSamples	(AHI_TagBase+127)	/* It's sample *frames* */
#define AHIDB_FullDuplex	(AHI_TagBase+129)	/* Boolean */
/* --- New for V2, they will be ignored by V1 --- */
#define AHIDB_MinMonitorVolume	(AHI_TagBase+130)
#define AHIDB_MaxMonitorVolume	(AHI_TagBase+131)
#define AHIDB_MinInputGain	(AHI_TagBase+132)
#define AHIDB_MaxInputGain	(AHI_TagBase+133)
#define AHIDB_MinOutputVolume	(AHI_TagBase+134)
#define AHIDB_MaxOutputVolume	(AHI_TagBase+135)
#define AHIDB_Inputs		(AHI_TagBase+136)
#define AHIDB_InputArg		(AHI_TagBase+137)	/* ti_Data is input index */
#define AHIDB_Input		(AHI_TagBase+138)
#define AHIDB_Outputs		(AHI_TagBase+139)
#define AHIDB_OutputArg		(AHI_TagBase+140)	/* ti_Data is input index */
#define AHIDB_Output		(AHI_TagBase+141)
/* --- New for V4, they will be ignored by V2 and earlier --- */
#define AHIDB_Data		(AHI_TagBaseR+142)	/* Private! */
#define AHIDB_DriverBaseName	(AHI_TagBaseR+143)	/* Private! */
/* --- New for V6, they will be ignored by V4 and earlier --- */
#define AHIDB_MultiChannel	(AHI_TagBase+144)	/* Boolean */

 /* AHI_BestAudioIDA tags */
/* --- New for V4, they will be ignored by V2 and earlier --- */
#define AHIB_Dizzy		(AHI_TagBase+190)

 /* AHI_AudioRequestA tags */
	/* Window control */
#define AHIR_Window		(AHI_TagBase+200)	/* Parent window */
#define AHIR_Screen		(AHI_TagBase+201)	/* Screen to open on if no window */
#define AHIR_PubScreenName	(AHI_TagBase+202)	/* Name of public screen */
#define AHIR_PrivateIDCMP	(AHI_TagBase+203)	/* Allocate private IDCMP? */
#define AHIR_IntuiMsgFunc	(AHI_TagBase+204)	/* Function to handle IntuiMessages */
#define AHIR_SleepWindow	(AHI_TagBase+205)	/* Block input in AHIR_Window? */
#define AHIR_ObsoleteUserData	(AHI_TagBase+206)	/* V4 UserData */
#define AHIR_UserData		(AHI_TagBase+207)	/* What to put in ahiam_UserData (V6) */
	/* Text display */
#define AHIR_TextAttr		(AHI_TagBase+220)	/* Text font to use for gadget text */
#define AHIR_Locale		(AHI_TagBase+221)	/* Locale to use for text */
#define AHIR_TitleText		(AHI_TagBase+222)	/* Title of requester */
#define AHIR_PositiveText	(AHI_TagBase+223)	/* Positive gadget text */
#define AHIR_NegativeText	(AHI_TagBase+224)	/* Negative gadget text */
	/* Initial settings */
#define AHIR_InitialLeftEdge	(AHI_TagBase+240)	/* Initial requester coordinates */
#define AHIR_InitialTopEdge	(AHI_TagBase+241)
#define AHIR_InitialWidth	(AHI_TagBase+242)	/* Initial requester dimensions */
#define AHIR_InitialHeight	(AHI_TagBase+243)
#define AHIR_InitialAudioID	(AHI_TagBase+244)	/* Initial audio mode id */
#define AHIR_InitialMixFreq	(AHI_TagBase+245)	/* Initial mixing/sampling frequency */
#define AHIR_InitialInfoOpened	(AHI_TagBase+246)	/* Info window initially opened? */
#define AHIR_InitialInfoLeftEdge (AHI_TagBase+247)	/* Initial Info window coords. */
#define AHIR_InitialInfoTopEdge (AHI_TagBase+248)
#define AHIR_InitialInfoWidth	(AHI_TagBase+249)	/* Not used! */
#define AHIR_InitialInfoHeight	(AHI_TagBase+250)	/* Not used! */
	/* Options */
#define AHIR_DoMixFreq		(AHI_TagBase+260)	/* Allow selection of mixing frequency? */
#define AHIR_DoDefaultMode	(AHI_TagBase+261)	/* Allow selection of default mode? (V4) */
	/* Filtering */
#define AHIR_FilterTags		(AHI_TagBase+270)	/* Pointer to filter taglist */
#define AHIR_FilterFunc		(AHI_TagBase+271)	/* Function to filter mode id's */

/*** DEFS */

#define AHINAME			"ahi.device"
#define AHI_INVALID_ID		(~0UL)			/* Invalid Audio ID */
#define AHI_DEFAULT_ID		(0x00000000UL)		/* Only for AHI_AllocAudioA()! */
#define AHI_LOOPBACK_ID		(0x00000001UL)		/* Special sample render Audio ID */
#define AHI_DEFAULT_FREQ	(0UL)			/* Only for AHI_AllocAudioA()! */
#define AHI_MIXFREQ		(~0UL)			/* Special frequency for AHI_SetFreq() */
#define AHI_NOSOUND		(0xffffU)		/* Turns a channel off */

 /* Set#? Flags */
#define AHISF_IMM		(1UL<<0)	/* Trigger action immediately	*/
#define AHISF_NODELAY		(1UL<<1)	/* Don't wait for zero-crossing */

#define AHISF_NONE		(0UL)		/* No flags (V6)		*/

#define AHISB_IMM		(0UL)
#define AHISB_NODELAY		(1UL)

 /* Effect Types */
#define AHIET_CANCEL		(1UL<<31)		/* OR with effect to disable */
#define AHIET_MASTERVOLUME	(1UL)
#define AHIET_OUTPUTBUFFER	(2UL)
/* --- New for V4 --- */
#define AHIET_DSPMASK		(3UL)
#define AHIET_DSPECHO		(4UL)
#define AHIET_CHANNELINFO	(5UL)

 /* Sound Types */
#define AHIST_NOTYPE		(~0UL)			/* Private */
#define AHIST_SAMPLE		(0UL)			/* 8 or 16 bit sample */
#define AHIST_DYNAMICSAMPLE	(1UL)			/* Dynamic sample */
#define AHIST_INPUT		(1UL<<29)		/* The input from your sampler */
#define AHIST_BW		(1UL<<30)		/* Private */

 /* Sample types */
/* Note that only AHIST_M8S, AHIST_S8S, AHIST_M16S and AHIST_S16S
   (plus AHIST_M32S, AHIST_S32S and AHIST_L7_1 in V6)
   are supported by AHI_LoadSound(). */
#define AHIST_M8S		(0UL)			/* Mono, 8 bit signed (BYTE) */
#define AHIST_M16S		(1UL)			/* Mono, 16 bit signed (WORD) */
#define AHIST_S8S		(2UL)			/* Stereo, 8 bit signed (2×BYTE) */
#define AHIST_S16S		(3UL)			/* Stereo, 16 bit signed (2×WORD) */
#define AHIST_M32S		(8UL)			/* Mono, 32 bit signed (LONG) */
#define AHIST_S32S		(10UL)			/* Stereo, 32 bit signed (2×LONG) */

#define AHIST_M8U		(4UL)			/* OBSOLETE! */
#define AHIST_L7_1		(0x00c3000aUL)		/* 7.1, 32 bit signed (8×LONG) */

 /* Error codes */
#define AHIE_OK			(0UL)			/* No error */
#define AHIE_NOMEM		(1UL)			/* Out of memory */
#define AHIE_BADSOUNDTYPE	(2UL)			/* Unknown sound type */
#define AHIE_BADSAMPLETYPE	(3UL)			/* Unknown/unsupported sample type */
#define AHIE_ABORTED		(4UL)			/* User-triggered abortion */
#define AHIE_UNKNOWN		(5UL)			/* Error, but unknown */
#define AHIE_HALFDUPLEX		(6UL)			/* CMD_WRITE/CMD_READ failure */



/* DEVICE INTERFACE DEFINITIONS FOLLOWS ************************************/

 /* Device units */

#define AHI_DEFAULT_UNIT	(0U)
#define AHI_NO_UNIT		(255U)


 /* The preference file */

#define ID_AHIU MAKE_ID('A','H','I','U')
#define ID_AHIG MAKE_ID('A','H','I','G')

struct AHIUnitPrefs
{
	UBYTE	ahiup_Unit;
	UBYTE   ahiup_Obsolete;                         /* Was ahiup_ScaleMode */
        UWORD	ahiup_Channels;
        ULONG	ahiup_AudioMode;
        ULONG	ahiup_Frequency;
        Fixed	ahiup_MonitorVolume;
        Fixed	ahiup_InputGain;
        Fixed	ahiup_OutputVolume;
        ULONG	ahiup_Input;
        ULONG	ahiup_Output;
};

struct AHIGlobalPrefs
{
	UWORD	ahigp_DebugLevel;			/* Range: 0-3 (for None, Low,
							   High and All) */
	BOOL	ahigp_DisableSurround;
	BOOL	ahigp_DisableEcho;
	BOOL	ahigp_FastEcho;
	Fixed	ahigp_MaxCPU;
	BOOL	ahigp_ClipMasterVolume;
	UWORD	ahigp_Pad;
	Fixed	ahigp_AntiClickTime;			/* In seconds (V6) */
	UWORD   ahigp_ScaleMode;			/* See below (V6) */
};

 /* Debug levels */
#define AHI_DEBUG_NONE		(0U)
#define AHI_DEBUG_LOW		(1U)
#define AHI_DEBUG_HIGH		(2U)
#define AHI_DEBUG_ALL		(3U)

 /* Scale modes */
#define AHI_SCALE_FIXED_SAFE	(0U)			/* x=y*1/max(ch)	*/
#define AHI_SCALE_DYNAMIC_SAFE	(1U)			/* x=y*1/ch		*/
#define AHI_SCALE_FIXED_0_DB	(2U)			/* x=y			*/
#define AHI_SCALE_FIXED_3_DB	(3U)			/* x=y*1/sqrt(2)	*/
#define AHI_SCALE_FIXED_6_DB	(4U)			/* x=y*1/2		*/

 /* AHIRequest */

struct AHIRequest
{
	struct	IOStdReq	 ahir_Std;		/* Standard IO request */
	UWORD			 ahir_Version;		/* Needed version */
/* --- New for V4, they will be ignored by V2 and earlier --- */
	UWORD			 ahir_Pad1;
	ULONG			 ahir_Private[2];	/* Hands off! */
	ULONG			 ahir_Type;		/* Sample format */
	ULONG			 ahir_Frequency;	/* Sample/Record frequency */
	Fixed			 ahir_Volume;		/* Sample volume */
	Fixed			 ahir_Position;		/* Stereo position */
	struct AHIRequest 	*ahir_Link;		/* For double buffering */
};

 /* Flags for OpenDevice() */

#define	AHIDF_NOMODESCAN	(1UL<<0)
#define	AHIDB_NOMODESCAN	(0UL)

#endif /* DEVICES_AHI_H */
