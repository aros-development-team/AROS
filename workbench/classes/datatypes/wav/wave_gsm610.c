/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#include "wave_gsm610.h"
#include "gsm/private.h"

struct GSM610_Format {
    UWORD formatTag;
    WORD numChannels;
    LONG samplesPerSec;
    LONG avgBytesPerSec;
    WORD blockAlign; /* amount to read for each block */
    WORD bitsPerSample;

    WORD extraSize; /* 2 */
    WORD samplesPerBlock;
};

struct gsmstate {
	gsm			handle;
	gsm_signal	*samples;
};

DEC_SETUPPROTO(SetupGSM610) {
	//int valP = 1;
	struct GSM610_Format * fmt;
	struct gsmstate * state;

	fmt = (struct GSM610_Format *)data->fmt;

	if (fmt->numChannels != 1)
		return ERROR_NOT_IMPLEMENTED;

	data->state = state = AllocVec(sizeof(*state), MEMF_CLEAR);
	if (!state) return ERROR_NO_FREE_STORE;

	// state->handle = gsm_create();
	state->handle = (gsm)AllocMem(sizeof(struct gsm_state), MEMF_CLEAR);
	if (!state->handle)
		return ERROR_NO_FREE_STORE;
	state->handle->nrp=40;

	// gsm_option(state->handle, GSM_OPT_WAV49, &valP);
	state->handle->wav_fmt = TRUE;

	fmt->blockAlign = 65;
	if (fmt->extraSize == 0 || fmt->samplesPerBlock == 0)
		data->blockFrames = 320;
	else if (fmt->extraSize == 2)
		data->blockFrames = read_le16(&fmt->samplesPerBlock);
	else
		return NOTOK;

	state->samples = AllocVec(2*data->blockFrames, MEMF_CLEAR);
	if (!state->samples)
		return ERROR_NO_FREE_STORE;

	return OK;
}

DEC_CLEANUPPROTO(CleanupGSM610) {
	struct gsmstate * state;

	state = (struct gsmstate *)data->state;

	FreeVec(state->samples);

	// gsm_destroy(state->handle);
	if (state->handle) FreeMem(state->handle, sizeof(struct gsm_state));

	FreeVec(state);
}

DECODERPROTO(DecodeGSM610) {
	struct gsmstate * state;
	gsm_signal *buff;
	LONG ch, fr;
	state = (struct gsmstate *)data->state;

	for (ch = 0; ch < fmt->numChannels; ch++) {
		/* decode the long 33 byte half */
		if (gsm_decode(state->handle, Src, state->samples) < 0) {
			return 0;
		}
		/* decode the short 32 byte half */
		if (gsm_decode(state->handle, Src+33, state->samples+160) < 0) {
			return 0;
		}
		buff = state->samples;
		for (fr = 0; fr < numFrames; fr++) {
			*Dst[ch]++ = (*buff++)>> 8;
		}
	}

	return numFrames;
}
