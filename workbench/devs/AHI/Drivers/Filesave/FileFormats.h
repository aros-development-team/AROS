#ifndef AHI_Drivers_Filesave_FileFormats_h
#define AHI_Drivers_Filesave_FileFormats_h

#include <exec/types.h>

#if !defined( __mc68000__)
#pragma pack(2)
#endif

/******************************************************************************
** 8SVX defs ******************************************************************
******************************************************************************/

/* From Jerry Morrison and Steve Hayes, Electronic Arts */

#define ID_8SVX      MAKE_ID('8', 'S', 'V', 'X')
#define ID_VHDR      MAKE_ID('V', 'H', 'D', 'R')
#define ID_BODY      MAKE_ID('B', 'O', 'D', 'Y')

/* sCompression: Choice of compression algorithm applied to the samples. */
#define sCmpNone       0	/* not compressed */
#define sCmpFibDelta   1	/* Fibonacci-delta encoding (Appendix C) */
				/* Could be more kinds in the future. */
typedef struct {
    ULONG oneShotHiSamples,	/* # samples in the high octave 1-shot part */
          repeatHiSamples,	/* # samples in the high octave repeat part */
          samplesPerHiCycle;	/* # samples/cycle in high octave, else 0 */
    UWORD samplesPerSec;	/* data sampling rate */
    UBYTE ctOctave,		/* # of octaves of waveforms */
          sCompression;		/* data compression technique used */
    Fixed volume;		/* playback nominal volume from 0 to Unity
				 * (full volume). Map this value into
				 * the output hardware's dynamic range.
				 */
    } Voice8Header;

/******************************************************************************
** AIFF and AIFC defs *********************************************************
******************************************************************************/

/* AIFF and AIFC defines were taken from Olaf `Olsen' Barthel's AIFF DataType. */

	// 80 bit IEEE Standard 754 floating point number

typedef struct {
	unsigned short	exponent;		// Exponent, bit #15 is sign bit for mantissa
	unsigned long	mantissa[2];		// 64 bit mantissa
} extended;

	// Audio Interchange Format chunk data

#define ID_AIFF MAKE_ID('A','I','F','F')
#define ID_AIFC MAKE_ID('A','I','F','C')

#define ID_FVER MAKE_ID('F','V','E','R')
#define ID_COMM MAKE_ID('C','O','M','M')
#define ID_SSND MAKE_ID('S','S','N','D')

	// "COMM" chunk header

typedef struct {
	short		numChannels;		// Number of channels
	unsigned long	numSampleFrames;	// Number of sample frames
	short		sampleSize;		// Number of bits per sample point
	extended	sampleRate;		// Replay rate in samples per second
} CommonChunk;

	// The same for "AIFC" type files

#define NO_COMPRESSION MAKE_ID('N','O','N','E') // No sound compression

typedef struct {
	short		numChannels;		// Number of channels
	unsigned long	numSampleFrames;	// Number of sample frames
	short		sampleSize;		// Number of bits per sample point
	extended	sampleRate;		// Replay rate in samples per second
	unsigned long	compressionType;	// Compression type
	char		compressionName[(sizeof("not compressed")+1)&(~1)];
} ExtCommonChunk;


	// "SSND" chunk header

typedef struct {
	unsigned long	offset, 		// Offset to sound data, for block alignment
			blockSize;		// Size of block data is aligned to
} SampledSoundHeader;

	// "FVER" chunk header

typedef struct {
	long		timestamp;		// Format version creation date
} FormatVersionHeader;

#define AIFCVersion1 0xA2805140 		// "AIFC" file format version #1


struct AIFCheader {
  ULONG                 FORMid;
  ULONG                 FORMsize;
  ULONG                 AIFCid;

  ULONG                 FVERid;
  ULONG                 FVERsize;
  FormatVersionHeader   FVERchunk;

  ULONG                 COMMid;
  ULONG                 COMMsize;
  ExtCommonChunk        COMMchunk;

  ULONG                 SSNDid;
  ULONG                 SSNDsize;
  SampledSoundHeader    SSNDchunk;
};

struct AIFFheader {
  ULONG                 FORMid;
  ULONG                 FORMsize;
  ULONG                 AIFFid;

  ULONG                 COMMid;
  ULONG                 COMMsize;
  CommonChunk           COMMchunk;

  ULONG                 SSNDid;
  ULONG                 SSNDsize;
  SampledSoundHeader    SSNDchunk;
};

struct EIGHTSVXheader {
  ULONG                 FORMid;
  ULONG                 FORMsize;
  ULONG                 EIGHTSVXid;

  ULONG                 VHDRid;
  ULONG                 VHDRsize;
  Voice8Header          VHDRchunk;

  ULONG                 BODYid;
  ULONG                 BODYsize;
};


/******************************************************************************
** WAV file format ************************************************************
******************************************************************************/

#define ID_RIFF MAKE_ID('R','I','F','F')
#define ID_WAVE MAKE_ID('W','A','V','E')
#define ID_fmt  MAKE_ID('f','m','t',' ')
#define ID_data MAKE_ID('d','a','t','a')


typedef struct {
	unsigned short	formatTag;
	unsigned short	numChannels;
	unsigned long	samplesPerSec;
	unsigned long	avgBytesPerSec;
	unsigned short	blockAlign;
	unsigned short	bitsPerSample;
} FormatChunk;

#define WAVE_PCM 1


struct WAVEheader {
  ULONG                 FORMid;
  ULONG                 FORMsize;
  ULONG                 WAVEid;

  ULONG                 FORMATid;
  ULONG                 FORMATsize;
  FormatChunk           FORMATchunk;
  
  ULONG                 DATAid;
  ULONG                 DATAsize;
};

/******************************************************************************
** S16 defs *******************************************************************
******************************************************************************/

#include "Studio16file.h"

#if !defined( __mc68000__)
#pragma pack()
#endif

#endif /* AHI_Drivers_Filesave_FileFormats_h */
