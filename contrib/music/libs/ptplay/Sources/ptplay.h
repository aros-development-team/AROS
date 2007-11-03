#ifndef LIBRARIES_PTREPLAY_H
#define LIBRARIES_PTREPLAY_H

/*
**	protracker 2.3a player
**	(C) 2001, 2003 Ronald Hof and Timm S. Mueller
*/

#define PT_MOD_UNKNOWN		0
#define PT_MOD_PROTRACKER	1

#define MODF_DOSONGEND		0x0001		/* perform song-end detection */
#define MODF_ALLOWPANNING	0x0002		/* allow mod to use command $8 for panning */
#define MODF_ALLOWFILTER	0x0004		/* allow mod to set lowpass filter */
#define MODF_SONGEND			0x0008		/* songend occured */

struct PlayMod
{
}; 

struct PatternData
{
	LONG period, sample;
	UBYTE effect, efd1, efd2, efboth;
	LONG pad;
};

struct Pattern
{
	struct PatternData data[64][4];
};

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

#endif	/* LIBRARIES_PTPLAY_H */
