/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#include "wave_class.h"
#include "decoders.h"

#include "wave_pcm.h"
#include "wave_ima_adpcm.h"
#include "wave_ms_adpcm.h"
#include "wave_alaw.h"
#include "wave_ieee_float.h"
#include "wave_gsm610.h"
#include "wave_g72x.h"

DECODERPROTO(DecodeBlocks);

static struct Decoder all_decoders[] = {
	{ WAVE_FORMAT_PCM,			0,	SetupPCM,			NULL,			DecodePCM,			NULL			},
	{ WAVE_FORMAT_IMA_ADPCM,	0,	SetupIMA_ADPCM,		NULL,			DecodeBlocks,		DecodeIMA_ADPCM	},
	{ WAVE_FORMAT_ADPCM,		0,	SetupMS_ADPCM,		NULL,			DecodeBlocks,		DecodeMS_ADPCM	},
	{ WAVE_FORMAT_IEEE_FLOAT,	0,	SetupIEEE_Float,	NULL,			NULL,				NULL			},
	{ WAVE_FORMAT_ALAW,			0,	SetupALaw,			NULL,			DecodeALaw,			NULL			},
	{ WAVE_FORMAT_MULAW,		0,	SetupALaw,			NULL,			DecodeMuLaw,		NULL			},
	#ifdef GSM610_SUPPORT
	{ WAVE_FORMAT_GSM610,		0,	SetupGSM610,		CleanupGSM610,	DecodeBlocks,		DecodeGSM610	},
	#endif
	#ifdef G72X_SUPPORT
	{ WAVE_FORMAT_G721_ADPCM,	0,	SetupG72x,			NULL,			DecodeBlocks,		DecodeG72x		},
	{ WAVE_FORMAT_G723_ADPCM,	0,	SetupG72x,			NULL,			DecodeBlocks,		DecodeG72x		},
	#endif
	{ 0 }
};

struct Decoder * GetDecoder (UWORD fmtTag) {
	struct Decoder *dec;
	for (dec = all_decoders; dec->formatTag; dec++) {
		if (dec->formatTag == fmtTag)
			return dec;
	}
	return NULL;
}

DECODERPROTO(DecodeBlocks) {
	LONG status;
	LONG frames_left;
	LONG frames;
	LONG blocksize;

	frames_left = numFrames;
	frames = data->blockFrames;
	blocksize = fmt->blockAlign;
	while (frames_left > 0) {
		if (frames_left < frames) frames = frames_left;

		status = data->DecodeFrames(data, fmt, Src, Dst, 1, frames);
		if (status != frames) {
			return numFrames-frames_left+status;
		}
		Src += blocksize;

		frames_left -= frames;
	}
	return numFrames;
}
