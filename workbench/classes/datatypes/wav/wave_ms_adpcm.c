/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

// M$ ADPCM format

#include "wave_ms_adpcm.h"
#include "bitpack.h"
#include "endian.h"

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif

struct coefset {
    WORD coef1;
    WORD coef2;
};

struct MS_ADPCM_Format {
    UWORD formatTag;
    WORD numChannels;
    LONG samplesPerSec;
    LONG avgBytesPerSec;
    WORD blockAlign; /* amount to read for each block */
    WORD bitsPerSample; /* 4 */

    WORD extraSize; /* 4+4*numCoefs */
    WORD samplesPerBlock;
    WORD numCoefs; // number of coef sets in file
    struct coefset aCoef[]; // numCoef coef sets
};

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack()
   #endif
#elif defined(__VBCC__)
   #pragma default-align
#endif

// Note: Coefs are fixed point 8.8 signed numbers.

/* block header:
 *
 * struct MS_ADPCM_Block {
 * 		BYTE bpredictor[nchannels];
 * 		WORD newdelta[nchannels];
 *		WORD isamp1[nchannels];
 *		WORD isamp2[nchannels];
 * };
 */

DEC_SETUPPROTO(SetupMS_ADPCM) {
	struct MS_ADPCM_Format * fmt;
	LONG i;
	fmt = (struct MS_ADPCM_Format *)data->fmt;

	if (fmt->bitsPerSample != 4)
		return DTERROR_UNKNOWN_COMPRESSION;
	fmt->numCoefs = read_le16(&fmt->numCoefs);
	if (4 + (fmt->numCoefs << 2) == data->chunk.size)
		return NOTOK;
	/* change endianness of coefsets */
	for (i = 0; i < fmt->numCoefs; i++) {
		fmt->aCoef[i].coef1 = (WORD)read_le16(&fmt->aCoef[i].coef1);
		fmt->aCoef[i].coef2 = (WORD)read_le16(&fmt->aCoef[i].coef2);
	}
	data->blockFrames = fmt->samplesPerBlock = read_le16(&fmt->samplesPerBlock);
	return OK;
}

static const WORD adaptiontable[16] = {
	230, 230, 230, 230, 307, 409, 512, 614,
	768, 614, 512, 409, 307, 230, 230, 230
};

struct ms_adpcm_stat {
	LONG nstep;
	LONG samp1, samp2;
	LONG coef1, coef2;
};

static LONG DecodeAdpcmSample (struct ms_adpcm_stat * stat, LONG err) {
	LONG step, nsamp;
	step = stat->nstep;
	stat->nstep = (adaptiontable[err] * step) >> 8;
	if (stat->nstep < 16) stat->nstep = 16;
	if (err & 8) err -= 16;
	nsamp = ((stat->samp1 * stat->coef1) + (stat->samp2 * stat->coef2)) >> 8;
	nsamp += (err * step);
	if (nsamp > 0x7FFF) nsamp = 0x7FFF;
	if (nsamp < -0x8000) nsamp = -0x8000;
	stat->samp2 = stat->samp1;
	stat->samp1 = nsamp;
	return nsamp;
}

DECODERPROTO(DecodeMS_ADPCM) {
	struct ms_adpcm_stat stat[MAX_CHANNELS];
	struct MS_ADPCM_Format *fmt_ext;

	LONG numChan;
	LONG numSamp, s, ch;

	fmt_ext=(struct MS_ADPCM_Format *)fmt;
	numChan = fmt_ext->numChannels;

	for (ch = 0; ch < numChan; ch++) {
		LONG bpred;
		bpred = *(UBYTE *)Src; Src++;
		if (bpred >= fmt_ext->numCoefs) bpred = 0;
		stat[ch].coef1 = fmt_ext->aCoef[bpred].coef1;
		stat[ch].coef2 = fmt_ext->aCoef[bpred].coef2;
	}
	for (ch = 0; ch < numChan; ch++) {
		stat[ch].nstep = (WORD)read_le16(Src); Src+=2;
	}
	for (ch = 0; ch < numChan; ch++) {
		stat[ch].samp1 = (WORD)read_le16(Src); Src+=2;
	}
	for (ch = 0; ch < numChan; ch++) {
		stat[ch].samp2 = (WORD)read_le16(Src); Src+=2;
		switch (numFrames) {
			default:
				Dst[ch][1] = stat[ch].samp1 >> 8;
			case 1:
				Dst[ch][0] = stat[ch].samp2 >> 8;
			case 0:
				break;
		}
		Dst[ch]+=2;
	}

	if (numFrames <= 2) return numFrames;
	numSamp = (numFrames-2) * numChan;
	ch = 0;
	{
		LONG tmp = numSamp >> 1;
		for (s = 0; s < tmp; s++) {
			*Dst[ch]++ = DecodeAdpcmSample(&stat[ch], *Src >> 4) >> 8;
			if (++ch == numChan) ch = 0;
			*Dst[ch]++ = DecodeAdpcmSample(&stat[ch], *Src & 0xF) >> 8;
			if (++ch == numChan) ch = 0;
			Src++;
		}
	}
	if (numSamp & 1) {
		*Dst[ch]++ = DecodeAdpcmSample(&stat[ch], *Src >> 4) >> 8;
	}

	return numFrames;
}
