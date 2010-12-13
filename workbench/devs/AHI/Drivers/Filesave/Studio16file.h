#ifndef STUDIO16FILE_H
#define STUDIO16FILE_H

/*
**	$VER: Studio16fileformat 2.0 (9.11.97)
**
**	The STUDIO16_2.0 format (KWK3)
**
**	Analysed by Kenneth "Kenny" Nilsen
**	mailto:kenny@bgnett.no
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/* Initializers */

#define S16FID		0x4B574B33  /* "KWK3" */
#define S16FINIT	0x1

/* File header structure
*
*  HEADER  (3690)
*  SAMPLES (samples x 16bit)
*/

#define	S16FILEHEADERSIZE	3690

/* I am no C programmer so please let me know if this one is incorrect: */

struct STUDIO16FILE
{
	ULONG	S16F_ID;		/* FILE HEADER ID (K16FID) */
	ULONG	S16F_RATE;		/* sample rate */
	ULONG	S16F_FILTER;		/* Init with $1 (K16FINIT) */
	UWORD	S16F_VOLUME;		/* sample volume */
	ULONG	S16F_SMPTE;		/* SMPTE timecode (H:B M:B S:B F:B) */
	ULONG	S16F_SMPTEFLOAT;	/* SMPTE sampling rate */
	ULONG	S16F_PAN;		/* pan setting */
	LONG	S16F_FLAGS;		/* misc flags */
	LONG	S16F_RESERVED;

/* here follows the sampleclips */

	ULONG	S16F_SAMPLES0;		/* number of samples (bytesize/2) */
	ULONG	S16F_SAMPLES1;		/* number of samples */
	ULONG	S16F_0;			/* start of first range [main] */
	ULONG	S16F_SAMPLES2;		/* number of samples-1  (end) */
	ULONG	S16F_EDITSTATUS;	/* if =null then no edits in file */
	char	S16F_1[3636];
/*	LABEL	S16F_SIZEOF	*/	/* =3690 */
};

/* Sample rates for Studio 16: */

#define	S16_FREQ_0	0x1589	/* 5513 hz */
#define	S16_FREQ_1	0x19D7	/* 6615 hz */
#define	S16_FREQ_2	0x1F40	/* 8000 hz */
#define	S16_FREQ_3	0x2580	/* 9600 hz */
#define	S16_FREQ_4	0x2B11	/* 11025 hz */
#define	S16_FREQ_5	0x3e80	/* 16000 hz */
#define	S16_FREQ_6	0x49D4	/* 18900 hz */
#define	S16_FREQ_7	0x5622	/* 22050 hz */
#define	S16_FREQ_8	0x6B25	/* 27429 hz */
#define	S16_FREQ_9	0x7D00	/* 32000 hz */
#define	S16_FREQ_A	0x8133	/* 33075 hz */
#define	S16_FREQ_B	0x93A8	/* 37800 hz */
#define	S16_FREQ_C	0xAC44	/* 44100 hz */
#define	S16_FREQ_D	0xBB80	/* 48000 hz */

/* Volumes */

#define	S16_VOL_6	0xD40	/* +6 dB */
#define	S16_VOL_0	0xC80	/* +0 dB */
#define	S16_VOL_40	0x780	/* Decibel = number-100 */
#define	S16_VOL_OFF	0	/* oo dB */

#define	S16_VOL_STEPS	0x20

/* Pans */

#define	S16_PAN_LEFT	0x0
#define	S16_PAN_MID	0xc80
#define	S16_PAN_RIGHT	0x1900

#define	S16_PAN_STEPS	0x20

/* NOTE: Fileclips and regions are not defined here!
*  Make sure to convert edits to permanent before converting a sample
*  FROM Studio16_2.0 to something else. */

#endif /* STUDIO16FILE_H */
