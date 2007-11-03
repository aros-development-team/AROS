
#ifndef _PTREPLAY_H
#define _PTREPLAY_H

/*
**	protracker 2.3a player
**	(C) 2001, 2003 Ronald Hof and Timm S. Mueller
*/

#if defined(AMIGA) || defined(__VBCC__)
	#include <exec/types.h>
#else
	typedef signed char BYTE;
	typedef unsigned char UBYTE;
	typedef signed short WORD;
	typedef unsigned short UWORD;
	typedef signed int LONG;
	typedef unsigned int ULONG;
	#ifndef TRUE
	#define	TRUE 1
	#endif
	#ifndef FALSE
	#define	FALSE 0
	#endif
	#ifndef NULL
	#define	NULL 0
	#endif
#endif

#ifdef	__VBCC__
#define M_PI				3.14159265358979323846  /* pi */
#endif

#ifndef LIBAPI
#define LIBAPI
#endif

#define PT_MOD_UNSUPPORTED		-1
#define PT_MOD_UNKNOWN			0
#define PT_MOD_PROTRACKER		1
#define PT_MOD_SOUNDTRACKER	2
#define PT_MOD_SOUNDFX			3

#define MODF_DOSONGEND		0x0001		/* perform song-end detection */
#define MODF_ALLOWPANNING	0x0002		/* allow mod to use command $8 for panning */
#define MODF_ALLOWFILTER	0x0004		/* allow mod to set lowpass filter */
#define MODF_SONGEND			0x0008		/* songend occured */

typedef struct
{
	UBYTE	name[32];
	BYTE *data;
	LONG length, repeat, replen, repend;		/* <<14, includes oversampling precision */
	LONG ft;
	LONG volume;
	LONG pad;
}pt_sample_s;	/* 32 bytes */


/* -> UBYTE <- cutoff, loopcount, retrig, delay	*/

typedef struct
{
	LONG sample, prevsample, sp, per, freq, oldsp;
	LONG dp, basevol, prevvol, vol, dvol, pan, ft;
	ULONG flags;
	LONG dtp, tptarget, tpsrc;
	LONG retrig, cutoff, delay;
	LONG arp, arpbase;
	LONG vibspeed, vibdepth, avibspeed, avibindex;
	LONG trespeed, tredepth, atrespeed, atreindex;
	LONG vibwave, tremwave, glissando, funkoffset, funkcnt;
	LONG delaysample;
	LONG loopstart, loopcount, loopflg;
} pt_channel_s;

typedef struct
{
	LONG period, sample;
	UBYTE effect, efd1, efd2, efboth;
//	LONG pad;
} pt_patterndata_s;	/* 16 bytes */

typedef struct
{
	pt_patterndata_s data[64][4];
}pt_pattern_s;

typedef struct
{
	char	name[23];
	UBYTE	modformat;
	UWORD	numpat, length;
	pt_sample_s		sample[32];
	pt_channel_s	chan[4];
	pt_pattern_s	*pattern;
	UBYTE	pos[128];
	ULONG flags;
	LONG	freq;
	LONG	mastervolume;
	UBYTE	spos, ppos, speed, ciaspeed;
	LONG	vbllen, bpos;
	UBYTE	tick, patdelay, skiptopos, filter;
	ULONG	songloopcount;
	FLOAT flta, fltb;
	FLOAT fltpi[4];
	FLOAT fltpo[4];
	ULONG	PlayLength;	// in secs
}pt_mod_s;

#ifndef	__MORPHOS__
#define	AllocVecTaskPooled(x)	AllocVec(x, MEMF_PUBLIC)
#define	FreeVecTaskPooled(x)		FreeVec(x)
#endif

#ifndef	MAKE_ID
#define	MAKE_ID(a,b,c,d)	((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#define	PTPLAY_CIAspeed			(TAG_USER + 0x00)	/* SG	ULONG					Default: 125					*/
#define	PTPLAY_Flags				(TAG_USER + 0x01)	/* SG ULONG					Default: MODF_ALLOWFILTER	*/
#define	PTPLAY_MasterVolume		(TAG_USER + 0x02)	/* SG	ULONG					Default: 256 					*/
#define	PTPLAY_PatternData		(TAG_USER + 0x03)	/* .G struct Pattern *										*/
#define	PTPLAY_PatternPosition	(TAG_USER + 0x04)	/* SG ULONG														*/
#define	PTPLAY_Patterns			(TAG_USER + 0x05)	/* .G ULONG														*/
#define	PTPLAY_Positions			(TAG_USER + 0x06)	/* .G	ULONG *													*/
#define	PTPLAY_SongLength			(TAG_USER + 0x07)	/* .G	ULONG														*/
#define	PTPLAY_SongLoopCount		(TAG_USER + 0x08)	/* .G	ULONG														*/
#define	PTPLAY_SongPosition		(TAG_USER + 0x09)	/* SG ULONG														*/
#define	PTPLAY_SongTitle			(TAG_USER + 0x0A)	/* .G	STRPTR													*/
#define	PTPLAY_TotalTime			(TAG_USER + 0x0B)	/* .G ULONG														*/

#define	PTV_TEST_FREQUENCY	50

pt_mod_s *pt_init(unsigned char *buf, int bufsize, int freq);

#endif
