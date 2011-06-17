/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#include "wave_alaw.h"

static WORD decode_alaw (UBYTE a_val);
static WORD decode_mulaw(UBYTE u_val);

DEC_SETUPPROTO(SetupALaw) {
	struct WaveFormatEx * fmt;
	fmt = data->fmt;
	if (fmt->extraSize != 0)
		return NOTOK;
	if (fmt->bitsPerSample != 8)
		return ERROR_NOT_IMPLEMENTED;
	data->blockFrames = 1;
	return OK;
}

DECODERPROTO(DecodeALaw) {
	LONG frame,chan;
	for (frame=0;frame<numFrames;frame++) {
		for (chan=0;chan<fmt->numChannels;chan++) {
			*Dst[chan]++ = decode_alaw(*Src++) >> 8;
		}
	}
	return numFrames;
}

DECODERPROTO(DecodeMuLaw) {
	LONG frame,chan;
	for (frame=0;frame<numFrames;frame++) {
		for (chan=0;chan<fmt->numChannels;chan++) {
			*Dst[chan]++ = decode_mulaw(*Src++) >> 8;
		}
	}
	return numFrames;
}

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */
#define	BIAS		(0x84)		/* Bias for linear code. */

static WORD decode_alaw (UBYTE a_val) {
	WORD t;
	WORD seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}

static WORD decode_mulaw(UBYTE u_val) {
	WORD t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}
