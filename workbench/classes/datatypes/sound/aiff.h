
/*
**	aiff.h
**	stolen from Olaf Barthels' aiff.dt
*/

#ifndef AIFF_H
#define AIFF_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

typedef struct {
	UWORD	exponent;			// Exponent, bit #15 is sign bit for mantissa
	ULONG	mantissa[2];			// 64 bit mantissa
} extended;

// Audio Interchange Format chunk data

#define ID_AIFF	MAKE_ID('A', 'I', 'F', 'F')

#define ID_COMM 	MAKE_ID('C', 'O', 'M', 'M')
#define ID_SSND 	MAKE_ID('S', 'S', 'N', 'D')

// "COMM" chunk header

typedef struct {
	WORD		numChannels;		// Number of channels
	ULONG		numSampleFrames;	// Number of sample frames
	WORD		sampleSize;		// Number of bits per sample point
	extended	sampleRate;		// Replay rate in samples per second
} CommonChunk;

// "SSND" chunk header

typedef struct {
	ULONG	offset,				// Offset to sound data, for block alignment
			blockSize;			// Size of block data is aligned to
} SampledSoundHeader;

static void ulong2extended( ULONG value, extended *ex )
{
	ex->exponent = 31 + 16383;

	while( ( value & 0x80000000 ) == 0 )
	{
		value <<= 1;

		ex->exponent--;
	}

	ex->mantissa[0] = value;
	ex->mantissa[1] = 0;
}

#endif
