/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#include "wave_ieee_float.h"

DEC_SETUPPROTO(SetupIEEE_Float) {
	struct WaveFormatEx * fmt;
	fmt = data->fmt;
	if (fmt->extraSize != 0)
		return NOTOK;
	switch (fmt->bitsPerSample) {
		case 32:
			data->Decode = DecodeIEEE_Float_32;
			break;
		case 64:
			data->Decode = DecodeIEEE_Float_64;
			break;
		default:
			data->Decode = NULL;
			return DTERROR_UNKNOWN_COMPRESSION;
	}
	data->blockFrames = 1;
	return OK;
}

DECODERPROTO(DecodeIEEE_Float_32) {
	LONG frame,chan;
	for (frame=0;frame<numFrames;frame++) {
		for (chan=0;chan<fmt->numChannels;chan++) {
			union {
				FLOAT f;
				ULONG i;
			} samp;
			samp.i = read_le32(Src); Src += 4;
			*Dst[chan]++ = (BYTE)(samp.f * 127.0);
		}
	}
	return numFrames;
}

DECODERPROTO(DecodeIEEE_Float_64) {
	LONG frame,chan;
	for (frame=0;frame<numFrames;frame++) {
		for (chan=0;chan<fmt->numChannels;chan++) {
			union {
				DOUBLE f;
				UQUAD i;
			} samp;
			samp.i = read_le64(Src); Src += 8;
			*Dst[chan]++ = (BYTE)(samp.f * 127.0);
		}
	}
	return numFrames;
}
