#ifndef LIBRARIES_AHISUB_H
#define LIBRARIES_AHISUB_H

/*
**	$VER: ahi_sub.h 6.0 (02.02.2005)
**	:ts=8 (TAB SIZE: 8)
**
**	ahi/[driver].audio definitions
**
**	(C) Copyright 1994-2005 Martin Blom
**	All Rights Reserved.
**
*/

/*****************************************************************************/


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef DEVICES_AHI_H
#include <devices/ahi.h>
#endif

#ifndef IFF_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

/*****************************************************************************/

/*** STRUCTURES */

/* AHIAudioCtrlDrv */
struct AHIAudioCtrlDrv
{
	struct AHIAudioCtrl ahiac_AudioCtrl;
	ULONG	     ahiac_Flags;		/* See below for definition	*/
	struct Hook *ahiac_SoundFunc;		/* AHIA_SoundFunc		*/
	struct Hook *ahiac_PlayerFunc;		/* AHIA_PlayerFunc		*/
	Fixed	     ahiac_PlayerFreq;		/* AHIA_PlayerFreq		*/
	Fixed	     ahiac_MinPlayerFreq;	/* AHIA_MinPlayerFreq		*/
	Fixed	     ahiac_MaxPlayerFreq;	/* AHIA_MaxPlayerFreq		*/
	ULONG	     ahiac_MixFreq;		/* AHIA_MixFreq			*/
	UWORD	     ahiac_Channels;		/* AHIA_Channels		*/
	UWORD	     ahiac_Sounds;		/* AHIA_Sounds			*/

	APTR	     ahiac_DriverData;		/* Unused. Store whatever you want here. */

	struct Hook *ahiac_MixerFunc;		/* Mixing routine Hook		*/
	struct Hook *ahiac_SamplerFunc;		/* Sampler routine Hook		*/
	ULONG	     ahiac_Obsolete;
	ULONG	     ahiac_BuffSamples;		/* Samples to mix this pass.	*/
	ULONG	     ahiac_MinBuffSamples;	/* Min. samples to mix each pass. */
	ULONG	     ahiac_MaxBuffSamples;	/* Max. samples to mix each pass. */
	ULONG	     ahiac_BuffSize;		/* Buffer size ahiac_MixerFunc needs. */
	ULONG	     ahiac_BuffType;		/* Buffer format (V2)		*/
	BOOL	   (*ahiac_PreTimer)(void);	/* Call before mixing (V4)	*/
	void	   (*ahiac_PostTimer)(void);	/* Call after mixing (V4)	*/
	ULONG	     ahiac_AntiClickSamples;	/* AntiClick samples (V6)	*/
	struct Hook *ahiac_PreTimerFunc;        /* A Hook wrapper for ahiac_PreTimer (V6) */
	struct Hook *ahiac_PostTimerFunc;       /* A Hook wrapper for ahiac_PostTimer (V6) */

/* The rest is PRIVATE! Hands off! They may change any time.
	[lots of private stuff] */
};

/*** TAGS */

#define	AHIDB_UserBase	(AHI_TagBase+500)	/* Use for driver specific tags	*/


/*** DEFS */

 /* AHIsub_AllocAudio return flags */
#define AHISF_ERROR		(1<<0)
#define AHISF_MIXING		(1<<1)
#define AHISF_TIMING		(1<<2)
#define AHISF_KNOWSTEREO	(1<<3)
#define AHISF_KNOWHIFI		(1<<4)
#define AHISF_CANRECORD 	(1<<5)
#define AHISF_CANPOSTPROCESS	(1<<6)
#define AHISF_KNOWMULTICHANNEL	(1<<7)

#define AHISB_ERROR		(0)
#define AHISB_MIXING		(1)
#define AHISB_TIMING		(2)
#define AHISB_KNOWSTEREO	(3)
#define AHISB_KNOWHIFI		(4)
#define AHISB_CANRECORD		(5)
#define AHISB_CANPOSTPROCESS	(6)
#define AHISB_KNOWMULTICHANNEL	(7)

 /* AHIsub_Start() and AHIsub_Stop() flags */
#define	AHISF_PLAY		(1<<0)
#define	AHISF_RECORD		(1<<1)

#define	AHISB_PLAY		(0)
#define	AHISB_RECORD		(1)

 /* ahiac_Flags */
#define	AHIACF_VOL		(1<<0)
#define	AHIACF_PAN		(1<<1)
#define	AHIACF_STEREO		(1<<2)
#define	AHIACF_HIFI		(1<<3)
#define	AHIACF_PINGPONG		(1<<4)
#define	AHIACF_RECORD		(1<<5)
#define AHIACF_MULTTAB  	(1<<6)			/* Private!		*/
#define	AHIACF_MULTICHANNEL	(1<<7)

#define	AHIACB_VOL		(0)
#define	AHIACB_PAN		(1)
#define	AHIACB_STEREO		(2)
#define	AHIACB_HIFI		(3)
#define	AHIACB_PINGPONG		(4)
#define	AHIACB_RECORD		(5)
#define AHIACB_MULTTAB  	(6)			/* Private!		*/
#define	AHIACB_MULTICHANNEL	(7)

 /* AHIsub_Set#? and AHIsub_(Un)LoadSound return code */
#define AHIS_UNKNOWN		(~0U)

 /* IFF chunk names for the audio mode file */
#define ID_AHIM		MAKE_ID('A','H','I','M')	/* AHI Modes		*/
#define ID_AUDN		MAKE_ID('A','U','D','N')	/* AUDio driver Name	*/
#define ID_AUDD		MAKE_ID('A','U','D','D')	/* AUDio driver Data	*/
#define ID_AUDM		MAKE_ID('A','U','D','M')	/* AUDio Mode		*/

#endif /* LIBRARIES_AHISUB_H */
