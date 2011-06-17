/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

// Note: DVI & IMA formats are identical

#include "wave_ima_adpcm.h"
#include "bitpack.h"
#include "endian.h"

struct IMA_ADPCM_Format {
    UWORD formatTag;
    WORD numChannels;
    LONG samplesPerSec;
    LONG avgBytesPerSec;
    WORD blockAlign; /* amount to read for each block */
    WORD bitsPerSample; /* 3 or 4 */

    WORD extraSize; /* 2 */
    WORD samplesPerBlock;
};

DEC_SETUPPROTO(SetupIMA_ADPCM) {
	struct IMA_ADPCM_Format * fmt;

	fmt = (struct IMA_ADPCM_Format *)data->fmt;

	/* check bitsPerSample */
	if (fmt->bitsPerSample != 3 && fmt->bitsPerSample != 4) {
		return DTERROR_UNKNOWN_COMPRESSION;
	}
	if (data->chunk.size != sizeof(*fmt)) {
		return NOTOK;
	}
	data->blockFrames = fmt->samplesPerBlock = read_le16(&fmt->samplesPerBlock);
	return OK;
}

static const WORD steptab[89] = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

/* 3 bps */
static const BYTE indextab3[8] = {
    -1, -1, 1, 2,
    -1, -1, 1, 2
};

/* 4 bps */
static const BYTE indextab4[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

/* Each block has nchannels of these */
struct IMA_BlockHeader {
    UBYTE isamp0[2]; /* Prev sample ro start decoding with (int) */
    BYTE stepTableIndex; /* Current index in steptable array (0-88) */
    UBYTE reserved;
};

DECODERPROTO(DecodeIMA_ADPCM) {
	LONG Index[MAX_CHANNELS],Diff,Value[MAX_CHANNELS];
	WORD Code;
	LONG sampleSize,frame,frame2,chan;
	LONG bframes,skip=0,t;
	const BYTE *indextab;
	BitPack_buffer b;

	sampleSize = fmt->bitsPerSample;

	if (sampleSize == 3) {
		indextab = indextab3;
		bframes = 32;
	} else {
		indextab = indextab4;
		bframes = 8;
	}

	for (chan=0;chan<fmt->numChannels;chan++) {
		Value[chan] = (WORD)read_le16(Src);Src+=2;
		Index[chan] = *Src;Src+=2;
		*Dst[chan]++ = Value[chan]>>8;
	}

	bitpack_init_lsb(&b, Src, 0);

	for (frame=1;frame<numFrames;frame+=bframes) {
		t=numFrames-frame;
		if (t < bframes) {
			skip = t * sampleSize;
			bframes -= t;
		}
		for (chan=0;chan<fmt->numChannels;chan++) {
			for (frame2=0;frame2<bframes;frame2++) {

				/* Step 1 - Get delta value */

				Code=bitpack_read_lsb(&b, sampleSize);

				/* Step 2 - Calculate difference and new expected value */

				{
					LONG Step,mask;
					Step = steptab[Index[chan]];
					Diff = 0;
					for (mask=1<<(sampleSize-2);mask;mask>>=1,Step>>=1) {
						if (Code & mask) {
							Diff += Step;
						}
					}
					Diff += Step;
					if (Code & (1<<(sampleSize-1))) {
						Value[chan] -= Diff;
						if (Value[chan]<-32678) Value[chan]=-32678;
					} else {
						Value[chan] += Diff;
						if (Value[chan]>32767) Value[chan]=32767;
					}
				}

				/* Step 3 - Find next index value */

				Index[chan]+=indextab[Code];

				if (Index[chan]<0) Index[chan]=0;
				if (Index[chan]>88) Index[chan]=88;

				/* Step 4 - Output value */

				*Dst[chan]++ = Value[chan]>>8;

			}
			if (skip)
				bitpack_seek_lsb(&b,OFFSET_CURRENT,skip);
		}
	}

	return numFrames;
}
