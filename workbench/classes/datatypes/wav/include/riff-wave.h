/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#ifndef RIFF_WAVE_H
#define RIFF_WAVE_H 1

#ifndef ENDIAN_H
#include "endian.h"
#endif

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif

/* Chunk IDs */

#define ID_RIFF 	MAKE_ID('R','I','F','F')
#define ID_WAVE	MAKE_ID('W','A','V','E')
#define ID_fmt	MAKE_ID('f','m','t',' ')
#define ID_fact	MAKE_ID('f','a','c','t')
#define ID_data	MAKE_ID('d','a','t','a')

struct RIFFChunk {
	ULONG type;
	ULONG size;
};

/* chunksize=16 */
struct WaveFormat {
	UWORD formatTag;
	UWORD numChannels; /* 1 mono, 2 stereo */
	ULONG samplesPerSec;
	ULONG avgBytesPerSec;
	UWORD blockAlign;
	UWORD bitsPerSample;
};

/* chunksize>=18 */
struct WaveFormatEx {
	UWORD formatTag;
	UWORD numChannels; /* 1 mono, 2 stereo */
	ULONG samplesPerSec;
	ULONG avgBytesPerSec;
	UWORD blockAlign;
	UWORD bitsPerSample;
	UWORD extraSize;
};

#if !defined(__AROS__)
#define Format WaveFormat
#endif
#define FormatEx WaveFormatEx

/* formatTag values */

#include "wave_formats.h"

/* WAVE_FORMAT_EXTENSIBLE format (unsupported as of yet): */

typedef struct {
	ULONG f1;
	UWORD f2;
	UWORD f3;
	UBYTE f4[8];
} _GUID;

/* chunksize>=40 */
struct WaveFormatExtensible {
	UWORD formatTag; // WAVE_FORMAT_EXTENSIBLE
	UWORD numChannels; /* 1 mono, 2 stereo */
	ULONG samplesPerSec;
	ULONG avgBytesPerSec;
	UWORD blockAlign;
	UWORD bitsPerSample;
	UWORD extraSize; /* >=22 */

	/* I have no idea how the following is to be read!!?? */
	union {
		UWORD validBitsPerSample;
		UWORD samplesPerBlock;
		UWORD reserved;
	} Samples;
	/* channel mask */
	ULONG channelMask;
	_GUID SubFmt;
};

/* GUID SubFormat IDs */

#ifndef _DATAFORMAT_SUBTYPE_PCM_
#define _DATAFORMAT_SUBTYPE_PCM_ {0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}}
//static const _GUID DATAFORMAT_SUBTYPE_PCM = {0xE923AABF, 0xCB58, 0x4471, {0xA1, 0x19, 0xFF, 0xFA, 0x01, 0xE4, 0xCE, 0x62}};
#endif

#ifndef _DATAFORMAT_SUBTYPE_UNKNOWN_
#define _DATAFORMAT_SUBTYPE_UNKNOWN_ {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
//static const _GUID DATAFORMAT_SUBTYPE_UNKNOWN = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
#endif

/* Microsoft speaker definitions */
#define WAVE_SPEAKER_FRONT_LEFT				0x1
#define WAVE_SPEAKER_FRONT_RIGHT				0x2
#define WAVE_SPEAKER_FRONT_CENTER			0x4
#define WAVE_SPEAKER_LOW_FREQUENCY			0x8
#define WAVE_SPEAKER_BACK_LEFT				0x10
#define WAVE_SPEAKER_BACK_RIGHT				0x20
#define WAVE_SPEAKER_FRONT_LEFT_OF_CENTER	0x40
#define WAVE_SPEAKER_FRONT_RIGHT_OF_CENTER	0x80
#define WAVE_SPEAKER_BACK_CENTER				0x100
#define WAVE_SPEAKER_SIDE_LEFT				0x200
#define WAVE_SPEAKER_SIDE_RIGHT				0x400
#define WAVE_SPEAKER_TOP_CENTER				0x800
#define WAVE_SPEAKER_TOP_FRONT_LEFT			0x1000
#define WAVE_SPEAKER_TOP_FRONT_CENTER		0x2000
#define WAVE_SPEAKER_TOP_FRONT_RIGHT			0x4000
#define WAVE_SPEAKER_TOP_BACK_LEFT			0x8000
#define WAVE_SPEAKER_TOP_BACK_CENTER			0x10000
#define WAVE_SPEAKER_TOP_BACK_RIGHT			0x20000
#define WAVE_SPEAKER_RESERVED				0x80000000

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack()
   #endif
#elif defined(__VBCC__)
   #pragma default-align
#endif

#endif /* RIFF_WAVE_H */
