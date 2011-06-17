/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#include <proto/exec.h>
#include "wave_g72x.h"
#include "g72x/g72x.h"
#include "bitpack.h"
#include "endian.h"

struct G72x_Format {
    UWORD formatTag;
    WORD numChannels;
    LONG samplesPerSec;
    LONG avgBytesPerSec;
    WORD blockAlign; /* amount to read for each block */
    WORD bitsPerSample;

    WORD extraSize; /* 2 */
    WORD auxSize;
};

struct WAV_G72x_state {
	struct g72x_state stat[MAX_CHANNELS];
	int (*decoder)(int code, struct g72x_state *state_ptr);
};

DEC_SETUPPROTO(SetupG72x) {
	struct ClassBase * libBase;
	struct G72x_Format * fmt;
	struct WAV_G72x_state * state;
	libBase = data->libBase;
	LONG ch;
	fmt = (struct G72x_Format *)data->fmt;

	if (fmt->extraSize != 2)
		return NOTOK;
	//if (fmt->numChannels != 1)
	//	return ERROR_NOT_IMPLEMENTED;

	fmt->auxSize = le2nat16(fmt->auxSize);

	data->state = state = (struct WAV_G72x_state *)AllocVec(sizeof(*state), MEMF_CLEAR);
	if (!state) return ERROR_NO_FREE_STORE;

	for (ch = 0; ch < fmt->numChannels; ch++)
		g72x_init_state(&state->stat[ch]);

	switch (fmt->formatTag) {
		case WAVE_FORMAT_G721_ADPCM:
			switch (fmt->bitsPerSample) {
				case 4:
					fmt->blockAlign = 64*fmt->numChannels + fmt->auxSize;
					state->decoder = g721_decoder;
					break;
				default:
					return DTERROR_UNKNOWN_COMPRESSION;
			}
			break;
		case WAVE_FORMAT_G723_ADPCM:
			switch (fmt->bitsPerSample) {
				case 2:
					fmt->blockAlign = 32*fmt->numChannels + fmt->auxSize;
					state->decoder = g723_16_decoder;
					break;
				case 3:
					fmt->blockAlign = 48*fmt->numChannels + fmt->auxSize;
					state->decoder = g723_24_decoder;
					break;
				case 5:
					fmt->blockAlign = 80*fmt->numChannels + fmt->auxSize;
					state->decoder = g723_40_decoder;
					break;
				default:
					return DTERROR_UNKNOWN_COMPRESSION;
			}
			break;
		default:
			return DTERROR_UNKNOWN_COMPRESSION;
	}
	data->blockFrames = 128;
	return OK;
}

/*DEC_CLEANUPPROTO(CleanupG72x) {
	struct ClassBase * libBase;
	struct WAV_G72x_state * state;
	libBase = data->libBase;
	state = (struct WAV_G72x_state *)data->state;
	FreeVec(state);
}*/

DECODERPROTO(DecodeG72x) {
	struct WAV_G72x_state * state;
	BitPack_buffer bp;
	LONG fr, ch;
	state = (struct WAV_G72x_state *)data->state;
	bitpack_init(&bp, Src+((struct G72x_Format *)fmt)->auxSize, 0);
	for (fr = 0; fr < numFrames; fr++) {
		for (ch = 0; ch < fmt->numChannels; ch++) {
			*Dst[ch]++ =
				(state->decoder(bitpack_read_msb(&bp, fmt->bitsPerSample), &state->stat[ch]) >> 8);
		}
	}
	return numFrames;
}
