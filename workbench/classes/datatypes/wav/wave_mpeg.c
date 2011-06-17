/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom (fredrik@a500.org)
 */

#include "wave_mpeg.h"

DEC_SETUPPROTO(SetupMPEG) {
	return OK;
}

/* DEC_CLEANUPPROTO(CleanupMPEG) {
} */

DECODERPROTO_v2(DecodeMPEG) {
	struct ClassBase * libBase = data->libBase;
	struct MPEGA_STREAM *mpas;
	struct Hook bshook = {0};

	MPA_CTRL mpactrl = {
		NULL,	// bitstream access hook
		// layers 1 & 2 settings (mono, stereo)
		{ FALSE, { 2, 0, 44100 }, { 2, 0, 44100 } },
		// layer 3 settings (mono, stereo) 
		{ FALSE, { 2, 0, 44100 }, { 2, 0, 44100 } },
		1, // check for validity at start
		0 // size of bitstream buffer (0 -> default)
	};

	bshook.h_Entry = (HOOKFUNC)bshook_func;
	bshook.h_SubEntry = (HOOKFUNC)data;
	bshook.h_Data = (APTR)file;
	mpactrl.bs_access = &bshook;

	if (mpas = IMPEGA->MPEGA_open(NULL, &mpactrl)) {
		WORD *pcm[2];
		LONG pcm_count;

		pcm[0] = AllocVec(MPEGA_PCM_SIZE, MEMF_CLEAR);
		pcm[1] = AllocVec(MPEGA_PCM_SIZE, MEMF_CLEAR);

		if (pcm[0] && pcm[1]) {

			while ((pcm_count = IMPEGA->MPEGA_decode_frame(mpas, pcm)) >= 0) {
			}

		}

		FreeVec(pcm[0]);
		FreeVec(pcm[1]);
	}
}
