#ifndef	LIBRARIES_PTPLAY_H
#define	LIBRARIES_PTPLAY_H

/*
**	protracker 2.3a player
**	(C) 2001, 2003, 2004 Ronald Hof, Timm S. Mueller and Per Johansson
**
** Library modifications by Ilkka Lehtoranta
*/

#define	PTPLAY_CIAspeed			(TAG_USER + 0x00)	/* SG	ULONG		Default: 125					*/
#define	PTPLAY_Flags				(TAG_USER + 0x01)	/* SG ULONG		Default: MODF_ALLOWFILTER	*/
#define	PTPLAY_MasterVolume		(TAG_USER + 0x02)	/* SG	ULONG		Default: 256 					*/
#define	PTPLAY_PatternPosition	(TAG_USER + 0x04)	/* SG ULONG											*/
#define	PTPLAY_Patterns			(TAG_USER + 0x05)	/* .G ULONG		Number of patterns			*/
#define	PTPLAY_SongLength			(TAG_USER + 0x07)	/* .G	ULONG 	Song length in patterns		*/
#define	PTPLAY_SongLoopCount		(TAG_USER + 0x08)	/* .G	ULONG		Number of loops done			*/
#define	PTPLAY_SongPosition		(TAG_USER + 0x09)	/* SG ULONG		Current position				*/
#define	PTPLAY_SongTitle			(TAG_USER + 0x0A)	/* .G	STRPTR	Song name						*/
#define	PTPLAY_TotalTime			(TAG_USER + 0x0B)	/* .G ULONG 	Song length in seconds		*/

#define PT_MOD_UNKNOWN			0
#define PT_MOD_PROTRACKER		1
#define PT_MOD_SOUNDTRACKER	2
#define PT_MOD_SOUNDFX			3

/* Values for PTPLAY_Flags
 */

#define MODF_DOSONGEND		0x0001		/* perform song-end detection */
#define MODF_ALLOWPANNING	0x0002		/* allow mod to use command $8 for panning */
#define MODF_ALLOWFILTER	0x0004		/* allow mod to set lowpass filter */
#define MODF_SONGEND			0x0008		/* songend occured */

#endif	/* LIBRARIES_PTPLAY_H */